/*
 * Copyright (c) [2004-2015] Novell, Inc.
 * Copyright (c) [2016-2020] SUSE LLC
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
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <dirent.h>
#include <mntent.h>
#include <boost/algorithm/string.hpp>
#include <boost/io/ios_state.hpp>
#include <boost/scoped_array.hpp>

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
	    // TODO: too much logging with LVM
	    // y2err("ioctl failed errno:" << errno << " (" << stringerror(errno) << ")");
	}

	return r1 == 0;
    }


    bool
    copyfile(int src_fd, int dest_fd)
    {
	posix_fadvise(src_fd, 0, 0, POSIX_FADV_SEQUENTIAL);

	// TODO: maybe use POSIX_FADV_DONTNEED on dest_fd, but this could
	// trigger a kernel bug (see bsc #888259)

	while (true)
	{
	    // use small value for count to make function better interruptible
	    ssize_t r1 = sendfile(dest_fd, src_fd, NULL, 0x10000);
	    if (r1 == 0)
		return true;

	    if (r1 < 0)
	    {
		y2err("sendfile failed errno:" << errno << " (" << stringerror(errno) << ")");
		return false;
	    }
	}
    }


    ssize_t
    readlink(const string& path, string& buf)
    {
	char tmp[1024];
	ssize_t ret = ::readlink(path.c_str(), tmp, sizeof(tmp));
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


    string
    prepend_root_prefix(const string& root_prefix, const string& path)
    {
        if (root_prefix == "/")
            return path;
        else
            return root_prefix + path;
    }


    string
    dirname(const string& name)
    {
	string::size_type pos = name.find_last_of('/');
	if (pos == string::npos)
	    return string(".");
	return string(name, 0, pos == 0 ? 1 : pos);
    }


    string
    basename(const string& name)
    {
	string::size_type pos = name.find_last_of('/');
	return string(name, pos + 1);
    }


    unsigned
    pagesize()
    {
	long r = sysconf(_SC_PAGESIZE);
	return r < 0 ? 4096 : r;
    }


    bool
    getMtabData(const string& mount_point, bool& found, MtabData& mtab_data)
    {
	FILE* f = setmntent("/proc/mounts", "r");
	if (!f)
	{
	    y2err("setmntent failed");
	    return false;
	}

	found = false;

	struct mntent m;
	// each mnt table element is limited to PAGE_SIZE top in Linux
	unsigned buf_size = 4 * pagesize();
	boost::scoped_array<char> buf(new char[buf_size]);

	while (getmntent_r(f, &m, buf.get(), buf_size))
	{
	    if (strcmp(m.mnt_type, "rootfs") == 0)
		continue;

	    if (m.mnt_dir == mount_point)
	    {
		found = true;
		mtab_data.device = m.mnt_fsname;
		mtab_data.dir = m.mnt_dir;
		mtab_data.type = m.mnt_type;
		boost::split(mtab_data.options, m.mnt_opts, boost::is_any_of(","),
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
#if (_POSIX_C_SOURCE >= 200112L) && ! _GNU_SOURCE
	char buf1[100];
	if (strerror_r(errnum, buf1, sizeof(buf1) - 1) == 0)
	    return string(buf1);
	return string("strerror failed");
#else
	char buf1[100];
	const char* buf2 = strerror_r(errnum, buf1, sizeof(buf1) - 1);
	return string(buf2);
#endif
    }


    string
    sformat(const char* format, ...)
    {
	char* result;
	string str;

	va_list ap;
	va_start(ap, format);
	if (vasprintf(&result, format, ap) != -1)
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


    long
    sysconf(int name, long fallback)
    {
	long ret = ::sysconf(name);
	return ret == -1 ? fallback : ret;
    }


    bool
    get_uid_username_gid(uid_t uid, string& username, gid_t& gid)
    {
	struct passwd pwd;
	struct passwd* result;

	vector<char> buf(sysconf(_SC_GETPW_R_SIZE_MAX, 1024));

	int e;
	while ((e = getpwuid_r(uid, &pwd, buf.data(), buf.size(), &result)) == ERANGE)
	    buf.resize(2 * buf.size());

	if (e != 0 || result == NULL)
	    return false;

	memset(pwd.pw_passwd, 0, strlen(pwd.pw_passwd));

	username = pwd.pw_name;
	gid = pwd.pw_gid;

	return true;
    }


    bool
    get_uid_dir(uid_t uid, string& dir)
    {
	struct passwd pwd;
	struct passwd* result;

	vector<char> buf(sysconf(_SC_GETPW_R_SIZE_MAX, 1024));

	int e;
	while ((e = getpwuid_r(uid, &pwd, buf.data(), buf.size(), &result)) == ERANGE)
	    buf.resize(2 * buf.size());

	if (e != 0 || result == NULL)
	    return false;

	memset(pwd.pw_passwd, 0, strlen(pwd.pw_passwd));

	dir = pwd.pw_dir;

	return true;
    }


    bool
    get_user_uid(const char* username, uid_t& uid)
    {
	struct passwd pwd;
	struct passwd* result;

	vector<char> buf(sysconf(_SC_GETPW_R_SIZE_MAX, 1024));

	int e;
	while ((e = getpwnam_r(username, &pwd, buf.data(), buf.size(), &result)) == ERANGE)
	    buf.resize(2 * buf.size());

	if (e != 0 || result == NULL)
	{
	    y2war("couldn't find username '" << username << "'");
	    return false;
	}

	memset(pwd.pw_passwd, 0, strlen(pwd.pw_passwd));

	uid = pwd.pw_uid;

	return true;
    }


    bool
    get_group_gid(const char* groupname, gid_t& gid)
    {
	struct group grp;
	struct group* result;

	vector<char> buf(sysconf(_SC_GETGR_R_SIZE_MAX, 1024));

	int e;
	while ((e = getgrnam_r(groupname, &grp, buf.data(), buf.size(), &result)) == ERANGE)
	    buf.resize(2 * buf.size());

	if (e != 0 || result == NULL)
	{
	    y2war("couldn't find groupname '" << groupname << "'");
	    return false;
	}

	memset(grp.gr_passwd, 0, strlen(grp.gr_passwd));

	gid = grp.gr_gid;

	return true;
    }


    vector<gid_t>
    getgrouplist(const char* username, gid_t gid)
    {
	int n = 16;
	vector<gid_t> gids(n);

	while (::getgrouplist(username, gid, &gids[0], &n) == -1)
	    gids.resize(n);

	gids.resize(n);

	sort(gids.begin(), gids.end());

	return gids;
    }


    StopWatch::StopWatch()
	: start_time(chrono::steady_clock::now())
    {
    }


    double
    StopWatch::read() const
    {
	chrono::steady_clock::time_point stop_time = chrono::steady_clock::now();
	chrono::steady_clock::duration duration = stop_time - start_time;
	return chrono::duration<double>(duration).count();
    }


    std::ostream&
    operator<<(std::ostream& s, const StopWatch& sw)
    {
	boost::io::ios_all_saver ias(s);
	return s << fixed << sw.read() << "s";
    }


    bool
    Uuid::operator==(const Uuid& rhs) const
    {
	return std::equal(std::begin(value), std::end(value), std::begin(rhs.value));
    }

}
