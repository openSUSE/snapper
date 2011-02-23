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

#include "config.h"
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


    Snapper::Snapper(const string& subvolume)
	: subvolume(subvolume), snapshots(this), files(this), compare_callback(NULL)
    {
	y2mil("Snapper constructor");
	y2mil("libsnapper version " VERSION);
	y2mil("subvolume:" << subvolume);

	snapshots.initialize();
    }


    Snapper::~Snapper()
    {
	y2mil("Snapper destructor");
    }


    // Directory of which snapshots are made, e.g. "/" or "/home".
    string
    Snapper::subvolumeDir() const
    {
	return subvolume;
    }


    // Directory containing directories for all snapshots, e.g. "/snapshots"
    // or "/home/snapshots".
    string
    Snapper::snapshotsDir() const
    {
	if (subvolumeDir() == "/")
	    return SNAPSHOTSDIR;
	else
	    return subvolumeDir() + SNAPSHOTSDIR;
    }


    Snapshots::const_iterator
    Snapper::getSnapshotCurrent() const
    {
	return snapshots.getSnapshotCurrent();
    }


    Snapshots::iterator
    Snapper::createSingleSnapshot(string description)
    {
	return snapshots.createSingleSnapshot(description);
    }


    Snapshots::iterator
    Snapper::createPreSnapshot(string description)
    {
	return snapshots.createPreSnapshot(description);
    }


    Snapshots::iterator
    Snapper::createPostSnapshot(Snapshots::const_iterator pre)
    {
	assert(pre != snapshots.end());

	return snapshots.createPostSnapshot(pre);
    }


    void
    Snapper::deleteSnapshot(Snapshots::iterator snapshot)
    {
	assert(snapshot != snapshots.end());

	snapshots.deleteSnapshot(snapshot);
    }


    void
    Snapper::startBackgroundComparsion(Snapshots::const_iterator snapshot1,
				       Snapshots::const_iterator snapshot2)
    {
	assert(snapshot1 != snapshots.end() && snapshot2 != snapshots.end());

	y2mil("num1:" << snapshot1->getNum() << " num2:" << snapshot2->getNum());

	bool invert = snapshot1->getNum() > snapshot2->getNum();

	if (invert)
	    swap(snapshot1, snapshot1);

	string dir1 = snapshot1->snapshotDir();
	string dir2 = snapshot2->snapshotDir();

	string output = snapshot2->baseDir() + "/filelist-" + decString(snapshot1->getNum()) +
	    ".txt";

	SystemCmd(COMPAREDIRSBIN " " + quote(dir1) + " " + quote(dir2) + " " + quote(output));
    }


    bool
    Snapper::setComparison(Snapshots::const_iterator new_snapshot1,
			   Snapshots::const_iterator new_snapshot2)
    {
	assert(new_snapshot1 != snapshots.end() && new_snapshot2 != snapshots.end());
	assert(new_snapshot1 != new_snapshot2);

	y2mil("num1:" << new_snapshot1->getNum() << " num2:" << new_snapshot2->getNum());

	snapshot1 = new_snapshot1;
	snapshot2 = new_snapshot2;

	files.initialize();

	return true;
    }


    RollbackStatistic
    Snapper::getRollbackStatistic() const
    {
	return files.getRollbackStatistic();
    }


    bool
    Snapper::doRollback()
    {
	return files.doRollback();
    }


    // Removes pre and post snapshots from tmp that do have a corresponding
    // snapshot but which is not included in tmp.
    void
    Snapper::filter1(vector<Snapshots::iterator>& tmp1)
    {
	vector<Snapshots::iterator> ret;

	for (vector<Snapshots::iterator>::const_iterator it1 = tmp1.begin(); it1 != tmp1.end(); ++it1)
	{
	    if ((*it1)->getType() == PRE)
	    {
		Snapshots::const_iterator it2 = snapshots.findPost(*it1);
		if (it2 != snapshots.end())
		{
		    if (find(tmp1.begin(), tmp1.end(), it2) == tmp1.end())
			continue;
		}
	    }

	    if ((*it1)->getType() == POST)
	    {
		Snapshots::const_iterator it2 = snapshots.findPre(*it1);
		if (it2 != snapshots.end())
		{
		    if (find(tmp1.begin(), tmp1.end(), it2) == tmp1.end())
			continue;
		}
	    }

	    ret.push_back(*it1);
	}

	swap(ret, tmp1);
    }


    bool
    Snapper::doCleanupAmount()
    {
	size_t limit = 10;		// TODO

	y2mil("limit:" << limit);

	vector<Snapshots::iterator> tmp;

	for (Snapshots::iterator it = snapshots.begin(); it != snapshots.end(); ++it)
	{
	    if (it->getCleanup() == "amount")
		tmp.push_back(it);
	}

	if (tmp.size() > limit)
	{
	    tmp.erase(tmp.end() - limit, tmp.end());
	    filter1(tmp);

	    y2mil("deleting " << tmp.size() << " snapshots");

	    for (vector<Snapshots::iterator>::iterator it = tmp.begin(); it != tmp.end(); ++it)
		deleteSnapshot(*it);
	}

	return true;
    }


    bool
    Snapper::doCleanupTimeline()
    {
	// TODO: hourly, daily, monthly, yearly algorithm

	size_t limit = 30;

	vector<Snapshots::iterator> tmp;

	for (Snapshots::iterator it = snapshots.begin(); it != snapshots.end(); ++it)
	{
	    if (it->getCleanup() == "timeline")
		tmp.push_back(it);
	}

	if (tmp.size() > limit)
	{
	    tmp.erase(tmp.end() - limit, tmp.end());
	    filter1(tmp);

	    y2mil("deleting " << tmp.size() << " snapshots");

	    for (vector<Snapshots::iterator>::iterator it = tmp.begin(); it != tmp.end(); ++it)
		deleteSnapshot(*it);
	}

	return true;
    }

}
