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
#include "snapper/AppUtil.h"
#include "snapper/XmlFile.h"
#include "snapper/Enum.h"
#include "snapper/SnapperTmpl.h"
#include "snapper/SystemCmd.h"
#include "snapper/SnapperDefines.h"
#include "snapper/File.h"


namespace snapper
{
    using namespace std;


/*
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
*/


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

	snapshots.assertInit();

	snapshot1 = snapshots.find(num1);
	if (snapshot1 == snapshots.end())
	    return false;

	snapshot2 = snapshots.find(num2);
	if (snapshot2 == snapshots.end())
	    return false;

	files.assertInit();

	return true;
    }


/*
    list<string>
    getFiles()
    {
	filelist.assertInit();

	list<string> ret;
	for (vector<File>::const_iterator it = filelist.begin(); it != filelist.end(); ++it)
	    ret.push_back(it->name);

	return ret;
    }
*/


    unsigned int
    getStatus(const string& name, Cmp cmp)
    {
	vector<File>::iterator it = files.find(name);
	if (it != files.end())
	    return it->getStatus(cmp);

	return -1;
    }


    void
    setRollback(const string& name, bool rollback)
    {
	vector<File>::iterator it = files.find(name);
	if (it != files.end())
	    it->setRollback(rollback);
    }


    bool
    getRollback(const string& name, bool rollback)
    {
	vector<File>::const_iterator it = files.find(name);
	if (it != files.end())
	    return it->getRollback();
	return false;
    }

}
