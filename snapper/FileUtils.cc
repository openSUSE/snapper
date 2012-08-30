/*
 * Copyright (c) [2011-2012] Novell, Inc.
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


#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stddef.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>

#include "snapper/FileUtils.h"
#include "snapper/AppUtil.h"
#include "snapper/Log.h"
#include "snapper/Exception.h"


namespace snapper
{
    using namespace std;


    SDir::SDir(const string& base_path)
	: base_path(base_path), path()
    {
	dirfd = ::open(base_path.c_str(), O_RDONLY | O_NOATIME);
	if (dirfd < 0)
	{
	    y2err("open failed path:" << base_path << " error:" << stringerror(errno));
	    throw IOErrorException();
	}

	struct stat stat;
	fstat(dirfd, &stat);
	if (!S_ISDIR(stat.st_mode))
	{
	    y2err("not a directory path:" << base_path);
	    throw IOErrorException();
	}
    }


    SDir::SDir(const SDir& dir, const string& name)
	: base_path(dir.base_path), path(dir.path + "/" + name)
    {
	dirfd = ::openat(dir.dirfd, name.c_str(), O_RDONLY | O_NOFOLLOW | O_NOATIME);
	if (dirfd < 0)
	{
	    y2err("open failed path:" << dir.fullname(name) << " (" << stringerror(errno) << ")");
	    throw IOErrorException();
	}

	struct stat stat;
	fstat(dirfd, &stat);
	if (!S_ISDIR(stat.st_mode))
	{
	    y2err("not a directory path:" << dir.fullname(name));
	    throw IOErrorException();
	}
    }


    SDir::SDir(const SDir& dir)
	: base_path(dir.base_path), path(dir.path)
    {
	dirfd = dup(dir.dirfd);
	if (dirfd == -1)
	{
	    y2err("dup failed" << " error:" << stringerror(errno));
	    throw IOErrorException();
	}
    }


    SDir&
    SDir::operator=(const SDir& dir)
    {
	if (this != &dir)
	{
	    ::close(dirfd);
	    dirfd = dup(dir.dirfd);
	    if (dirfd == -1)
	    {
		y2err("dup failed" << " error:" << stringerror(errno));
		throw IOErrorException();
	    }
	}

	return *this;
    }


    SDir::~SDir()
    {
	::close(dirfd);
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
    SDir::entries(std::function<bool(unsigned char, const char* name)> pred) const
    {
	int fd = dup(dirfd);
	if (fd == -1)
	{
	    y2err("dup failed" << " error:" << stringerror(errno));
	    throw IOErrorException();
	}

	DIR* dp = fdopendir(fd);
	if (dp == NULL)
	{
	    y2err("fdopendir failed path:" << fullname() << " error:" << stringerror(errno));
	    ::close(fd);
	    throw IOErrorException();
	}

	vector<string> ret;

	size_t len = offsetof(struct dirent, d_name) + fpathconf(dirfd, _PC_NAME_MAX) + 1;
	struct dirent* ep = (struct dirent*) malloc(len);
	struct dirent* epp;

	while (readdir_r(dp, ep, &epp) == 0 && epp != NULL)
	{
	    if (strcmp(ep->d_name, ".") != 0 && strcmp(ep->d_name, "..") != 0 &&
		pred(ep->d_type, ep->d_name))
		ret.push_back(ep->d_name);
	}

	free(ep);

	closedir(dp);

	sort(ret.begin(), ret.end());

	return ret;
    }


    int
    SDir::stat(const string& name, struct stat* buf, int flags) const
    {
	return ::fstatat(dirfd, name.c_str(), buf, flags);
    }


    int
    SDir::open(const string& name, int flags) const
    {
	return ::openat(dirfd, name.c_str(), flags);
    }


    int
    SDir::open(const string& name, int flags, mode_t mode) const
    {
	return ::openat(dirfd, name.c_str(), flags, mode);
    }


    int
    SDir::readlink(const string& name, string& buf) const
    {
	char tmp[1024];
	int ret = ::readlinkat(dirfd, name.c_str(), tmp, sizeof(tmp));
	if (ret >= 0)
	    buf = string(tmp, ret);
	return ret;
    }


    int
    SDir::mkdir(const string& name, mode_t mode) const
    {
	return ::mkdirat(dirfd, name.c_str(), mode);
    }


    int
    SDir::unlink(const string& name, int flags) const
    {
	return ::unlinkat(dirfd, name.c_str(), flags);
    }


    int
    SDir::chmod(const string& name, mode_t mode, int flags) const
    {
	return ::fchmodat(dirfd, name.c_str(), mode, flags);
    }


    int
    SDir::chown(const string& name, uid_t owner, gid_t group, int flags) const
    {
	return ::fchownat(dirfd, name.c_str(), owner, group, flags);
    }


    int
    SDir::rename(const string& oldname, const string& newname) const
    {
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

	    int fd = open(name, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
	    if (fd >= 0)
		return fd;
	    else if (errno != EEXIST)
		return -1;
	}

	return -1;
    }


    SFile::SFile(const SDir& dir, const string& name)
	: dir(dir), name(name)
    {
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


    int
    SFile::readlink(string& buf) const
    {
	return dir.readlink(name, buf);
    }

}
