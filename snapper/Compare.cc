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
#include <vector>
#include <algorithm>

#include "snapper/AppUtil.h"
#include "snapper/File.h"
#include "snapper/Compare.h"
#include "snapper/Exception.h"


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
	if (fd1 < 0)
	{
	    y2err("open failed path:" << fullname1 << " errno:" << errno);
	    return false;
	}

	int fd2 = open(fullname2.c_str(), O_RDONLY | O_NOATIME);
	if (fd2 < 0)
	{
	    y2err("open failed path:" << fullname2 << " errno:" << errno);
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
		y2err("read failed path:" << fullname1 << " errno:" << errno);
		equal = false;
		break;
	    }

	    int r2 = read(fd2, block2, t);
	    if (r2 != t)
	    {
		y2err("read failed path:" << fullname2 << " errno:" << errno);
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
    cmpFilesContentLnk(const string& fullname1, struct stat stat1, const string& fullname2,
		       struct stat stat2)
    {
	if (stat1.st_mtime == stat2.st_mtime)
	    return true;

	string tmp1;
	bool r1 = readlink(fullname1, tmp1);
	if (!r1)
	{
	    y2err("readlink failed path:" << fullname1 << " errno:" << errno);
	    return false;
	}

	string tmp2;
	bool r2 = readlink(fullname2, tmp2);
	if (!r2)
	{
	    y2err("readlink failed path:" << fullname2 << " errno:" << errno);
	    return false;
	}

	return tmp1 == tmp2;
    }


    bool
    cmpFilesContent(const string& fullname1, struct stat stat1, const string& fullname2,
		    struct stat stat2)
    {
	if ((stat1.st_mode & S_IFMT) != (stat2.st_mode & S_IFMT))
	    throw LogicErrorException();

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
	int r1 = lstat(fullname1.c_str(), &stat1);

	struct stat stat2;
	int r2 = lstat(fullname2.c_str(), &stat2);

	if (r1 != 0 && r2 == 0)
	    return CREATED;

	if (r1 == 0 && r2 != 0)
	    return DELETED;

	if (r1 != 0)
	{
	    y2err("lstats failed path:" << fullname1);
	    throw IOErrorException();
	}

	if (r2 != 0)
	{
	    y2err("lstats failed path:" << fullname2);
	    throw IOErrorException();
	}

	return cmpFiles(fullname1, stat1, fullname2, stat2);
    }


    bool
    filter(const string& name)
    {
	if (name == "/snapshots")
	    return true;

	return false;
    }


    struct NameStat
    {
	NameStat(const string& name, const struct stat& stat)
	    : name(name), stat(stat) {}

	string name;
	struct stat stat;

	friend bool operator<(const NameStat& lhs, const NameStat& rhs)
	    { return lhs.name < rhs.name; }
    };


    vector<NameStat>
    readDirectory(const string& base_path, const string& path)
    {
	vector<NameStat> ret;

	int fd = open((base_path + path).c_str(), O_RDONLY | O_NOATIME);
	if (fd < 0)
	{
	    y2err("open failed path:" << base_path + path << " error:" << errno);
	    return ret;
	}

	DIR* dp = fdopendir(fd);
	if (dp == NULL)
	{
	    y2err("fdopendir failed path:" << base_path + path << " error:" << errno);
	    close(fd);
	    return ret;
	}

	struct dirent* ep;
	while ((ep = readdir(dp)))
	{
	    if (strcmp(ep->d_name, ".") == 0 || strcmp(ep->d_name, "..") == 0)
		continue;

	    string fullname = path + "/" + ep->d_name;
	    if (filter(fullname))
		continue;

	    struct stat stat;
	    int r = fstatat(fd, ep->d_name, &stat, AT_SYMLINK_NOFOLLOW);
	    if (r != 0)
	    {
		y2err("fstatat failed path:" << ep->d_name << " errno:" << errno);
		continue;
	    }

	    ret.push_back(NameStat(ep->d_name, stat));
	}

	closedir(dp);

	sort(ret.begin(), ret.end());

	return ret;
    }


    void
    listSubdirs(const string& base_path, const string& path, unsigned int status, cmpdirs_cb_t cb)
    {
	vector<NameStat> tmp = readDirectory(base_path, path);

	for (vector<NameStat>::const_iterator it1 = tmp.begin(); it1 != tmp.end(); ++it1)
	{
	    cb(path + "/" + it1->name, status);

	    if (S_ISDIR(it1->stat.st_mode))
		listSubdirs(base_path, path + "/" + it1->name, status, cb);
	}
    }


    struct CmpData
    {
	string base_path1;
	string base_path2;

	dev_t dev1;
	dev_t dev2;

	cmpdirs_cb_t cb;
    };


    void
    cmpDirsWorker(const CmpData& cmp_data, const string& path);


    void
    lonesome(const string& base_path, const string& path, const string& name, const struct stat& stat,
	     unsigned int status, cmpdirs_cb_t cb)
    {
	cb(path + "/" + name, status);

	if (S_ISDIR(stat.st_mode))
	    listSubdirs(base_path, path + "/" + name, status, cb);
    }


    void
    twosome(const CmpData& cmp_data, const string& path, const string& name, const struct stat& stat1,
	    const struct stat& stat2)
    {
	string fullname1 = cmp_data.base_path1 + path + "/" + name;
	string fullname2 = cmp_data.base_path2 + path + "/" + name;

	unsigned int status = 0;
	if (stat1.st_dev == cmp_data.dev1 && stat2.st_dev == cmp_data.dev2)
	    status = cmpFiles(fullname1, stat1, fullname2, stat2);

	if (status != 0)
	{
	    cmp_data.cb(path + "/" + name, status);
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
	const vector<NameStat> pre = readDirectory(cmp_data.base_path1, path);
	vector<NameStat>::const_iterator first1 = pre.begin();
	vector<NameStat>::const_iterator last1 = pre.end();

	const vector<NameStat> post = readDirectory(cmp_data.base_path2, path);
	vector<NameStat>::const_iterator first2 = post.begin();
	vector<NameStat>::const_iterator last2 = post.end();

	while (first1 != last1 || first2 != last2)
	{
	    if (first1 == last1)
	    {
		if (first2->stat.st_dev == cmp_data.dev2)
		    lonesome(cmp_data.base_path2, path, first2->name, first2->stat, CREATED, cmp_data.cb);

		++first2;
	    }
	    else if (first2 == last2)
	    {
		if (first1->stat.st_dev == cmp_data.dev1)
		    lonesome(cmp_data.base_path1, path, first1->name, first1->stat, DELETED, cmp_data.cb);

		++first1;
	    }
	    else if (first2->name < first1->name)
	    {
		if (first2->stat.st_dev == cmp_data.dev2)
		    lonesome(cmp_data.base_path2, path, first2->name, first2->stat, CREATED, cmp_data.cb);

		++first2;
	    }
	    else if (first1->name < first2->name)
	    {
		if (first1->stat.st_dev == cmp_data.dev1)
		    lonesome(cmp_data.base_path1, path, first1->name, first1->stat, DELETED, cmp_data.cb);

		++first1;
	    }
	    else
	    {
		if (first1->name != first2->name)
		    throw LogicErrorException();

		twosome(cmp_data, path, first1->name, first1->stat, first2->stat);
		++first1;
		++first2;
	    }
	}
    }


    void
    cmpDirs(const string& path1, const string& path2, cmpdirs_cb_t cb)
    {
	y2mil("path1:" << path1 << " path2:" << path2);

	CmpData cmp_data;

	cmp_data.base_path1 = path1;
	cmp_data.base_path2 = path2;

	cmp_data.cb = cb;

	struct stat stat1;
	int r1 = lstat(path1.c_str(), &stat1);

	struct stat stat2;
	int r2 = lstat(path2.c_str(), &stat2);

	if (r1 != 0)
	{
	    y2err("lstats failed path:" << path1);
	    throw IOErrorException();
	}

	if (r2 != 0)
	{
	    y2err("lstats failed path:" << path2);
	    throw IOErrorException();
	}

	cmp_data.dev1 = stat1.st_dev;
	cmp_data.dev2 = stat2.st_dev;

	y2mil("dev1:" << cmp_data.dev1 << " dev2:" << cmp_data.dev2);

	StopWatch stopwatch;
	cmpDirsWorker(cmp_data, "");
	y2mil("stopwatch " << stopwatch << " for comparing directories");
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


    unsigned int
    stringToStatus(const string& str)
    {
	unsigned int ret = 0;

	if (str.length() >= 1)
	{
	    switch (str[0])
	    {
		case '+': ret |= CREATED; break;
		case '-': ret |= DELETED; break;
		case 't': ret |= TYPE; break;
		case 'c': ret |= CONTENT; break;
	    }
	}

	if (str.length() >= 2)
	{
	    if (str[1] == 'p')
		ret |= PERMISSIONS;
	}

	if (str.length() >= 3)
	{
	    if (str[2] == 'u')
		ret |= USER;
	}

	if (str.length() >= 4)
	{
	    if (str[3] == 'g')
		ret |= GROUP;
	}

	return ret;
    }


    unsigned int
    invertStatus(unsigned int status)
    {
	if (status & (CREATED | DELETED))
	    return status ^ (CREATED | DELETED);
	return status;
    }

}
