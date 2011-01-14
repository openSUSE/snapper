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


    bool files_loaded = false;

    list<string> files;

    map<string, unsigned int> pre_to_post_status;
    map<string, unsigned int> pre_to_system_status;
    map<string, unsigned int> post_to_system_status;


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


    bool operator<(const Snapshot& a, const Snapshot& b)
    {
	return a.num < b.num;
    }


    string
    snapshotDir(const Snapshot& snapshot)
    {
	if (snapshot.num == 0)
	    return "/";
	else
	    return SNAPSHOTSDIR "/" + decString(snapshot.num) + "/snapshot";
    }


    void
    readSnapshots()
    {
	list<string> infos = glob(SNAPSHOTSDIR "/*/snapshot.info", GLOB_NOSORT);
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


    const list<Snapshot>&
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

	xml.save(SNAPSHOTSDIR "/" + decString(snapshot.num) + "/snapshot.info");

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
	if (num1 == 0 || !getSnapshot(num1, snapshot1))
	    return false;

	if (num2 != 0 && !getSnapshot(num2, snapshot2))
	    return false;

	if (snapshot1.num != snapshot2.pre_num)
	    return false;

	return true;
    }


    void
    log(const string& file, unsigned int status)
    {
	files.push_back(file);
	pre_to_post_status[file] = status;
    }


    void
    createFilelist()
    {
	y2mil("num1:" << snapshot1.num << " num2:" << snapshot2.num);

	files.clear();
	pre_to_post_status.clear();

	cmpDirs(snapshotDir(snapshot1), snapshotDir(snapshot2), log);
    }


    bool
    loadFilelist()
    {
	y2mil("num1:" << snapshot1.num << " num2:" << snapshot2.num);

	files.clear();
	pre_to_post_status.clear();

	string input = SNAPSHOTSDIR "/" + decString(snapshot2.num) + "/filelist-" +
	    decString(snapshot1.num) + ".txt";

	FILE* file = fopen(input.c_str(), "r");
	if (file == NULL)
	    return false;

	char* line = NULL;
	size_t len = 0;

	while (getline(&line, &len, file) != -1)
	{
	    string file = string(line, 5, strlen(line) - 6);

	    files.push_back(file);
	    pre_to_post_status[file] = stringToStatus(string(line, 0, 4));
	}

	free(line);

	fclose(file);

	return true;
    }


    bool
    saveFilelist()
    {
	y2mil("num1:" << snapshot1.num << " num2:" << snapshot2.num);

	string output = SNAPSHOTSDIR "/" + decString(snapshot2.num) + "/filelist-" +
	    decString(snapshot1.num) + ".txt";

	char* tmp_name = (char*) malloc(output.length() + 12);
	strcpy(tmp_name, output.c_str());
	strcat(tmp_name, ".tmp-XXXXXX");

	int fd = mkstemp(tmp_name);

	FILE* file = fdopen(fd, "w");

	for (list<string>::const_iterator it = files.begin(); it != files.end(); ++it)
	    fprintf(file, "%s %s\n", statusToString(getStatus(*it, CMP_PRE_TO_POST)).c_str(), it->c_str());

	fclose(file);

	rename(tmp_name, output.c_str());

	free(tmp_name);

	return true;
    }


    const list<string>&
    getFiles()
    {
	if (!files_loaded)
	{
	    if (!loadFilelist())
	    {
		createFilelist();
		saveFilelist();
	    }

	    files_loaded = true;
	}

	return files;
    }


    unsigned int
    getStatus(const string& file, Cmp cmp)
    {
	return pre_to_post_status[file];
    }

}
