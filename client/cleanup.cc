/*
 * Copyright (c) [2011-2014] Novell, Inc.
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


#include <algorithm>

#include <snapper/SnapperTmpl.h>

#include "utils/equal-date.h"

#include "commands.h"

using namespace std;


struct younger_than
{
    younger_than(time_t t)
	: t(t) {}
    bool operator()(XSnapshots::const_iterator it)
	{ return it->getDate() > t; }
    const time_t t;
};


// Removes snapshots younger than min_age from tmp
void
filter1(list<XSnapshots::const_iterator>& tmp, time_t min_age)
{
    tmp.remove_if(younger_than(time(NULL) - min_age));
}


// Removes pre and post snapshots from tmp that do have a corresponding
// snapshot but which is not included in tmp.
void
filter2(const XSnapshots& snapshots, list<XSnapshots::const_iterator>& tmp)
{
    list<XSnapshots::const_iterator> ret;

    for (list<XSnapshots::const_iterator>::const_iterator it1 = tmp.begin(); it1 != tmp.end(); ++it1)
    {
	if ((*it1)->getType() == PRE)
	{
	    XSnapshots::const_iterator it2 = snapshots.findPost(*it1);
	    if (it2 != snapshots.end())
	    {
		if (find(tmp.begin(), tmp.end(), it2) == tmp.end())
		    continue;
	    }
	}

	if ((*it1)->getType() == POST)
	{
	    XSnapshots::const_iterator it2 = snapshots.findPre(*it1);
	    if (it2 != snapshots.end())
	    {
		if (find(tmp.begin(), tmp.end(), it2) == tmp.end())
		    continue;
	    }
	}

	ret.push_back(*it1);
    }

    swap(ret, tmp);
}


bool
is_important(XSnapshots::const_iterator it1)
{
    map<string, string>::const_iterator it2 = it1->getUserdata().find("important");
    return it2 != it1->getUserdata().end() && it2->second == "yes";
}


bool
do_cleanup_number(DBus::Connection& conn, const string& config_name, bool verbose)
{
    time_t min_age = 1800;
    size_t limit = 50;
    size_t limit_important = 10;

    XConfigInfo ci = command_get_xconfig(conn, config_name);
    map<string, string>::const_iterator pos;
    if ((pos = ci.raw.find("NUMBER_MIN_AGE")) != ci.raw.end())
	pos->second >> min_age;
    if ((pos = ci.raw.find("NUMBER_LIMIT")) != ci.raw.end())
	pos->second >> limit;
    if ((pos = ci.raw.find("NUMBER_LIMIT_IMPORTANT")) != ci.raw.end())
	pos->second >> limit_important;

    size_t num = 0;
    size_t num_important = 0;

    XSnapshots snapshots = command_list_xsnapshots(conn, config_name);

    list<XSnapshots::const_iterator> tmp;

    for (XSnapshots::const_iterator it = snapshots.begin(); it != snapshots.end(); ++it)
    {
	if (it->getCleanup() == "number")
	    tmp.push_front(it);
    }

    list<XSnapshots::const_iterator>::iterator it = tmp.begin();
    while (it != tmp.end())
    {
	bool keep = false;

	if (num_important < limit_important && is_important(*it))
	{
	    ++num_important;
	    keep = true;
	}
	if (num < limit)
	{
	    ++num;
	    keep = true;
	}

	if (keep)
	    it = tmp.erase(it);
	else
	    ++it;
    }

    tmp.reverse();

    filter1(tmp, min_age);
    filter2(snapshots, tmp);

    for (list<XSnapshots::const_iterator>::const_iterator it = tmp.begin(); it != tmp.end(); ++it)
    {
	list<unsigned int> nums;
	nums.push_back((*it)->getNum());
	command_delete_xsnapshots(conn, config_name, nums, verbose);
    }

    return true;
}


bool
is_first(list<XSnapshots::const_iterator>::const_iterator first,
	 list<XSnapshots::const_iterator>::const_iterator last,
	 XSnapshots::const_iterator it1,
	 std::function<bool(const struct tm& tmp1, const struct tm& tmp2)> pred)
{
    time_t t1 = it1->getDate();
    struct tm tmp1;
    localtime_r(&t1, &tmp1);

    for (list<XSnapshots::const_iterator>::const_iterator it2 = first; it2 != last; ++it2)
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
is_first_yearly(list<XSnapshots::const_iterator>::const_iterator first,
		list<XSnapshots::const_iterator>::const_iterator last,
		XSnapshots::const_iterator it1)
{
    return is_first(first, last, it1, equal_year);
}

bool
is_first_monthly(list<XSnapshots::const_iterator>::const_iterator first,
		 list<XSnapshots::const_iterator>::const_iterator last,
		 XSnapshots::const_iterator it1)
{
    return is_first(first, last, it1, equal_month);
}

bool
is_first_weekly(list<XSnapshots::const_iterator>::const_iterator first,
		list<XSnapshots::const_iterator>::const_iterator last,
		XSnapshots::const_iterator it1)
{
    return is_first(first, last, it1, equal_week);
}

bool
is_first_daily(list<XSnapshots::const_iterator>::const_iterator first,
	       list<XSnapshots::const_iterator>::const_iterator last,
	       XSnapshots::const_iterator it1)
{
    return is_first(first, last, it1, equal_day);
}

bool
is_first_hourly(list<XSnapshots::const_iterator>::const_iterator first,
		list<XSnapshots::const_iterator>::const_iterator last,
		XSnapshots::const_iterator it1)
{
    return is_first(first, last, it1, equal_hour);
}


bool
do_cleanup_timeline(DBus::Connection& conn, const string& config_name, bool verbose)
{
    time_t min_age = 1800;
    size_t limit_hourly = 10;
    size_t limit_daily = 10;
    size_t limit_monthly = 10;
    size_t limit_weekly = 0;
    size_t limit_yearly = 10;

    XConfigInfo ci = command_get_xconfig(conn, config_name);
    map<string, string>::const_iterator pos;
    if ((pos = ci.raw.find("TIMELINE_MIN_AGE")) != ci.raw.end())
	pos->second >> min_age;
    if ((pos = ci.raw.find("TIMELINE_LIMIT_HOURLY")) != ci.raw.end())
	pos->second >> limit_hourly;
    if ((pos = ci.raw.find("TIMELINE_LIMIT_DAILY")) != ci.raw.end())
	pos->second >> limit_daily;
    if ((pos = ci.raw.find("TIMELINE_LIMIT_WEEKLY")) != ci.raw.end())
	pos->second >> limit_weekly;
    if ((pos = ci.raw.find("TIMELINE_LIMIT_MONTHLY")) != ci.raw.end())
	pos->second >> limit_monthly;
    if ((pos = ci.raw.find("TIMELINE_LIMIT_YEARLY")) != ci.raw.end())
	pos->second >> limit_yearly;

    size_t num_hourly = 0;
    size_t num_daily = 0;
    size_t num_weekly = 0;
    size_t num_monthly = 0;
    size_t num_yearly = 0;

    XSnapshots snapshots = command_list_xsnapshots(conn, config_name);

    list<XSnapshots::const_iterator> tmp;

    for (XSnapshots::const_iterator it = snapshots.begin(); it != snapshots.end(); ++it)
    {
	if (it->getCleanup() == "timeline")
	    tmp.push_front(it);
    }

    list<XSnapshots::const_iterator>::iterator it = tmp.begin();
    while (it != tmp.end())
    {
	bool keep = false;

	if (num_hourly < limit_hourly && is_first_hourly(it, tmp.end(), *it))
	{
	    ++num_hourly;
	    keep = true;
	}
	if (num_daily < limit_daily && is_first_daily(it, tmp.end(), *it))
	{
	    ++num_daily;
	    keep = true;
	}
	if (num_weekly < limit_weekly && is_first_weekly(it, tmp.end(), *it))
	{
	    ++num_weekly;
	    keep = true;
	}
	if (num_monthly < limit_monthly && is_first_monthly(it, tmp.end(), *it))
	{
	    ++num_monthly;
	    keep = true;
	}
	if (num_yearly < limit_yearly && is_first_yearly(it, tmp.end(), *it))
	{
	    ++num_yearly;
	    keep = true;
	}

	if (keep)
	    it = tmp.erase(it);
	else
	    ++it;
    }

    tmp.reverse();

    filter1(tmp, min_age);
    filter2(snapshots, tmp);

    for (list<XSnapshots::const_iterator>::const_iterator it = tmp.begin(); it != tmp.end(); ++it)
    {
	list<unsigned int> nums;
	nums.push_back((*it)->getNum());
	command_delete_xsnapshots(conn, config_name, nums, verbose);
    }

    return true;
}


bool
do_cleanup_empty_pre_post(DBus::Connection& conn, const string& config_name, bool verbose)
{
    time_t min_age = 1800;

    XConfigInfo ci = command_get_xconfig(conn, config_name);
    map<string, string>::const_iterator pos;
    if ((pos = ci.raw.find("EMPTY_PRE_POST_MIN_AGE")) != ci.raw.end())
	pos->second >> min_age;

    XSnapshots snapshots = command_list_xsnapshots(conn, config_name);

    list<XSnapshots::const_iterator> tmp;

    for (XSnapshots::const_iterator it1 = snapshots.begin(); it1 != snapshots.end(); ++it1)
    {
	if (it1->getType() == PRE)
	{
	    XSnapshots::const_iterator it2 = snapshots.findPost(it1);

	    if (it2 != snapshots.end())
	    {
		command_create_xcomparison(conn, config_name, it1->getNum(), it2->getNum());

		list<XFile> files = command_get_xfiles(conn, config_name, it1->getNum(), it2->getNum());

		if (files.empty())
		{
		    tmp.push_back(it1);
		    tmp.push_back(it2);
		}

		command_delete_xcomparison(conn, config_name, it1->getNum(), it2->getNum());
	    }
	}
    }

    filter1(tmp, min_age);
    filter2(snapshots, tmp);

    for (list<XSnapshots::const_iterator>::const_iterator it = tmp.begin(); it != tmp.end(); ++it)
    {
	list<unsigned int> nums;
	nums.push_back((*it)->getNum());
	command_delete_xsnapshots(conn, config_name, nums, verbose);
    }

    return true;
}
