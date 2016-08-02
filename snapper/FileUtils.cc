/*
 * Copyright (c) [2011-2014] Novell, Inc.
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


#include "config.h"

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/xattr.h>
#include <fcntl.h>
#include <stddef.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#ifdef ENABLE_SELINUX
#include <selinux/selinux.h>
#endif
#include <algorithm>

#include "snapper/FileUtils.h"
#include "snapper/AppUtil.h"
#include "snapper/Log.h"
#include "snapper/Exception.h"
#ifdef ENABLE_SELINUX
#include "snapper/Selinux.h"
#endif


namespace snapper
{
    using namespace std;


    boost::mutex SDir::cwd_mutex;


    SDir::SDir(const string& base_path)
	: base_path(base_path), path()
    {
	dirfd = ::open(base_path.c_str(), O_RDONLY | O_NOATIME | O_CLOEXEC);
	if (dirfd < 0)
	    SN_THROW(IOErrorException(sformat("open failed path:%s errno:%d (%s)", base_path.c_str(),
					      errno, stringerror(errno).c_str())));

	struct stat buf;
	if (fstat(dirfd, &buf) != 0)
	    SN_THROW(IOErrorException(sformat("fstat failed path:%s errno:%d (%s)", base_path.c_str(),
					      errno, stringerror(errno).c_str())));

	if (!S_ISDIR(buf.st_mode))
	    SN_THROW(IOErrorException("not a directory path:" + base_path));

	setXaStatus();
    }


    SDir::SDir(const SDir& dir, const string& name)
	: base_path(dir.base_path), path(dir.path + "/" + name)
    {
	assert(name.find('/') == string::npos);
	assert(name != "..");

	dirfd = ::openat(dir.dirfd, name.c_str(), O_RDONLY | O_NOFOLLOW | O_NOATIME | O_CLOEXEC);
	if (dirfd < 0)
	    SN_THROW(IOErrorException(sformat("open failed path:%s errno:%d (%s)", dir.fullname().c_str(),
					      errno, stringerror(errno).c_str())));

	struct stat buf;
	if (fstat(dirfd, &buf) != 0)
	    SN_THROW(IOErrorException(sformat("fstat failed path:%s errno:%d (%s)", base_path.c_str(),
					      errno, stringerror(errno).c_str())));

	if (!S_ISDIR(buf.st_mode))
	{
	    close(dirfd);
	    SN_THROW(IOErrorException("not a directory path:" + dir.fullname(name)));
	}

	xastatus = dir.xastatus;
    }


    SDir::SDir(const SDir& dir)
	: base_path(dir.base_path), path(dir.path)
    {
	dirfd = fcntl(dir.dirfd, F_DUPFD_CLOEXEC, 0);
	if (dirfd == -1)
	    SN_THROW(IOErrorException(sformat("fcntl(F_DUPFD_CLOEXEC) failed error:%d (%s)", errno,
					      stringerror(errno).c_str())));

	xastatus = dir.xastatus;
    }


    SDir&
    SDir::operator=(const SDir& dir)
    {
	if (this != &dir)
	{
	    ::close(dirfd);
	    dirfd = fcntl(dir.dirfd, F_DUPFD_CLOEXEC, 0);
	    if (dirfd == -1)
		SN_THROW(IOErrorException(sformat("fcntl(F_DUPFD_CLOEXEC) failed error:%d (%s)", errno,
						  stringerror(errno).c_str())));

	    xastatus = dir.xastatus;
	}

	return *this;
    }


    SDir::~SDir()
    {
	::close(dirfd);
    }


    SDir
    SDir::deepopen(const SDir& dir, const string& name)
    {
	string::size_type pos = name.find('/');
	if (pos == string::npos)
	    return SDir(dir, name);

	return deepopen(SDir(dir, string(name, 0, pos)), string(name, pos + 1));
    }


    string
    SDir::fullname(bool with_base_path) const
    {
	return with_base_path ? base_path + path : path;
    }


    string
    SDir::fullname(const string& name, bool with_base_path) const
    {
	return fullname(with_base_path) + "/" + name;
    }


    static bool
    all_entries(unsigned char type, const char* name)
    {
	return true;
    }


    vector<string>
    SDir::entries() const
    {
	return entries(all_entries);
    }


    vector<string>
    SDir::entries(entries_pred_t pred) const
    {
	int fd = fcntl(dirfd, F_DUPFD_CLOEXEC, 0);
	if (fd == -1)
	    SN_THROW(IOErrorException(sformat("fcntl(F_DUPFD_CLOEXEC) failed error:%d (%s)", errno,
					      stringerror(errno).c_str())));

	DIR* dp = fdopendir(fd);
	if (dp == NULL)
	{
	    ::close(fd);
	    SN_THROW(IOErrorException(sformat("fdopendir failed path:%s error:%d (%s)",
					      fullname().c_str(), errno, stringerror(errno).c_str())));
	}

	vector<string> ret;

	long sz = fpathconf(dirfd, _PC_NAME_MAX);
	if (sz == -1)
	    sz = NAME_MAX;
	size_t len = offsetof(struct dirent, d_name) + sz + 1;
	struct dirent* ep = (struct dirent*) malloc(len);
	struct dirent* epp;

	rewinddir(dp);
	while (readdir_r(dp, ep, &epp) == 0 && epp != NULL)
	{
	    if (strcmp(ep->d_name, ".") != 0 && strcmp(ep->d_name, "..") != 0 &&
		pred(ep->d_type, ep->d_name))
		ret.push_back(ep->d_name);
	}

	free(ep);

	closedir(dp);

	return ret;
    }


    vector<string>
    SDir::entries_recursive() const
    {
	return entries_recursive(all_entries);
    }


    vector<string>
    SDir::entries_recursive(entries_pred_t pred) const
    {
	vector<string> ret;

	vector<string> a = entries(pred);
	for (vector<string>::const_iterator it1 = a.begin(); it1 != a.end(); ++it1)
	{
	    ret.push_back(*it1);

	    struct stat buf;
	    stat(*it1, &buf, AT_SYMLINK_NOFOLLOW);
	    if (S_ISDIR(buf.st_mode))
	    {
		vector<string> b = SDir(*this, *it1).entries_recursive();
		for (vector<string>::const_iterator it2 = b.begin(); it2 != b.end(); ++it2)
		{
		    ret.push_back(*it1 + "/" + *it2);
		}
	    }
	}

	return ret;
    }


    int
    SDir::stat(struct stat* buf) const
    {
	return ::fstat(dirfd, buf);
    }


    int
    SDir::stat(const string& name, struct stat* buf, int flags) const
    {
	assert(name.find('/') == string::npos);
	assert(name != "..");

	return ::fstatat(dirfd, name.c_str(), buf, flags);
    }


    int
    SDir::open(const string& name, int flags) const
    {
	assert(name.find('/') == string::npos);
	assert(name != "..");

	return ::openat(dirfd, name.c_str(), flags);
    }


    int
    SDir::open(const string& name, int flags, mode_t mode) const
    {
	assert(name.find('/') == string::npos);
	assert(name != "..");

	return ::openat(dirfd, name.c_str(), flags, mode);
    }


    ssize_t
    SDir::readlink(const string& name, string& buf) const
    {
	assert(name.find('/') == string::npos);
	assert(name != "..");

	char tmp[1024];
	ssize_t ret = ::readlinkat(dirfd, name.c_str(), tmp, sizeof(tmp));
	if (ret >= 0)
	    buf = string(tmp, ret);
	return ret;
    }


    int
    SDir::mkdir(const string& name, mode_t mode) const
    {
	assert(name.find('/') == string::npos);
	assert(name != "..");

	return ::mkdirat(dirfd, name.c_str(), mode);
    }


    int
    SDir::unlink(const string& name, int flags) const
    {
	assert(name.find('/') == string::npos);
	assert(name != "..");

	return ::unlinkat(dirfd, name.c_str(), flags);
    }


    int
    SDir::chmod(const string& name, mode_t mode, int flags) const
    {
	assert(name.find('/') == string::npos);
	assert(name != "..");

	return ::fchmodat(dirfd, name.c_str(), mode, flags);
    }


    int
    SDir::chown(const string& name, uid_t owner, gid_t group, int flags) const
    {
	assert(name.find('/') == string::npos);
	assert(name != "..");

	return ::fchownat(dirfd, name.c_str(), owner, group, flags);
    }


    int
    SDir::rename(const string& oldname, const string& newname) const
    {
	assert(oldname.find('/') == string::npos);
	assert(oldname != "..");

	assert(newname.find('/') == string::npos);
	assert(newname != "..");

	return ::renameat(dirfd, oldname.c_str(), dirfd, newname.c_str());
    }


    int
    SDir::mktemp(string& name) const
    {
	static const char letters[] = "abcdefghijklmnopqrstuvwxyz" "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	    "0123456789";

	static uint64_t value;

	struct timeval tv;
	gettimeofday(&tv, NULL);
	value += ((uint64_t) tv.tv_usec << 16) ^ tv.tv_sec;

	unsigned int attempts = 62 * 62 * 62;

	string::size_type length = name.size();

	for (unsigned int count = 0; count < attempts; value += 7777, ++count)
	{
	    uint64_t v = value;
	    for (string::size_type i = length - 6; i < length; ++i)
	    {
		name[i] = letters[v % 62];
		v /= 62;
	    }

	    int fd = open(name, O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC, S_IRUSR | S_IWUSR);
	    if (fd >= 0)
		return fd;
	    else if (errno != EEXIST)
		return -1;
	}

	return -1;
    }


    bool
    SDir::mkdtemp(string& name) const
    {
	char* t = strdup((fullname() + "/" + name).c_str());
	if (t == NULL)
	    return false;

	if (::mkdtemp(t) == NULL)
	{
	    free(t);
	    return false;
	}

	name = string(&t[strlen(t) - name.size()]);

	free(t);
	return true;
    }


    bool
    SDir::xaSupported() const
    {
	return xastatus == XA_SUPPORTED;
    }


    ssize_t
    SDir::listxattr(const string& path, char* list, size_t size) const
    {
	assert(path.find('/') == string::npos);
	assert(path != "..");

	int fd = ::openat(dirfd, path.c_str(), O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_NOATIME |
			  O_CLOEXEC);
	if (fd >= 0)
	{
	    ssize_t r1 = ::flistxattr(fd, list, size);
	    ::close(fd);
	    return r1;
	}
	else if (errno == ELOOP || errno == ENXIO || errno == EWOULDBLOCK)
	{
	    boost::lock_guard<boost::mutex> lock(cwd_mutex);

	    int r1 = fchdir(dirfd);
	    if (r1 != 0)
	    {
		y2err("fchdir failed errno:" << errno << " (" << stringerror(errno) << ")");
		return -1;
	    }

	    ssize_t r2 = ::llistxattr(path.c_str(), list, size);
	    chdir("/");
	    return r2;
	}
	else
	{
	    return -1;
	}
    }


    ssize_t
    SDir::getxattr(const string& path, const char* name, void* value, size_t size) const
    {
	assert(path.find('/') == string::npos);
	assert(path != "..");

	int fd = ::openat(dirfd, path.c_str(), O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_NOATIME |
			  O_CLOEXEC);
	if (fd >= 0)
	{
	    ssize_t r1 = ::fgetxattr(fd, name, value, size);
	    ::close(fd);
	    return r1;
	}
	else if (errno == ELOOP || errno == ENXIO || errno == EWOULDBLOCK)
	{
	    boost::lock_guard<boost::mutex> lock(cwd_mutex);

	    int r1 = fchdir(dirfd);
	    if (r1 != 0)
	    {
		y2err("fchdir failed errno:" << errno << " (" << stringerror(errno) << ")");
		return -1;
	    }

	    ssize_t r2 = ::lgetxattr(path.c_str(), name, value, size);
	    chdir("/");
	    return r2;
	}
	else
	{
	    return -1;
	}
    }


    void
    SDir::setXaStatus(void)
    {
	xastatus = XA_UNKNOWN;

#ifdef ENABLE_XATTRS
	ssize_t ret = flistxattr(dirfd, NULL, 0);
	if (ret < 0)
	{
	    if (errno == ENOTSUP)
	    {
		xastatus = XA_UNSUPPORTED;
	    }
	    else
	    {
                SN_THROW(IOErrorException(sformat("Couldn't get extended attributes status for %s/%s, "
						  "errno:%d (%s)", base_path.c_str(), path.c_str(),
						  errno, stringerror(errno).c_str())));
	    }
	}
	else
	{
	    xastatus = XA_SUPPORTED;
	}
#endif
    }


    bool
    SDir::mount(const string& device, const string& mount_type, unsigned long mount_flags,
		const string& mount_data) const
    {
	boost::lock_guard<boost::mutex> lock(cwd_mutex);

	int r1 = fchdir(dirfd);
	if (r1 != 0)
	{
	    y2err("fchdir failed errno:" << errno << " (" << stringerror(errno) << ")");
	    return false;
	}

	int r2 = ::mount(device.c_str(), ".", mount_type.c_str(), mount_flags, mount_data.c_str());
	if (r2 != 0)
	{
	    y2err("mount failed errno:" << errno << " (" << stringerror(errno) << ")");
	    chdir("/");
	    return false;
	}

	chdir("/");
	return true;
    }


    bool
    SDir::umount(const string& mount_point) const
    {
	boost::lock_guard<boost::mutex> lock(cwd_mutex);

	int r1 = fchdir(dirfd);
	if (r1 != 0)
	{
	    y2err("fchdir failed errno:" << errno << " (" << stringerror(errno) << ")");
	    return false;
	}

#ifdef UMOUNT_NOFOLLOW
	int r2 = ::umount2(mount_point.c_str(), UMOUNT_NOFOLLOW);
#else
	int r2 = ::umount2(mount_point.c_str(), 0);
#endif
	if (r2 != 0)
	{
	    y2err("umount failed errno:" << errno << " (" << stringerror(errno) << ")");
	    chdir("/");
	    return false;
	}

	chdir("/");
	return true;
    }


    bool
    SDir::fsetfilecon(const string& name, char* con) const
    {
	assert(name.find('/') == string::npos);
	assert(name != "..");

	bool retval = true;

#ifdef ENABLE_SELINUX
	if (_is_selinux_enabled())
	{
	    char *src_con = NULL;

	    int fd = ::openat(dirfd, name.c_str(), O_RDONLY | O_NOFOLLOW | O_NOATIME
			      | O_NONBLOCK | O_CLOEXEC);
	    if (fd < 0)
	    {
		// symlink, detached dev node?
		if (errno != ELOOP && errno != ENXIO && errno != EWOULDBLOCK)
		{
		    y2err("open failed errno: " << errno << " (" << stringerror(errno) << ")");
		    return false;
		}

		boost::lock_guard<boost::mutex> lock(cwd_mutex);

		if (fchdir(dirfd) < 0)
		{
		    y2err("fchdir failed errno: " << errno << " (" << stringerror(errno) << ")");
		    return false;
		}

		if (lgetfilecon(name.c_str(), &src_con) < 0 || selinux_file_context_cmp(src_con, con))
		{
		    y2deb("setting new SELinux context on " << fullname() << "/" << name);
		    if (lsetfilecon(name.c_str(), con))
		    {
			y2err("lsetfilecon on " << fullname() << "/" << name << " failed errno: " << errno << " (" << stringerror(errno) << ")");
			retval = false;
		    }
		}

		chdir("/");

	    }
	    else
	    {
		if (fgetfilecon(fd, &src_con) < 0 || selinux_file_context_cmp(src_con, con))
		{
		    y2deb("setting new SELinux context on " << fullname() << "/" << name);
		    if (::fsetfilecon(fd, con))
		    {
			y2err("fsetfilecon on " << fullname() << "/" << name << " failed errno: " << errno << " (" << stringerror(errno) << ")");
			retval = false;
		    }
		}

		::close(fd);
	    }

	    freecon(src_con);
	}
#endif
	return retval;
    }


    bool
    SDir::restorecon(const string& name, SelinuxLabelHandle* sh) const
    {
	assert(name.find('/') == string::npos);
	assert(name != "..");

	bool retval = true;
#ifdef ENABLE_SELINUX
	if (_is_selinux_enabled())
	{
	    assert(sh);

	    struct stat buf;
	    if (stat(name, &buf, AT_SYMLINK_NOFOLLOW))
	    {
		y2err("Failed to stat " << fullname() << "/" << name);
		return false;
	    }

	    char* con = sh->selabel_lookup(fullname() + "/" + name, buf.st_mode);
	    if (con)
	    {
		retval = fsetfilecon(name, con);
	    }
	    else
	    {
		retval = false;
	    }

	    freecon(con);
	}
#endif
	return retval;
    }


    bool
    SDir::fsetfilecon(char* con) const
    {
	bool retval = true;

#ifdef ENABLE_SELINUX
	if (_is_selinux_enabled())
	{
	    char* src_con = NULL;

	    if (fgetfilecon(fd(), &src_con) < 0 || selinux_file_context_cmp(src_con, con))
	    {
		y2deb("setting new SELinux context on " << fullname());
		if (::fsetfilecon(fd(), con))
		{
		    y2err("fsetfilecon on " << fullname() << " failed errno: " << errno << " (" << stringerror(errno) << ")");
		    retval = false;
		}
	    }

	    freecon(src_con);
	}
#endif
	return retval;
    }


    bool
    SDir::restorecon(SelinuxLabelHandle* sh) const
    {
	bool retval = true;
#ifdef ENABLE_SELINUX
	if (_is_selinux_enabled())
	{
	    assert(sh);

	    struct stat buf;

	    if (stat(&buf))
	    {
		y2err("Failed to stat " << fullname());
		return false;
	    }

	    char* con = sh->selabel_lookup(fullname(), buf.st_mode);
	    if (con)
	    {
		retval = fsetfilecon(con);
	    }
	    else
	    {
		y2war("can't get proper label for path:" << fullname());
		retval = false;
	    }

	    freecon(con);
	}
#endif
	return retval;
    }


    SFile::SFile(const SDir& dir, const string& name)
	: dir(dir), name(name)
    {
	assert(name.find('/') == string::npos);
	assert(name != "..");
    }


    string
    SFile::fullname(bool with_base_path) const
    {
	return dir.fullname(name, with_base_path);
    }


    int
    SFile::stat(struct stat* buf, int flags) const
    {
	return dir.stat(name, buf, flags);
    }


    int
    SFile::open(int flags) const
    {
	return dir.open(name, flags);
    }


    ssize_t
    SFile::readlink(string& buf) const
    {
	return dir.readlink(name, buf);
    }


    int
    SFile::chmod(mode_t mode, int flags) const
    {
	return dir.chmod(name, mode, flags);
    }


    bool
    SFile::xaSupported() const
    {
	return dir.xaSupported();
    }


    ssize_t
    SFile::listxattr(char* list, size_t size) const
    {
	return dir.listxattr(name, list, size);
    }


    ssize_t
    SFile::getxattr(const char* name, void* value, size_t size) const
    {
	return dir.getxattr(SFile::name, name, value, size);
    }


    void
    SFile::fsetfilecon(char* con) const
    {
	dir.fsetfilecon(name, con);
    }

    void
    SFile::restorecon(SelinuxLabelHandle* sh) const
    {
	dir.restorecon(name, sh);
    }


    TmpDir::TmpDir(SDir& base_dir, const string& name_template)
	: base_dir(base_dir), name(name_template)
    {
	if (!base_dir.mkdtemp(name))
	    throw runtime_error_with_errno("mkdtemp failed", errno);
    }


    TmpDir::~TmpDir()
    {
	if (base_dir.unlink(name, AT_REMOVEDIR) != 0)
	    y2err("unlink failed, errno:" << errno);
    }


    string
    TmpDir::getFullname() const
    {
	return base_dir.fullname() + "/" + name;
    }


    TmpMount::TmpMount(SDir& base_dir, const string& device, const string& name_template,
		       const string& mount_type, unsigned long mount_flags,
		       const string& mount_data)
	: TmpDir(base_dir, name_template)
    {
	SDir subdir(base_dir, name);
	if (!subdir.mount(device, mount_type, mount_flags, mount_data))
	    throw runtime_error_with_errno("mount failed", errno);
    }


    TmpMount::~TmpMount()
    {
	if (!base_dir.umount(name))
	    y2err("umount failed, errno:" << errno);
    }

}
