/*
 * Copyright (c) 2011 Novell, Inc.
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
#include <assert.h>
#include <vector>
#include <algorithm>

#include "snapper/AppUtil.h"
#include "snapper/Files.h"
#include "snapper/SnapperInterface.h"


namespace snapper
{
    using namespace std;


    bool
    cmpFilesContentReg(const string& fullname1, struct stat stat1, const string& fullname2,
		       struct stat stat2)
    {
	if (stat1.st_mtime == stat2.st_mtime)
	    return true;

	if (stat1.st_size != stat2.st_size)
	    return false;

	if (stat1.st_size == 0 && stat2.st_size == 0)
	    return true;

	if ((stat1.st_dev == stat2.st_dev) && (stat1.st_ino == stat2.st_ino))
	    return true;

	int fd1 = open(fullname1.c_str(), O_RDONLY | O_NOATIME);
	assert(fd1 >= 0);
	posix_fadvise(fd1, 0, stat1.st_size, POSIX_FADV_SEQUENTIAL);

	int fd2 = open(fullname2.c_str(), O_RDONLY | O_NOATIME);
	assert(fd2 >= 0);
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
	    assert(r1 == t);

	    int r2 = read(fd2, block2, t);
	    assert(r2 == t);

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
    cmpFilesContentLnk(const string& fullname1, struct stat stat1, const string& fullname2,
		       struct stat stat2)
    {
	if (stat1.st_mtime == stat2.st_mtime)
	    return true;

	string tmp1;
	bool r1 = readlink(fullname1, tmp1);
	assert(r1);

	string tmp2;
	bool r2 = readlink(fullname2, tmp2);
	assert(r2);

	return tmp1 == tmp2;
    }


    bool
    cmpFilesContent(const string& fullname1, struct stat stat1, const string& fullname2,
		    struct stat stat2)
    {
	assert((stat1.st_mode & S_IFMT) == (stat2.st_mode & S_IFMT));

	switch (stat1.st_mode & S_IFMT)
	{
	    case S_IFREG:
		return cmpFilesContentReg(fullname1, stat1, fullname2, stat2);

	    case S_IFLNK:
		return cmpFilesContentLnk(fullname1, stat1, fullname2, stat2);

	    default:
		return true;
	}
    }


    unsigned int
    cmpFiles(const string& fullname1, struct stat stat1, const string& fullname2, struct stat stat2)
    {
	unsigned int status = 0;

	if ((stat1.st_mode & S_IFMT) != (stat2.st_mode & S_IFMT))
	{
	    status |= TYPE;
	}
	else
	{
	    if (!cmpFilesContent(fullname1, stat1, fullname2, stat2))
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
    cmpFiles(const string& fullname1, const string& fullname2)
    {
	struct stat stat1;
	int r1 = stat(fullname1.c_str(), &stat1);

	struct stat stat2;
	int r2 = stat(fullname2.c_str(), &stat2);

	if (r1 != 0 && r2 == 0)
	    return CREATED;

	if (r1 == 0 && r2 != 0)
	    return DELETED;

	return cmpFiles(fullname1, stat1, fullname2, stat2);
    }


    bool
    filter(const string& name)
    {
	if (name == "/snapshots")
	    return true;

	return false;
    }


    vector<string>
    readDirectory(const string& base_path, const string& path)
    {
	vector<string> ret;

	int fd = open((base_path + path).c_str(), O_RDONLY | O_NOATIME);
	assert(fd >= 0);

	DIR* dp = fdopendir(fd);
	assert(dp != NULL);

	struct dirent* ep;
	while ((ep = readdir(dp)))
	{
	    if (strcmp(ep->d_name, ".") == 0 || strcmp(ep->d_name, "..") == 0)
		continue;

	    string fullname = path + "/" + ep->d_name;
	    if (filter(fullname))
		continue;

	    ret.push_back(ep->d_name);
	}

	closedir(dp);

	sort(ret.begin(), ret.end());

	return ret;
    }


    void
    listSubdirs(const string& base_path, const string& path, unsigned int status,
		void(*cb)(const string& name, unsigned int status))
    {
	vector<string> tmp = readDirectory(base_path, path);

	for (vector<string>::const_iterator it1 = tmp.begin(); it1 != tmp.end(); ++it1)
	{
	    (*cb)(path + "/" + *it1, status);

	    struct stat stat;
	    int r = lstat((base_path + path + "/" + *it1).c_str(), &stat);
	    assert(r == 0);

	    if (S_ISDIR(stat.st_mode))
		listSubdirs(base_path, path + "/" + *it1, status, cb);
	}
    }


    struct CmpData
    {
	string base_path1;
	string base_path2;

	dev_t dev1;
	dev_t dev2;

	void(*cb)(const string& name, unsigned int status);
    };


    void
    cmpDirsWorker(const CmpData& cmp_data, const string& path);


    void
    lonesome(const string& base_path, const string& path, const string& name, unsigned int status,
	     void(*cb)(const string& name, unsigned int status))
    {
	(*cb)(path + "/" + name, status);

	struct stat stat;
	int r = lstat((base_path + path + "/" + name).c_str(), &stat);
	assert(r == 0);

	if (S_ISDIR(stat.st_mode))
	    listSubdirs(base_path, path + "/" + name, status, cb);
    }


    void
    twosome(const CmpData& cmp_data, const string& path, const string& name)
    {
	string fullname1 = cmp_data.base_path1 + path + "/" + name;
	struct stat stat1;
	int r1 = lstat(fullname1.c_str(), &stat1);
	assert(r1 == 0);

	string fullname2 = cmp_data.base_path2 + path + "/" + name;
	struct stat stat2;
	int r2 = lstat(fullname2.c_str(), &stat2);
	assert(r2 == 0);

	unsigned int status = 0;
	if (stat1.st_dev == cmp_data.dev1 && stat2.st_dev == cmp_data.dev2)
	    status = cmpFiles(fullname1, stat1, fullname2, stat2);

	if (status != 0)
	{
	    (*cmp_data.cb)(path + "/" + name, status);
	}

	if (!(status & TYPE))
	{
	    if (S_ISDIR(stat1.st_mode))
		if (stat1.st_dev == cmp_data.dev1 && stat2.st_dev == cmp_data.dev2)
		    cmpDirsWorker(cmp_data, path + "/" + name);
	}
	else
	{
	    if (S_ISDIR(stat1.st_mode))
		if (stat1.st_dev == cmp_data.dev1)
		    listSubdirs(cmp_data.base_path1, path + "/" + name, DELETED, cmp_data.cb);

	    if (S_ISDIR(stat2.st_mode))
		if (stat2.st_dev == cmp_data.dev2)
		    listSubdirs(cmp_data.base_path2, path + "/" + name, CREATED, cmp_data.cb);
	}
    }


    void
    cmpDirsWorker(const CmpData& cmp_data, const string& path)
    {
	const vector<string> pre = readDirectory(cmp_data.base_path1, path);
	vector<string>::const_iterator first1 = pre.begin();
	vector<string>::const_iterator last1 = pre.end();

	const vector<string> post = readDirectory(cmp_data.base_path2, path);
	vector<string>::const_iterator first2 = post.begin();
	vector<string>::const_iterator last2 = post.end();

	while (first1 != last1 || first2 != last2)
	{
	    if (first1 == last1)
	    {
		lonesome(cmp_data.base_path2, path, *first2, CREATED, cmp_data.cb);
		++first2;
	    }
	    else if (first2 == last2)
	    {
		lonesome(cmp_data.base_path1, path, *first1, DELETED, cmp_data.cb);
		++first1;
	    }
	    else if (*first2 < *first1)
	    {
		lonesome(cmp_data.base_path2, path, *first2, CREATED, cmp_data.cb);
		++first2;
	    }
	    else if (*first1 < *first2)
	    {
		lonesome(cmp_data.base_path1, path, *first1, DELETED, cmp_data.cb);
		++first1;
	    }
	    else
	    {
		assert(*first1 == *first2);
		twosome(cmp_data, path, *first1);
		++first1;
		++first2;
	    }
	}
    }


    void
    cmpDirs(const string& path1, const string& path2, void(*cb)(const string& name,
								unsigned int status))
    {
	y2mil("path1:" << path1 << " path2:" << path2);

	CmpData cmp_data;
	cmp_data.base_path1 = path1;
	cmp_data.base_path2 = path2;
	cmp_data.cb = cb;

	struct stat stat1;
	int r1 = stat(path1.c_str(), &stat1);
	assert(r1 == 0);
	cmp_data.dev1 = stat1.st_dev;

	struct stat stat2;
	int r2 = stat(path2.c_str(), &stat2);
	assert(r2 == 0);
	cmp_data.dev2 = stat2.st_dev;

	y2mil("dev1:" << cmp_data.dev1 << " dev1:" << cmp_data.dev2);

	cmpDirsWorker(cmp_data, "");
    }


    string
    statusToString(unsigned int status)
    {
	string ret;

	if (status & CREATED)
	    ret += "+";
	else if (status & DELETED)
	    ret += "-";
	else if (status & TYPE)
	    ret += "t";
	else if (status & CONTENT)
	    ret += "c";
	else
	    ret += ".";

	ret += status & PERMISSIONS ? "p" : ".";
	ret += status & USER ? "u" : ".";
	ret += status & GROUP ? "g" : ".";

	return ret;
    }

}
