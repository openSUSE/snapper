/*
 * Copyright (c) [2011-2013] Novell, Inc.
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
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <algorithm>
#include <boost/thread.hpp>

#include "snapper/Log.h"
#include "snapper/AppUtil.h"
#include "snapper/File.h"
#include "snapper/Compare.h"
#include "snapper/Exception.h"


namespace snapper
{
    using namespace std;


    bool
    cmpFilesContentReg(const SFile& file1, const struct stat& stat1, const SFile& file2,
		       const struct stat& stat2)
    {
	if (stat1.st_mtim.tv_sec == stat2.st_mtim.tv_sec && stat1.st_mtim.tv_nsec == stat2.st_mtim.tv_nsec)
	    return true;

	if (stat1.st_size != stat2.st_size)
	    return false;

	if (stat1.st_size == 0 && stat2.st_size == 0)
	    return true;

	if ((stat1.st_dev == stat2.st_dev) && (stat1.st_ino == stat2.st_ino))
	    return true;

	int fd1 = file1.open(O_RDONLY | O_NOFOLLOW | O_NOATIME | O_CLOEXEC);
	if (fd1 < 0)
	{
	    y2err("open failed path:" << file1.fullname() << " errno:" << errno);
	    return false;
	}

	int fd2 = file2.open(O_RDONLY | O_NOFOLLOW | O_NOATIME | O_CLOEXEC);
	if (fd2 < 0)
	{
	    y2err("open failed path:" << file2.fullname() << " errno:" << errno);
	    close(fd1);
	    return false;
	}

	posix_fadvise(fd1, 0, stat1.st_size, POSIX_FADV_SEQUENTIAL);
	posix_fadvise(fd2, 0, stat2.st_size, POSIX_FADV_SEQUENTIAL);

	static_assert(sizeof(off_t) >= 8, "off_t is too small");

	const off_t block_size = 4096;

	char block1[block_size];
	char block2[block_size];

	bool equal = true;

	off_t length = stat1.st_size;
	while (length > 0)
	{
	    off_t t = min(block_size, length);

	    int r1 = read(fd1, block1, t);
	    if (r1 != t)
	    {
		y2err("read failed path:" << file1.fullname() << " errno:" << errno);
		equal = false;
		break;
	    }

	    int r2 = read(fd2, block2, t);
	    if (r2 != t)
	    {
		y2err("read failed path:" << file2.fullname() << " errno:" << errno);
		equal = false;
		break;
	    }

	    if (memcmp(block1, block2, t) != 0)
	    {
		equal = false;
		break;
	    }

	    length -= t;
	}

	close(fd1);
	close(fd2);

	return equal;
    }


    bool
    cmpFilesContentLnk(const SFile& file1, const struct stat& stat1, const SFile& file2,
		       const struct stat& stat2)
    {
	if (stat1.st_mtim.tv_sec == stat2.st_mtim.tv_sec && stat1.st_mtim.tv_nsec == stat2.st_mtim.tv_nsec)
	    return true;

	string tmp1;
	bool r1 = file1.readlink(tmp1);
	if (!r1)
	{
	    y2err("readlink failed path:" << file1.fullname() << " errno:" << errno);
	    return false;
	}

	string tmp2;
	bool r2 = file2.readlink(tmp2);
	if (!r2)
	{
	    y2err("readlink failed path:" << file2.fullname() << " errno:" << errno);
	    return false;
	}

	return tmp1 == tmp2;
    }


    bool
    cmpFilesContent(const SFile& file1, const struct stat& stat1, const SFile& file2,
		    const struct stat& stat2)
    {
	if ((stat1.st_mode & S_IFMT) != (stat2.st_mode & S_IFMT))
	    throw LogicErrorException();

	switch (stat1.st_mode & S_IFMT)
	{
	    case S_IFREG:
		return cmpFilesContentReg(file1, stat1, file2, stat2);

	    case S_IFLNK:
		return cmpFilesContentLnk(file1, stat1, file2, stat2);

	    default:
		return true;
	}
    }


    unsigned int
    cmpFiles(const SFile& file1, const struct stat& stat1, const SFile& file2,
	     const struct stat& stat2)
    {
	unsigned int status = 0;

	if ((stat1.st_mode & S_IFMT) != (stat2.st_mode & S_IFMT))
	{
	    status |= TYPE;
	}
	else
	{
	    if (!cmpFilesContent(file1, stat1, file2, stat2))
		status |= CONTENT;
	}

	if ((stat1.st_mode ^ stat2.st_mode) & (S_IRWXU | S_IRWXG | S_IRWXO | S_ISUID |
					       S_ISGID | S_ISVTX))
	{
	    status |= PERMISSIONS;
	}

	if (stat1.st_uid != stat2.st_uid)
	{
	    status |= USER;
	}

	if (stat1.st_gid != stat2.st_gid)
	{
	    status |= GROUP;
	}

	return status;
    }


    unsigned int
    cmpFiles(const string& base_path1, const string& base_path2, const string& name)
    {
	SDir dir1(base_path1);
	SDir dir2(base_path2);

	SFile file1(dir1, name); // TODO not secure
	SFile file2(dir2, name); // TODO not secure

	struct stat stat1;
	int r1 = file1.stat(&stat1, AT_SYMLINK_NOFOLLOW);

	struct stat stat2;
	int r2 = file2.stat(&stat2, AT_SYMLINK_NOFOLLOW);

	if (r1 != 0 && r2 == 0)
	    return CREATED;

	if (r1 == 0 && r2 != 0)
	    return DELETED;

	if (r1 != 0)
	{
	    y2err("stat failed path:" << file1.fullname());
	    throw IOErrorException();
	}

	if (r2 != 0)
	{
	    y2err("lstat failed path:" << file2.fullname());
	    throw IOErrorException();
	}

	return cmpFiles(file1, stat1, file2, stat2);
    }


    bool
    filter(const string& name)
    {
	if (name == "/.snapshots")
	    return true;

	return false;
    }


    void
    listSubdirs(const SDir& dir, const string& path, unsigned int status, cmpdirs_cb_t cb)
    {
	boost::this_thread::interruption_point();

	vector<string> entries = dir.entries();

	for (vector<string>::const_iterator it = entries.begin(); it != entries.end(); ++it)
	{
	    cb(path + "/" + *it, status);

	    struct stat stat;
	    dir.stat(*it, &stat, AT_SYMLINK_NOFOLLOW);
	    if (S_ISDIR(stat.st_mode))
		listSubdirs(SDir(dir, *it), path + "/" + *it, status, cb);
	}
    }


    struct CmpData
    {
	dev_t dev1;
	dev_t dev2;

	cmpdirs_cb_t cb;
    };


    void
    cmpDirsWorker(const CmpData& cmp_data, const SDir& dir1, const SDir& dir2, const string& path);


    void
    lonesome(const SDir& dir, const string& path, const string& name, const struct stat& stat,
	     unsigned int status, cmpdirs_cb_t cb)
    {
	cb(path + "/" + name, status);

	if (S_ISDIR(stat.st_mode))
	    listSubdirs(SDir(dir, name), path + "/" + name, status, cb);
    }


    void
    twosome(const CmpData& cmp_data, const SDir& dir1, const SDir& dir2, const string& path,
	    const string& name, const struct stat& stat1, const struct stat& stat2)
    {
	unsigned int status = 0;
	if (stat1.st_dev == cmp_data.dev1 && stat2.st_dev == cmp_data.dev2)
	    status = cmpFiles(SFile(dir1, name), stat1, SFile(dir2, name), stat2);

	if (status != 0)
	{
	    cmp_data.cb(path + "/" + name, status);
	}

	if (!(status & TYPE))
	{
	    if (S_ISDIR(stat1.st_mode))
		if (stat1.st_dev == cmp_data.dev1 && stat2.st_dev == cmp_data.dev2)
		    cmpDirsWorker(cmp_data, SDir(dir1, name), SDir(dir2, name), path + "/" + name);
	}
	else
	{
	    if (S_ISDIR(stat1.st_mode))
		if (stat1.st_dev == cmp_data.dev1)
		    listSubdirs(SDir(dir1, name), path + "/" + name, DELETED, cmp_data.cb);

	    if (S_ISDIR(stat2.st_mode))
		if (stat2.st_dev == cmp_data.dev2)
		    listSubdirs(SDir(dir2, name), path + "/" + name, CREATED, cmp_data.cb);
	}
    }


    void
    cmpDirsWorker(const CmpData& cmp_data, const SDir& dir1, const SDir& dir2, const string& path)
    {
	boost::this_thread::interruption_point();

	const vector<string> entries1 = dir1.entries();
	vector<string>::const_iterator first1 = entries1.begin();
	vector<string>::const_iterator last1 = entries1.end();

	const vector<string> entries2 = dir2.entries();
	vector<string>::const_iterator first2 = entries2.begin();
	vector<string>::const_iterator last2 = entries2.end();

	while (first1 != last1 || first2 != last2)
	{
	    if (first1 != last1 && filter(path + "/" + *first1))
	    {
		++first1;
	    }
	    else if (first2 != last2 && filter(path + "/" + *first2))
	    {
		++first2;
	    }
	    else if (first1 == last1)
	    {
		struct stat stat2;
		dir2.stat(*first2, &stat2, AT_SYMLINK_NOFOLLOW); // TODO error check

		if (stat2.st_dev == cmp_data.dev2)
		    lonesome(dir2, path, *first2, stat2, CREATED, cmp_data.cb);

		++first2;
	    }
	    else if (first2 == last2)
	    {
		struct stat stat1;
		dir1.stat(*first1, &stat1, AT_SYMLINK_NOFOLLOW); // TODO error check

		if (stat1.st_dev == cmp_data.dev1)
		    lonesome(dir1, path, *first1, stat1, DELETED, cmp_data.cb);

		++first1;
	    }
	    else if (*first2 < *first1)
	    {
		struct stat stat2;
		dir2.stat(*first2, &stat2, AT_SYMLINK_NOFOLLOW); // TODO error check

		if (stat2.st_dev == cmp_data.dev2)
		    lonesome(dir2, path, *first2, stat2, CREATED, cmp_data.cb);

		++first2;
	    }
	    else if (*first1 < *first2)
	    {
		struct stat stat1;
		dir1.stat(*first1, &stat1, AT_SYMLINK_NOFOLLOW); // TODO error check

		if (stat1.st_dev == cmp_data.dev1)
		    lonesome(dir1, path, *first1, stat1, DELETED, cmp_data.cb);

		++first1;
	    }
	    else
	    {
		if (*first1 != *first2)
		    throw LogicErrorException();

		struct stat stat1;
		dir1.stat(*first1, &stat1, AT_SYMLINK_NOFOLLOW); // TODO error check

		struct stat stat2;
		dir2.stat(*first2, &stat2, AT_SYMLINK_NOFOLLOW); // TODO error check

		twosome(cmp_data, dir1, dir2, path, *first1, stat1, stat2);
		++first1;
		++first2;
	    }
	}
    }


    void
    cmpDirs(const SDir& dir1, const SDir& dir2, cmpdirs_cb_t cb)
    {
	y2mil("path1:" << dir1.fullname() << " path2:" << dir2.fullname());

	struct stat stat1;
	int r1 = dir1.stat(&stat1);
	if (r1 != 0)
	{
	    y2err("stat failed path:" << dir1.fullname() << " errno:" << errno);
	    throw IOErrorException();
	}

	struct stat stat2;
	int r2 = dir2.stat(&stat2);
	if (r2 != 0)
	{
	    y2err("stat failed path:" << dir2.fullname() << " errno:" << errno);
	    throw IOErrorException();
	}

	CmpData cmp_data;
	cmp_data.cb = cb;
	cmp_data.dev1 = stat1.st_dev;
	cmp_data.dev2 = stat2.st_dev;

	y2mil("dev1:" << cmp_data.dev1 << " dev2:" << cmp_data.dev2);

	StopWatch stopwatch;
	cmpDirsWorker(cmp_data, dir1, dir2, "");
	y2mil("stopwatch " << stopwatch << " for comparing directories");
    }

}
