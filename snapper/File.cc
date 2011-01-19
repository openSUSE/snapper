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

#include "snapper/Snapper.h"
#include "snapper/File.h"
#include "snapper/AppUtil.h"
#include "snapper/XmlFile.h"
#include "snapper/Enum.h"
#include "snapper/SnapperTmpl.h"
#include "snapper/SystemCmd.h"
#include "snapper/SnapperDefines.h"
#include "snapper/Compare.h"


namespace snapper
{
    Filelist filelist;


    void
    append_helper(const string& name, unsigned int status)
    {
	filelist.files.push_back(File(name, status));
    }


    void
    Filelist::create()
    {
	y2mil("num1:" << snapshot1->num << " num2:" << snapshot2->num);

	files.clear();

	cmpDirs(snapshot1->snapshotDir(), snapshot2->snapshotDir(), append_helper);

	sort(files.begin(), files.end());

	y2mil("found " << files.size() << " lines");
    }


    bool
    Filelist::load()
    {
	y2mil("num1:" << snapshot1->num << " num2:" << snapshot2->num);

	files.clear();

	string input = snapshot2->baseDir() + "/filelist-" + decString(snapshot1->num) + ".txt";

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
	    string name = string(line, 5, strlen(line) - 6);

	    File file(name, stringToStatus(string(line, 0, 4)));
	    files.push_back(file);
	}

	free(line);

	fclose(file);

	sort(files.begin(), files.end());

	y2mil("read " << files.size() << " lines");

	return true;
    }


    bool
    Filelist::save()
    {
	y2mil("num1:" << snapshot1->num << " num2:" << snapshot2->num);

	string output = snapshot2->baseDir() + "/filelist-" + decString(snapshot1->num) + ".txt";

	char* tmp_name = (char*) malloc(output.length() + 12);
	strcpy(tmp_name, output.c_str());
	strcat(tmp_name, ".tmp-XXXXXX");

	int fd = mkstemp(tmp_name);

	FILE* file = fdopen(fd, "w");

	for (vector<File>::const_iterator it = files.begin(); it != files.end(); ++it)
	    fprintf(file, "%s %s\n", statusToString(it->getPreToPostStatus()).c_str(),
		    it->getName().c_str());

	fclose(file);

	rename(tmp_name, output.c_str());

	free(tmp_name);

	return true;
    }


    void
    Filelist::assertInit()
    {
	if (!initialized)
	    initialize();
    }


    void
    Filelist::initialize()
    {
	if (initialized)
	    return;

	if (!load())
	{
	    create();
	    save();
	}

	initialized = true;
    }


    inline bool
    file_name_less(const File& file, const string& name)
    {
	return file.getName() < name;
    }


    vector<File>::iterator
    Filelist::find(const string& name)
    {
	return lower_bound(files.begin(), files.end(), name, file_name_less);
    }


    vector<File>::const_iterator
    Filelist::find(const string& name) const
    {
	return lower_bound(files.begin(), files.end(), name, file_name_less);
    }


    unsigned int
    File::getPreToSystemStatus()
    {
	if (pre_to_system_status == (unsigned int)(-1))
	    pre_to_system_status = cmpFiles(getAbsolutePath(name, LOC_PRE),
					    getAbsolutePath(name, LOC_SYSTEM));
	return pre_to_system_status;
    }


    unsigned int
    File::getPostToSystemStatus()
    {
	if (post_to_system_status == (unsigned int)(-1))
	    post_to_system_status = cmpFiles(getAbsolutePath(name, LOC_POST),
					     getAbsolutePath(name, LOC_SYSTEM));
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

}
