/*
 * Copyright (c) [2004-2012] Novell, Inc.
 *
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, contact Novell, Inc.
 *
 * To contact Novell about this file by physical or electronic mail, you may
 * find current contact information at www.novell.com.
 */


#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <glob.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <pwd.h>
#include <dirent.h>
#include <mntent.h>
#include <libxml/tree.h>
#include <boost/algorithm/string.hpp>

#include "snapper/Log.h"
#include "snapper/AppUtil.h"


namespace snapper
{
    using namespace std;


    bool
    checkDir(const string& Path_Cv)
    {
	struct stat Stat_ri;
	return stat(Path_Cv.c_str(), &Stat_ri) >= 0 && S_ISDIR(Stat_ri.st_mode);
    }


    list<string>
    glob(const string& path, int flags)
    {
	list<string> ret;

	glob_t globbuf;
	if (glob(path.c_str(), flags, 0, &globbuf) == 0)
	{
	    for (char** p = globbuf.gl_pathv; *p != 0; p++)
		ret.push_back(*p);
	}
	globfree (&globbuf);

	return ret;
    }


    bool
    clonefile(int src_fd, int dest_fd)
    {
#define BTRFS_IOCTL_MAGIC 0x94
#define BTRFS_IOC_CLONE _IOW(BTRFS_IOCTL_MAGIC, 9, int)

	int r1 = ioctl(dest_fd, BTRFS_IOC_CLONE, src_fd);
	if (r1 != 0)
	{
	    y2err("ioctl failed errno:" << errno << " (" << stringerror(errno) << ")");
	}

	return r1 == 0;
    }


    bool
    copyfile(int src_fd, int dest_fd)
    {
	struct stat src_stat;
	int r1 = fstat(src_fd, &src_stat);
	if (r1 != 0)
	{
	    y2err("fstat failed errno:" << errno << " (" << stringerror(errno) << ")");
	    return false;
	}

	posix_fadvise(src_fd, 0, src_stat.st_size, POSIX_FADV_SEQUENTIAL);

	static_assert(sizeof(off_t) >= 8, "off_t is too small");

	const off_t block_size = 4096;

	char block[block_size];

	off_t length = src_stat.st_size;
	while (length > 0)
	{
	    off_t t = min(block_size, length);

	    int r2 = read(src_fd, block, t);
	    if (r2 != t)
	    {
		y2err("read failed errno:" << errno << " (" << stringerror(errno) << ")");
		return false;
	    }

	    int r3 = write(dest_fd, block, t);
	    if (r3 != t)
	    {
		y2err("write failed errno:" << errno << " (" << stringerror(errno) << ")");
		return false;
	    }

	    length -= t;
	}

	return true;
    }


    int
    readlink(const string& path, string& buf)
    {
	char tmp[1024];
	int ret = ::readlink(path.c_str(), tmp, sizeof(tmp));
	if (ret >= 0)
	    buf = string(tmp, ret);
	return ret;
    }


    int
    symlink(const string& oldpath, const string& newpath)
    {
	return ::symlink(oldpath.c_str(), newpath.c_str());
    }


    string
    realpath(const string& path)
    {
	char* buf = ::realpath(path.c_str(), NULL);
	if (!buf)
	    return string();
	string s(buf);
	free(buf);
	return s;
    }


    bool
    getMtabData(const string& mount_point, bool& found, MtabData& mtab_data)
    {
	FILE* f = setmntent("/etc/mtab", "r");
	if (!f)
	{
	    y2err("setmntent failed");
	    return false;
	}

	found = false;

	struct mntent* m;
	while ((m = getmntent(f)))
	{
	    if (strcmp(m->mnt_type, "rootfs") == 0)
		continue;

	    if (m->mnt_dir == mount_point)
	    {
		found = true;
		mtab_data.device = m->mnt_fsname;
		mtab_data.dir = m->mnt_dir;
		mtab_data.type = m->mnt_type;
		boost::split(mtab_data.options, m->mnt_opts, boost::is_any_of(","),
			     boost::token_compress_on);
		break;
	    }
	}

	endmntent(f);

	return true;
    }


    string
    stringerror(int errnum)
    {
#if (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && ! _GNU_SOURCE
	char buf1[100];
	if (strerror_r(errno, buf1, sizeof(buf1)-1) == 0)
	    return string(buf1);
	return string("strerror failed");
#else
	char buf1[100];
	const char* buf2 = strerror_r(errno, buf1, sizeof(buf1)-1);
	return string(buf2);
#endif
    }


    string
    sformat(const string& format, ...)
    {
	char* result;
	string str;

	va_list ap;
	va_start(ap, format);
	if (vasprintf(&result, format.c_str(), ap) != -1)
	{
	    str = result;
	    free(result);
	}
	va_end(ap);

	return str;
    }


    string
    hostname()
    {
	struct utsname buf;
	if (uname(&buf) != 0)
	    return string("unknown");
	string hostname(buf.nodename);
	if (strlen(buf.domainname) > 0)
	    hostname += "." + string(buf.domainname);
	return hostname;
    }


    string
    datetime(time_t t1, bool utc, bool classic)
    {
	struct tm t2;
	utc ? gmtime_r(&t1, &t2) : localtime_r(&t1, &t2);
	char buf[64 + 1];
	if (strftime(buf, sizeof(buf), classic ? "%F %T" : "%c", &t2) == 0)
	    return string("unknown");
	return string(buf);
    }


    time_t
    scan_datetime(const string& str, bool utc)
    {
	struct tm s;
	memset(&s, 0, sizeof(s));
	const char* p = strptime(str.c_str(), "%F %T", &s);
	if (!p || *p != '\0')
	    return (time_t)(-1);
	return utc ? timegm(&s) : timelocal(&s);
    }


    string
    username(uid_t uid)
    {
	struct passwd pwd;
	struct passwd* result;

	long bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
	char buf[bufsize];

	if (getpwuid_r(uid, &pwd, buf, bufsize, &result) != 0 || result != &pwd)
	    return "unknown";

	return pwd.pw_name;
    }


    StopWatch::StopWatch()
    {
	gettimeofday(&start_tv, NULL);
    }


    std::ostream& operator<<(std::ostream& s, const StopWatch& sw)
    {
	struct timeval stop_tv;
	gettimeofday(&stop_tv, NULL);

	struct timeval tv;
	timersub(&stop_tv, &sw.start_tv, &tv);

	return s << fixed << double(tv.tv_sec) + (double)(tv.tv_usec) / 1000000.0 << "s";
    }

}
