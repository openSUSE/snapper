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
#include <glob.h>
#include <string.h>
#include <unistd.h>

#include "snapper/File.h"
#include "snapper/Snapper.h"
#include "snapper/Factory.h"
#include "snapper/AppUtil.h"
#include "snapper/XmlFile.h"
#include "snapper/Enum.h"
#include "snapper/SnapperTmpl.h"
#include "snapper/SystemCmd.h"
#include "snapper/SnapperDefines.h"
#include "snapper/Compare.h"


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


    struct AppendHelper
    {
	AppendHelper(const Snapper* snapper, vector<File>& entries)
	    : snapper(snapper), entries(entries) {}
	void operator()(const string& name, unsigned int status)
	    { entries.push_back(File(snapper, name, status)); }
	const Snapper* snapper;
	vector<File>& entries;
    };


    void
    Files::create()
    {
	y2mil("num1:" << snapper->getSnapshot1()->getNum() << " num2:" <<
	      snapper->getSnapshot2()->getNum());

	if (snapper->getCompareCallback())
	    snapper->getCompareCallback()->start();

#if 1
	cmpdirs_cb_t cb = AppendHelper(snapper, entries);
#else
	cmpdirs_cb_t cb = [&snapper, &entries](const string& name, unsigned int status) {
	    entries.push_back(File(snapper, name, status));
	};
#endif
	cmpDirs(snapper->getSnapshot1()->snapshotDir(), snapper->getSnapshot2()->snapshotDir(), cb);

	sort(entries.begin(), entries.end());

	if (snapper->getCompareCallback())
	    snapper->getCompareCallback()->stop();

	y2mil("found " << entries.size() << " lines");
    }


    bool
    Files::load()
    {
	y2mil("num1:" << snapper->getSnapshot1()->getNum() << " num2:" <<
	      snapper->getSnapshot2()->getNum());

	assert(!snapper->getSnapshot1()->isCurrent() && !snapper->getSnapshot2()->isCurrent());

	unsigned int num1 = snapper->getSnapshot1()->getNum();
	unsigned int num2 = snapper->getSnapshot2()->getNum();

	bool invert = num1 > num2;

	if (invert)
	    swap(num1, num2);

	string input = snapper->snapshotsDir() + "/" + decString(num2) + "/filelist-" +
	    decString(num1) + ".txt";

	FILE* file = fopen(input.c_str(), "r");
	if (file == NULL)
	{
	    y2mil("file not found");
	    return false;
	}

	char* line = NULL;
	size_t len = 0;

	while (getline(&line, &len, file) != -1)
	{
	    // TODO: more robust splitting

	    string name = string(line, 5, strlen(line) - 6);

	    unsigned int status = stringToStatus(string(line, 0, 4));

	    if (invert)
		status = invertStatus(status);

	    File file(snapper, name, status);
	    entries.push_back(file);
	}

	free(line);

	fclose(file);

	sort(entries.begin(), entries.end());

	y2mil("read " << entries.size() << " lines");

	return true;
    }


    bool
    Files::save()
    {
	y2mil("num1:" << snapper->getSnapshot1()->getNum() << " num2:" << snapper->getSnapshot2()->getNum());

	assert(!snapper->getSnapshot1()->isCurrent() && !snapper->getSnapshot2()->isCurrent());

	unsigned int num1 = snapper->getSnapshot1()->getNum();
	unsigned int num2 = snapper->getSnapshot2()->getNum();

	bool invert = num1 > num2;

	if (invert)
	    swap(num1, num2);

	string output = snapper->snapshotsDir() + "/" + decString(num2) + "/filelist-" +
	    decString(num1) + ".txt";

	char* tmp_name = (char*) malloc(output.length() + 12);
	strcpy(tmp_name, output.c_str());
	strcat(tmp_name, ".tmp-XXXXXX");

	int fd = mkstemp(tmp_name);

	FILE* file = fdopen(fd, "w");

	for (const_iterator it = entries.begin(); it != entries.end(); ++it)
	{
	    unsigned int status = it->getPreToPostStatus();

	    if (invert)
		status = invertStatus(status);

	    fprintf(file, "%s %s\n", statusToString(status).c_str(), it->getName().c_str());
	}

	fclose(file);

	rename(tmp_name, output.c_str());

	free(tmp_name);

	return true;
    }


    void
    Files::initialize()
    {
	entries.clear();

	if (snapper->getSnapshot1()->isCurrent() || snapper->getSnapshot2()->isCurrent())
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
    File::getAbsolutePath(Location loc)
    {
	switch (loc)
	{
	    case LOC_PRE:
		return snapper->getSnapshot1()->snapshotDir() + name;

	    case LOC_POST:
		return snapper->getSnapshot2()->snapshotDir() + name;

	    case LOC_SYSTEM:
		if (snapper->rootDir() == "/")
		    return name;
		else
		    return snapper->rootDir() + name;
	}

	return "error";
    }


    bool
    File::doRollback()
    {
	if (getPreToPostStatus() & CREATED || getPreToPostStatus() & TYPE)
	{
	    cout << "delete " << name << endl;

	    struct stat fs;
	    getLStat(getAbsolutePath(LOC_POST), fs);

	    switch (fs.st_mode & S_IFMT)
	    {
		case S_IFDIR: {
		    rmdir(getAbsolutePath(LOC_SYSTEM).c_str());
		} break;

		case S_IFREG: {
		    unlink(getAbsolutePath(LOC_SYSTEM).c_str());
		} break;

		case S_IFLNK: {
		    unlink(getAbsolutePath(LOC_SYSTEM).c_str());
		} break;
	    }
	}

	if (getPreToPostStatus() & DELETED || getPreToPostStatus() & TYPE)
	{
	    cout << "create " << name << endl;

	    struct stat fs;
	    getLStat(getAbsolutePath(LOC_PRE), fs);

	    switch (fs.st_mode & S_IFMT)
	    {
		case S_IFDIR: {
		    mkdir(getAbsolutePath(LOC_SYSTEM).c_str(), 0);
		    chmod(getAbsolutePath(LOC_SYSTEM).c_str(), fs.st_mode);
		    chown(getAbsolutePath(LOC_SYSTEM).c_str(), fs.st_uid, fs.st_gid);
		} break;

		case S_IFREG: {
		    SystemCmd cmd(CPBIN " --preserve=mode,ownership " +
				  getAbsolutePath(LOC_PRE) + " " + getAbsolutePath(LOC_SYSTEM));
		} break;

		case S_IFLNK: {
		    string tmp;
		    readlink(getAbsolutePath(LOC_PRE), tmp);
		    symlink(tmp, getAbsolutePath(LOC_SYSTEM));
		    lchown(getAbsolutePath(LOC_SYSTEM).c_str(), fs.st_uid, fs.st_gid);
		} break;
	    }
	}

	if (getPreToPostStatus() & (CONTENT | PERMISSIONS | USER | GROUP))
	{
	    cout << "modify " << name << endl;

	    struct stat fs;
	    getLStat(getAbsolutePath(LOC_PRE), fs);

	    if (getPreToPostStatus() & CONTENT)
	    {
		switch (fs.st_mode & S_IFMT)
		{
		    case S_IFREG: {
			SystemCmd cmd(CPBIN " --preserve=mode,ownership " +
				      getAbsolutePath(LOC_PRE) + " " + getAbsolutePath(LOC_SYSTEM));
		    } break;

		    case S_IFLNK: {
			unlink(getAbsolutePath(LOC_SYSTEM).c_str());
			string tmp;
			readlink(getAbsolutePath(LOC_PRE), tmp);
			symlink(tmp, getAbsolutePath(LOC_SYSTEM));
		    } break;
		}
	    }

	    if (getPreToPostStatus() & PERMISSIONS)
	    {
		chmod(getAbsolutePath(LOC_SYSTEM).c_str(), fs.st_mode);
	    }

	    if (getPreToPostStatus() & (USER | GROUP))
	    {
		chown(getAbsolutePath(LOC_SYSTEM).c_str(), fs.st_uid, fs.st_gid);
	    }
	}

	return true;
    }


    RollbackStatistic
    Files::getRollbackStatistic() const
    {
	RollbackStatistic rs;

	for (vector<File>::const_iterator it = entries.begin(); it != entries.end(); ++it)
	{
	    if (it->getRollback())
	    {
		if (it->getPreToPostStatus() == CREATED)
		    rs.numDelete++;
		else if (it->getPreToPostStatus() == DELETED)
		    rs.numCreate++;
		else
		    rs.numModify++;
	    }
	}

	return rs;
    }


    bool
    Files::doRollback()
    {
	y2mil("begin rollback");

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

	y2mil("end rollback");

	return true;
    }

}
