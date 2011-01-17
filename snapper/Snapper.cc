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


#include <glob.h>
#include <string.h>
#include <map>
#include <iostream>

#include "snapper/Snapper.h"
#include "snapper/AppUtil.h"
#include "snapper/XmlFile.h"
#include "snapper/Enum.h"
#include "snapper/SnapperTmpl.h"
#include "snapper/SystemCmd.h"
#include "snapper/SnapperDefines.h"
#include "snapper/Files.h"


namespace snapper
{
    using namespace std;


    bool initialized = false;

    list<Snapshot> snapshots;


    Snapshot snapshot1;
    Snapshot snapshot2;


    Filelist filelist;


    std::ostream& operator<<(std::ostream& s, const Snapshot& x)
    {
	s << "type:" << toString(x.type) << " num:" << x.num;

	if (x.pre_num != 0)
	    s << " pre-num:" << x.pre_num;

	s << " date:" << x.date;

	if (!x.description.empty())
	    s << " description:" << x.description;

	return s;
    }


    string
    snapshotDir(const Snapshot& snapshot)
    {
	if (snapshot.num == 0)
	    return "/";
	else
	    return SNAPSHOTSDIR "/" + decString(snapshot.num) + "/snapshot";
    }


    string
    getAbsolutePath(const string& name, Location loc)
    {
	switch (loc)
	{
	    case LOC_PRE:
		return snapshotDir(snapshot1) + name;

	    case LOC_POST:
		return snapshotDir(snapshot2) + name;

	    case LOC_SYSTEM:
		return name;
	}

	return "error";
    }


    void
    readSnapshots()
    {
	list<string> infos = glob(SNAPSHOTSDIR "/*/info.xml", GLOB_NOSORT);
	for (list<string>::const_iterator it = infos.begin(); it != infos.end(); ++it)
	{
	    unsigned int num;
	    it->substr(11) >> num;

	    XmlFile file(*it);
	    const xmlNode* root = file.getRootElement();
	    const xmlNode* node = getChildNode(root, "snapshot");

	    Snapshot snapshot;

	    string tmp;

	    if (getChildValue(node, "type", tmp))
	    {
		if (!toValue(tmp, snapshot.type, true))
		{
		}
	    }

	    getChildValue(node, "num", snapshot.num);
	    assert(num == snapshot.num);

	    getChildValue(node, "date", snapshot.date);

	    getChildValue(node, "description", snapshot.description);

	    getChildValue(node, "pre_num", snapshot.pre_num);

	    snapshots.push_back(snapshot);
	}

	snapshots.sort();
    }


    void
    initialize()
    {
	initialized = true;

	readSnapshots();
    }


    void
    assertInit()
    {
	if (!initialized)
	    initialize();
    }


    bool
    getSnapshot(unsigned int num, Snapshot& snapshot)
    {
	assertInit();

	for (list<Snapshot>::const_iterator it = snapshots.begin();
	     it != snapshots.end(); ++it)
	{
	    if (it->num == num)
	    {
		snapshot = *it;
		return true;
	    }
	}

	return false;
    }


    list<Snapshot>
    getSnapshots()
    {
	assertInit();

	return snapshots;
    }


    void
    listSnapshots()
    {
	assertInit();

	for (list<Snapshot>::const_iterator it = snapshots.begin();
	     it != snapshots.end(); ++it)
	{
	    cout << *it << endl;
	}
    }


    unsigned int
    nextSnapshotNumber()
    {
	assertInit();

	unsigned int num = 1;

	if (!snapshots.empty())
	    num = snapshots.rbegin()->num + 1;

	return num;
    }


    bool
    writeInfo(const Snapshot& snapshot)
    {
	createPath(SNAPSHOTSDIR "/" + decString(snapshot.num));

	XmlFile xml;
	xmlNode* node = xmlNewNode("snapshot");
	xml.setRootElement(node);

	setChildValue(node, "type", toString(snapshot.type));

	setChildValue(node, "num", snapshot.num);

	setChildValue(node, "date", snapshot.date);

	if (snapshot.type == SINGLE || snapshot.type == PRE)
	    setChildValue(node, "description", snapshot.description);

	if (snapshot.type == POST)
	    setChildValue(node, "pre_num", snapshot.pre_num);

	xml.save(SNAPSHOTSDIR "/" + decString(snapshot.num) + "/info.xml");

	return true;
    }


    bool
    createBtrfsSnapshot(const Snapshot& snapshot)
    {
	string dest = SNAPSHOTSDIR "/" + decString(snapshot.num) + "/snapshot";

	SystemCmd cmd(BTRFSBIN " subvolume snapshot / " + dest);
	return cmd.retcode() == 0;
    }


    unsigned int
    createSingleSnapshot(string description)
    {
	Snapshot snapshot;
	snapshot.type = SINGLE;
	snapshot.num = nextSnapshotNumber();
	snapshot.date = datetime();
	snapshot.description = description;

	snapshots.push_back(snapshot);
	writeInfo(snapshot);
	createBtrfsSnapshot(snapshot);

	return snapshot.num;
    }


    unsigned int
    createPreSnapshot(string description)
    {
	Snapshot snapshot;
	snapshot.type = PRE;
	snapshot.num = nextSnapshotNumber();
	snapshot.date = datetime();
	snapshot.description = description;

	snapshots.push_back(snapshot);
	writeInfo(snapshot);
	createBtrfsSnapshot(snapshot);

	return snapshot.num;
    }


    unsigned int
    createPostSnapshot(unsigned int pre_num)
    {
	Snapshot snapshot;
	snapshot.type = POST;
	snapshot.num = nextSnapshotNumber();
	snapshot.date = datetime();
	snapshot.pre_num = pre_num;

	snapshots.push_back(snapshot);
	writeInfo(snapshot);
	createBtrfsSnapshot(snapshot);

	return snapshot.num;
    }


    void
    startBackgroundComparsion(unsigned int num1, unsigned int num2)
    {
	y2mil("num1:" << num1 << " num2:" << num2);

	string dir1 = SNAPSHOTSDIR "/" + decString(num1) + "/snapshot";
	string dir2 = SNAPSHOTSDIR "/" + decString(num2) + "/snapshot";

	string output = SNAPSHOTSDIR "/" + decString(num2) + "/filelist-" + decString(num1) + ".txt";

	SystemCmd(COMPAREDIRSBIN " " + quote(dir1) + " " + quote(dir2) + " " + quote(output));
    }


    bool
    setComparisonNums(unsigned int num1, unsigned int num2)
    {
	y2mil("num1:" << num1 << " num2:" << num2);

	if (!getSnapshot(num1, snapshot1))
	    return false;

	if (!getSnapshot(num2, snapshot2))
	    return false;

	filelist.assertInit();

	return true;
    }


    void
    append_helper(const string& name, unsigned int status)
    {
	filelist.files.push_back(File(name, status));
    }


    void
    Filelist::create()
    {
	y2mil("num1:" << snapshot1.num << " num2:" << snapshot2.num);

	files.clear();

	cmpDirs(snapshotDir(snapshot1), snapshotDir(snapshot2), append_helper);

	sort(files.begin(), files.end());

	y2mil("found " << files.size() << " lines");
    }


    bool
    Filelist::load()
    {
	y2mil("num1:" << snapshot1.num << " num2:" << snapshot2.num);

	files.clear();

	string input = SNAPSHOTSDIR "/" + decString(snapshot2.num) + "/filelist-" +
	    decString(snapshot1.num) + ".txt";

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
	y2mil("num1:" << snapshot1.num << " num2:" << snapshot2.num);

	string output = SNAPSHOTSDIR "/" + decString(snapshot2.num) + "/filelist-" +
	    decString(snapshot1.num) + ".txt";

	char* tmp_name = (char*) malloc(output.length() + 12);
	strcpy(tmp_name, output.c_str());
	strcat(tmp_name, ".tmp-XXXXXX");

	int fd = mkstemp(tmp_name);

	FILE* file = fdopen(fd, "w");

	for (vector<File>::const_iterator it = files.begin(); it != files.end(); ++it)
	    fprintf(file, "%s %s\n", statusToString(it->pre_to_post_status).c_str(), it->name.c_str());

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


    list<string>
    getFiles()
    {
	filelist.assertInit();

	list<string> ret;
	for (vector<File>::const_iterator it = filelist.begin(); it != filelist.end(); ++it)
	    ret.push_back(it->name);

	return ret;
    }


    inline bool
    file_name_less(const File& file, const string& name)
    {
	return file.name < name;
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
    File::getPreToPostStatus()
    {
	return pre_to_post_status;
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


    unsigned int
    getStatus(const string& name, Cmp cmp)
    {
	vector<File>::iterator it = filelist.find(name);
	if (it != filelist.end())
	    return it->getStatus(cmp);

	return -1;
    }


    void
    setRollback(const string& name, bool rollback)
    {
	vector<File>::iterator it = filelist.find(name);
	if (it != filelist.end())
	    it->rollback = rollback;
    }


    bool
    getRollback(const string& name, bool rollback)
    {
	vector<File>::const_iterator it = filelist.find(name);
	if (it != filelist.end())
	    return it->rollback;
	return false;
    }

}
