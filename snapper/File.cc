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


#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <fnmatch.h>
#include <errno.h>
#include <boost/algorithm/string.hpp>

#include "snapper/File.h"
#include "snapper/Snapper.h"
#include "snapper/Comparison.h"
#include "snapper/AppUtil.h"
#include "snapper/XmlFile.h"
#include "snapper/Enum.h"
#include "snapper/SnapperTmpl.h"
#include "snapper/SystemCmd.h"
#include "snapper/SnapperDefines.h"
#include "snapper/Compare.h"
#include "snapper/AsciiFile.h"
#include "snapper/Exception.h"


namespace snapper
{

    std::ostream& operator<<(std::ostream& s, const RollbackStatistic& rs)
    {
	s << "numCreate:" << rs.numCreate
	  << " numModify:" << rs.numModify
	  << " numDelete:" << rs.numDelete;

	return s;
    }


    RollbackStatistic::RollbackStatistic()
	: numCreate(0), numModify(0), numDelete(0)
    {
    }


    bool
    RollbackStatistic::empty() const
    {
	return numCreate == 0 && numModify == 0 && numDelete == 0;
    }


    std::ostream& operator<<(std::ostream& s, const File& file)
    {
	s << "name:\"" << file.name << "\"";

	s << " pre_to_post_status:\"" << statusToString(file.pre_to_post_status) << "\"";

	if (file.pre_to_system_status != (unsigned int)(-1))
	    s << " pre_to_post_status:\"" << statusToString(file.pre_to_system_status) << "\"";

	if (file.post_to_system_status != (unsigned int)(-1))
	    s << " post_to_post_status:\"" << statusToString(file.post_to_system_status) << "\n";

	return s;
    }


    const Snapper*
    File::getSnapper() const
    {
	return comparison->getSnapper();
    }


    const Snapper*
    Files::getSnapper() const
    {
	return comparison->getSnapper();
    }


#if 1
    struct AppendHelper
    {
	AppendHelper(const Comparison* comparison, vector<File>& entries)
	    : comparison(comparison), entries(entries) {}
	void operator()(const string& name, unsigned int status)
	    { entries.push_back(File(comparison, name, status)); }
	const Comparison* comparison;
	vector<File>& entries;
    };
#endif


    void
    Files::create()
    {
	y2mil("num1:" << comparison->getSnapshot1()->getNum() << " num2:" <<
	      comparison->getSnapshot2()->getNum());

	if (getSnapper()->getCompareCallback())
	    getSnapper()->getCompareCallback()->start();

	comparison->getSnapshot1()->mountFilesystemSnapshot();
	comparison->getSnapshot2()->mountFilesystemSnapshot();

#if 1
	cmpdirs_cb_t cb = AppendHelper(comparison, entries);
#else
	cmpdirs_cb_t cb = [&comparison, &entries](const string& name, unsigned int status) {
	    entries.push_back(File(comparison, name, status));
	};
#endif
	cmpDirs(comparison->getSnapshot1()->snapshotDir(), comparison->getSnapshot2()->snapshotDir(), cb);

	sort(entries.begin(), entries.end());

	if (getSnapper()->getCompareCallback())
	    getSnapper()->getCompareCallback()->stop();

	y2mil("found " << entries.size() << " lines");
    }


    bool
    Files::load()
    {
	y2mil("num1:" << comparison->getSnapshot1()->getNum() << " num2:" <<
	      comparison->getSnapshot2()->getNum());

	if (comparison->getSnapshot1()->isCurrent() || comparison->getSnapshot2()->isCurrent())
	    throw IllegalSnapshotException();

	unsigned int num1 = comparison->getSnapshot1()->getNum();
	unsigned int num2 = comparison->getSnapshot2()->getNum();

	bool invert = num1 > num2;

	if (invert)
	    swap(num1, num2);

	string input = getSnapper()->infosDir() + "/" + decString(num2) + "/filelist-" +
	    decString(num1) + ".txt";

	try
	{
	    AsciiFileReader asciifile(input);

	    string line;
	    while (asciifile.getline(line))
	    {
		string::size_type pos = line.find(" ");
		if (pos == string::npos)
		    continue;

		unsigned int status = stringToStatus(string(line, 0, pos));
		string name = string(line, pos + 1);

		if (invert)
		    status = invertStatus(status);

		File file(comparison, name, status);
		entries.push_back(file);
	    }

	    sort(entries.begin(), entries.end());

	    y2mil("read " << entries.size() << " lines");

	    return true;
	}
	catch (const FileNotFoundException& e)
	{
	    return false;
	}

	return true;
    }


    bool
    Files::save()
    {
	y2mil("num1:" << comparison->getSnapshot1()->getNum() << " num2:" << comparison->getSnapshot2()->getNum());

	if (comparison->getSnapshot1()->isCurrent() || comparison->getSnapshot2()->isCurrent())
	    throw IllegalSnapshotException();

	unsigned int num1 = comparison->getSnapshot1()->getNum();
	unsigned int num2 = comparison->getSnapshot2()->getNum();

	bool invert = num1 > num2;

	if (invert)
	    swap(num1, num2);

	string output = getSnapper()->infosDir() + "/" + decString(num2) + "/filelist-" +
	    decString(num1) + ".txt";

	string tmp_name = output + ".tmp-XXXXXX";

	FILE* file = mkstemp(tmp_name);
	if (!file)
	{
	    y2err("mkstemp failed errno:" << errno << " (" << strerror(errno) << ")");
	    throw IOErrorException();
	}

	for (const_iterator it = entries.begin(); it != entries.end(); ++it)
	{
	    unsigned int status = it->getPreToPostStatus();

	    if (invert)
		status = invertStatus(status);

	    fprintf(file, "%s %s\n", statusToString(status).c_str(), it->getName().c_str());
	}

	fclose(file);

	rename(tmp_name.c_str(), output.c_str());

	return true;
    }


    struct FilterHelper
    {
	FilterHelper(const vector<string>& patterns)
	    : patterns(patterns) {}
	bool operator()(const File& file)
	    {
		for (vector<string>::const_iterator it = patterns.begin(); it != patterns.end(); ++it)
		    if (fnmatch(it->c_str(), file.getName().c_str(), 0) == 0)
			return true;
		return false;
	    }
	const vector<string>& patterns;
    };


    void
    Files::filter()
    {
	const vector<string>& ignore_patterns = getSnapper()->getIgnorePatterns();
	entries.erase(remove_if(entries.begin(), entries.end(), FilterHelper(ignore_patterns)),
		      entries.end());
    }


    void
    Files::initialize()
    {
	entries.clear();

	if (comparison->getSnapshot1()->isCurrent() || comparison->getSnapshot2()->isCurrent())
	{
	    create();
	}
	else
	{
	    if (!load())
	    {
		create();
		save();
	    }
	}

	filter();
    }


    inline bool
    file_name_less(const File& file, const string& name)
    {
	return file.getName() < name;
    }


    Files::iterator
    Files::find(const string& name)
    {
	iterator ret = lower_bound(entries.begin(), entries.end(), name, file_name_less);
	return ret->getName() == name ? ret : end();
    }


    Files::const_iterator
    Files::find(const string& name) const
    {
	const_iterator ret = lower_bound(entries.begin(), entries.end(), name, file_name_less);
	return ret->getName() == name ? ret : end();
    }


    Files::iterator
    Files::findAbsolutePath(const string& filename)
    {
	if (!boost::starts_with(filename, getSnapper()->subvolumeDir()))
	    return end();

	return find(string(filename, getSnapper()->subvolumeDir().size()));
    }


    Files::const_iterator
    Files::findAbsolutePath(const string& filename) const
    {
	if (!boost::starts_with(filename, getSnapper()->subvolumeDir()))
	    return end();

	return find(string(filename, getSnapper()->subvolumeDir().size()));
    }


    unsigned int
    File::getPreToSystemStatus()
    {
	if (pre_to_system_status == (unsigned int)(-1))
	    pre_to_system_status = cmpFiles(getAbsolutePath(LOC_PRE),
					    getAbsolutePath(LOC_SYSTEM));
	return pre_to_system_status;
    }


    unsigned int
    File::getPostToSystemStatus()
    {
	if (post_to_system_status == (unsigned int)(-1))
	    post_to_system_status = cmpFiles(getAbsolutePath(LOC_POST),
					     getAbsolutePath(LOC_SYSTEM));
	return post_to_system_status;
    }


    unsigned int
    File::getStatus(Cmp cmp)
    {
	switch (cmp)
	{
	    case CMP_PRE_TO_POST:
		return getPreToPostStatus();

	    case CMP_PRE_TO_SYSTEM:
		return getPreToSystemStatus();

	    case CMP_POST_TO_SYSTEM:
		return getPostToSystemStatus();
	}

	return -1;
    }


    string
    File::getAbsolutePath(Location loc) const
    {
	string prefix;

	switch (loc)
	{
	    case LOC_PRE:
		prefix = comparison->getSnapshot1()->snapshotDir();
		break;

	    case LOC_POST:
		prefix = comparison->getSnapshot2()->snapshotDir();
		break;

	    case LOC_SYSTEM:
		prefix = getSnapper()->subvolumeDir();
		break;
	}

	return prefix == "/" ? name : prefix + name;
    }


    vector<string>
    File::getDiff(const string& options) const
    {
	SystemCmd cmd(DIFFBIN " " + options + " " + quote(getAbsolutePath(LOC_PRE)) + " " +
		      quote(getAbsolutePath(LOC_POST)));
	return cmd.stdout();
    }


    bool
    File::createParentDirectories(const string& path) const
    {
	string::size_type pos = path.rfind('/');
	if (pos == string::npos)
	    return true;

	const string& leading_path = path.substr(0, pos);

	struct stat fs;
	if (stat(leading_path.c_str(), &fs) == 0)
	{
	    if (!S_ISDIR(fs.st_mode))
	    {
		y2err("not a directory path:" << leading_path);
		return false;
	    }

	    return true;
	}

	if (!createParentDirectories(leading_path))
	    return false;

	if (mkdir(leading_path.c_str(), 0777) != 0)
	{
	    y2err("mkdir failed path:" << leading_path << " errno:" << errno);
	    return false;
	}

	return true;
    }


    bool
    File::createAllTypes() const
    {
	struct stat fs;
	if (lstat(getAbsolutePath(LOC_PRE).c_str(), &fs) != 0)
	{
	    y2err("lstat failed path:" << getAbsolutePath(LOC_SYSTEM) << " errno:" << errno);
	    return false;
	}
	else if (!createParentDirectories(getAbsolutePath(LOC_SYSTEM)))
	{
	    return false;
	}
	else
	{
	    switch (fs.st_mode & S_IFMT)
	    {
		case S_IFDIR: {
		    if (!createDirectory(fs.st_mode, fs.st_uid, fs.st_gid))
			return false;
		} break;

		case S_IFREG: {
		    if (!createFile(fs.st_mode, fs.st_uid, fs.st_gid))
			return false;
		} break;

		case S_IFLNK: {
		    if (!createLink(fs.st_uid, fs.st_gid))
			return false;
		} break;
	    }
	}

	return true;
    }


    bool
    File::createDirectory(mode_t mode, uid_t owner, gid_t group) const
    {
	if (mkdir(getAbsolutePath(LOC_SYSTEM).c_str(), 0) != 0)
	{
	    if (errno == EEXIST && !checkDir(getAbsolutePath(LOC_SYSTEM)))
	    {
		y2err("mkdir failed path:" << getAbsolutePath(LOC_SYSTEM) << " errno:" << errno);
		return false;
	    }
	}

	if (chmod(getAbsolutePath(LOC_SYSTEM).c_str(), mode) != 0)
	{
	    y2err("chmod failed path:" << getAbsolutePath(LOC_SYSTEM) << " errno:" << errno);
	    return false;
	}

	if (chown(getAbsolutePath(LOC_SYSTEM).c_str(), owner, group) != 0)
	{
	    y2err("chown failed path:" << getAbsolutePath(LOC_SYSTEM) << " errno:" << errno);
	    return false;
	}

	return true;
    }


    bool
    File::createFile(mode_t mode, uid_t owner, gid_t group) const
    {
	// TODO: use clonefile
	SystemCmd cmd(CPBIN " --preserve=mode,ownership " +
		      getAbsolutePath(LOC_PRE) + " " + getAbsolutePath(LOC_SYSTEM));
	return cmd.retcode() == 0;
    }


    bool
    File::createLink(uid_t owner, gid_t group) const
    {
	string tmp;
	readlink(getAbsolutePath(LOC_PRE), tmp);

	if (symlink(tmp, getAbsolutePath(LOC_SYSTEM)) != 0)
	{
	    y2err("symlink failed path:" << getAbsolutePath(LOC_SYSTEM) <<
		  " errno:" << errno);
	    return false;
	}

	if (lchown(getAbsolutePath(LOC_SYSTEM).c_str(), owner, group) != 0)
	{
	    y2err("lchown failed path:" << getAbsolutePath(LOC_SYSTEM) << " errno:" << errno);
	    return false;
	}

	return true;
    }


    bool
    File::deleteAllTypes() const
    {
	struct stat fs;
	if (lstat(getAbsolutePath(LOC_POST).c_str(), &fs) == 0)
	{
	    switch (fs.st_mode & S_IFMT)
	    {
		case S_IFDIR: {
		    if (rmdir(getAbsolutePath(LOC_SYSTEM).c_str()) != 0)
		    {
			y2err("rmdir failed path:" << getAbsolutePath(LOC_SYSTEM) <<
			      " errno:" << errno);
			return false;
		    }
		} break;

		case S_IFREG:
		case S_IFLNK: {
		    if (unlink(getAbsolutePath(LOC_SYSTEM).c_str()) != 0)
		    {
			y2err("unlink failed path:" << getAbsolutePath(LOC_SYSTEM) <<
			      " errno:" << errno);
			return false;
		    }
		} break;
	    }
	}

	return true;
    }


    bool
    File::modifyAllTypes() const
    {
	struct stat fs;
	if (lstat(getAbsolutePath(LOC_PRE).c_str(), &fs) != 0)
	{
	    y2err("lstat failed path:" << getAbsolutePath(LOC_SYSTEM) << " errno:" << errno);
	    return false;
	}
	else if (!createParentDirectories(getAbsolutePath(LOC_SYSTEM)))
	{
	    return false;
	}
	else
	{
	    if (getPreToPostStatus() & CONTENT)
	    {
		switch (fs.st_mode & S_IFMT)
		{
		    case S_IFREG: {
			if (!deleteAllTypes())
			    return false;
			else if (!createFile(fs.st_mode, fs.st_uid, fs.st_gid))
			    return false;
		    } break;

		    case S_IFLNK: {
			if (!deleteAllTypes())
			    return false;
			else if (!createLink(fs.st_uid, fs.st_gid))
			    return false;
		    } break;
		}
	    }

	    if (getPreToPostStatus() & PERMISSIONS)
	    {
		if (chmod(getAbsolutePath(LOC_SYSTEM).c_str(), fs.st_mode) != 0)
		{
		    y2err("chmod failed path:" << getAbsolutePath(LOC_SYSTEM) <<
			  " errno:" << errno);
		    return false;
		}
	    }

	    if (getPreToPostStatus() & (USER | GROUP))
	    {
		if (lchown(getAbsolutePath(LOC_SYSTEM).c_str(), fs.st_uid, fs.st_gid) != 0)
		{
		    y2err("lchown failed path:" << getAbsolutePath(LOC_SYSTEM) <<
			  " errno:" << errno);
		    return false;
		}
	    }
	}

	return true;
    }


    bool
    File::doRollback()
    {
	if (getSnapper()->getRollbackCallback())
	{
	    switch (getAction())
	    {
		case CREATE: getSnapper()->getRollbackCallback()->createInfo(name); break;
		case MODIFY: getSnapper()->getRollbackCallback()->modifyInfo(name); break;
		case DELETE: getSnapper()->getRollbackCallback()->deleteInfo(name); break;
	    }
	}

	bool error = false;

	if (getPreToPostStatus() & CREATED || getPreToPostStatus() & TYPE)
	{
	    if (!deleteAllTypes())
		error = true;
	}

	if (getPreToPostStatus() & DELETED || getPreToPostStatus() & TYPE)
	{
	    if (!createAllTypes())
		error = true;
	}

	if (getPreToPostStatus() & (CONTENT | PERMISSIONS | USER | GROUP))
	{
	    if (!modifyAllTypes())
		error = true;
	}

	if (error && getSnapper()->getRollbackCallback())
	{
	    switch (getAction())
	    {
		case CREATE: getSnapper()->getRollbackCallback()->createError(name); break;
		case MODIFY: getSnapper()->getRollbackCallback()->modifyError(name); break;
		case DELETE: getSnapper()->getRollbackCallback()->deleteError(name); break;
	    }
	}

	return true;
    }


    File::Action
    File::getAction() const
    {
	if (getPreToPostStatus() == CREATED)
	    return DELETE;
	if (getPreToPostStatus() == DELETED)
	    return CREATE;
	return MODIFY;
    }


    RollbackStatistic
    Files::getRollbackStatistic() const
    {
	RollbackStatistic rs;

	for (vector<File>::const_iterator it = entries.begin(); it != entries.end(); ++it)
	{
	    if (it->getRollback())
	    {
		switch (it->getAction())
		{
		    case File::CREATE: rs.numCreate++; break;
		    case File::MODIFY: rs.numModify++; break;
		    case File::DELETE: rs.numDelete++; break;
		}
	    }
	}

	return rs;
    }


    bool
    Files::doRollback()
    {
	y2mil("begin rollback");

	if (getSnapper()->getRollbackCallback())
	    getSnapper()->getRollbackCallback()->start();

	for (vector<File>::reverse_iterator it = entries.rbegin(); it != entries.rend(); ++it)
	{
	    if (it->getRollback())
	    {
		if (it->getPreToPostStatus() == CREATED)
		{
		    it->doRollback();
		}
	    }
	}

	for (vector<File>::iterator it = entries.begin(); it != entries.end(); ++it)
	{
	    if (it->getRollback())
	    {
		if (it->getPreToPostStatus() != CREATED)
		{
		    it->doRollback();
		}
	    }
	}

	if (getSnapper()->getRollbackCallback())
	    getSnapper()->getRollbackCallback()->stop();

	y2mil("end rollback");

	return true;
    }

}
