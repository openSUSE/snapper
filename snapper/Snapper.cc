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
#include "snapper/AsciiFile.h"


namespace snapper
{
    using namespace std;


    Snapper::Snapper(const string& config_name)
	: config_name(config_name), config(NULL), subvolume("/"), snapshots(this), files(this),
	  compare_callback(NULL)
    {
	y2mil("Snapper constructor");
	y2mil("libsnapper version " VERSION);
	y2mil("config_name:" << config_name);

	// TODO error checking
	config = new SysconfigFile(CONFIGSDIR "/" + config_name);

	string val;
	if (config->getValue("SUBVOLUME", val))
	    subvolume = val;

	y2mil("subvolume:" << subvolume);

	snapshots.initialize();
    }


    Snapper::~Snapper()
    {
	y2mil("Snapper destructor");

	delete config;
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
    Snapper::filter1(list<Snapshots::iterator>& tmp1)
    {
	list<Snapshots::iterator> ret;

	for (list<Snapshots::iterator>::const_iterator it1 = tmp1.begin(); it1 != tmp1.end(); ++it1)
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
    Snapper::doCleanupNumber()
    {
	size_t limit = 50;

	string val;
	if (config->getValue("NUMBER_LIMIT", val))
	    val >> limit;

	y2mil("limit:" << limit);

	list<Snapshots::iterator> tmp;

	for (Snapshots::iterator it = snapshots.begin(); it != snapshots.end(); ++it)
	{
	    if (it->getCleanup() == "number")
		tmp.push_back(it);
	}

	if (tmp.size() > limit)
	{
	    list<Snapshots::iterator>::iterator it = tmp.end();
	    advance(it, - limit);
	    tmp.erase(it, tmp.end());

	    filter1(tmp);

	    y2mil("deleting " << tmp.size() << " snapshots");

	    for (list<Snapshots::iterator>::iterator it = tmp.begin(); it != tmp.end(); ++it)
		deleteSnapshot(*it);
	}

	return true;
    }


    bool
    is_first(list<Snapshots::iterator>::const_iterator first,
	     list<Snapshots::iterator>::const_iterator last,
	     Snapshots::const_iterator it1,
	     std::function<bool(const struct tm& tmp1, const struct tm& tmp2)> pred)
    {
	time_t t1 = it1->getDate();
	struct tm tmp1;
	localtime_r(&t1, &tmp1);

	for (list<Snapshots::iterator>::const_iterator it2 = first; it2 != last; ++it2)
	{
	    if (it1 == *it2)
		continue;

	    time_t t2 = (*it2)->getDate();
	    struct tm tmp2;
	    localtime_r(&t2, &tmp2);

	    if (!pred(tmp1, tmp2))
		return true;

	    if (t1 > t2)
		return false;
	}

	return true;
    }


    bool
    equal_year(const struct tm& tmp1, const struct tm& tmp2)
    {
	return tmp1.tm_year == tmp2.tm_year;
    }

    bool
    equal_month(const struct tm& tmp1, const struct tm& tmp2)
    {
	return equal_year(tmp1, tmp2) && tmp1.tm_mon == tmp2.tm_mon;
    }

    bool
    equal_day(const struct tm& tmp1, const struct tm& tmp2)
    {
	return equal_month(tmp1, tmp2) && tmp1.tm_mday == tmp2.tm_mday;
    }

    bool
    equal_hour(const struct tm& tmp1, const struct tm& tmp2)
    {
	return equal_day(tmp1, tmp2) && tmp1.tm_hour == tmp2.tm_hour;
    }


    bool
    is_first_yearly(list<Snapshots::iterator>::const_iterator first,
		    list<Snapshots::iterator>::const_iterator last,
		    Snapshots::const_iterator it1)
    {
	return is_first(first, last, it1, equal_year);
    }

    bool
    is_first_monthly(list<Snapshots::iterator>::const_iterator first,
		     list<Snapshots::iterator>::const_iterator last,
		     Snapshots::const_iterator it1)
    {
	return is_first(first, last, it1, equal_month);
    }

    bool
    is_first_daily(list<Snapshots::iterator>::const_iterator first,
		   list<Snapshots::iterator>::const_iterator last,
		   Snapshots::const_iterator it1)
    {
	return is_first(first, last, it1, equal_day);
    }

    bool
    is_first_hourly(list<Snapshots::iterator>::const_iterator first,
		    list<Snapshots::iterator>::const_iterator last,
		    Snapshots::const_iterator it1)
    {
	return is_first(first, last, it1, equal_hour);
    }


    bool
    Snapper::doCleanupTimeline()
    {
	size_t limit_hourly = 10;
	size_t limit_daily = 10;
	size_t limit_monthly = 10;
	size_t limit_yearly = 10;

	string val;
	if (config->getValue("TIMELINE_LIMIT_HOURLY", val))
	    val >> limit_hourly;
	if (config->getValue("TIMELINE_LIMIT_DAILY", val))
	    val >> limit_daily;
	if (config->getValue("TIMELINE_LIMIT_MONTHLY", val))
	    val >> limit_monthly;
	if (config->getValue("TIMELINE_LIMIT_YEARLY", val))
	    val >> limit_yearly;

	y2mil("limit_hourly:" << limit_hourly << " limit_daily:" << limit_daily <<
	      " limit_monthly:" << limit_monthly << " limit_yearly:" << limit_yearly);

	size_t num_hourly = 0;
	size_t num_daily = 0;
	size_t num_monthly = 0;
	size_t num_yearly = 0;

	list<Snapshots::iterator> tmp;

	for (Snapshots::iterator it = snapshots.begin(); it != snapshots.end(); ++it)
	{
	    if (it->getCleanup() == "timeline")
		tmp.push_front(it);
	}

	list<Snapshots::iterator>::iterator it = tmp.begin();
	while (it != tmp.end())
	{
	    if (num_hourly < limit_hourly && is_first_hourly(it, tmp.end(), *it))
	    {
		++num_hourly;
		it = tmp.erase(it);
	    }
	    else if (num_daily < limit_daily && is_first_daily(it, tmp.end(), *it))
	    {
		++num_daily;
		it = tmp.erase(it);
	    }
	    else if (num_monthly < limit_monthly && is_first_monthly(it, tmp.end(), *it))
	    {
		++num_monthly;
		it = tmp.erase(it);
	    }
	    else if (num_yearly < limit_yearly && is_first_yearly(it, tmp.end(), *it))
	    {
		++num_yearly;
		it = tmp.erase(it);
	    }
	    else
	    {
		++it;
	    }
	}

	tmp.reverse();

	filter1(tmp);

	y2mil("deleting " << tmp.size() << " snapshots");

	for (list<Snapshots::iterator>::iterator it = tmp.begin(); it != tmp.end(); ++it)
	    deleteSnapshot(*it);

	return true;
    }

}
