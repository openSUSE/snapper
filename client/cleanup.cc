/*
 * Copyright (c) [2011-2014] Novell, Inc.
 * Copyright (c) 2016 SUSE LLC
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


#include "utils/Range.h"
#include "utils/equal-date.h"
#include "commands.h"

#include <vector>

using namespace std;


struct Parameters
{
    Parameters(DBus::Connection& conn, const string& config_name);
    virtual ~Parameters() {}

    virtual bool is_degenerated() const { return true; }

    friend ostream& operator<<(ostream& s, const Parameters& parameters);

    time_t min_age;
    double space_limit;
};


Parameters::Parameters(DBus::Connection& conn, const string& config_name)
    : min_age(1800), space_limit(0.5)
{
    XConfigInfo ci = command_get_xconfig(conn, config_name);

    ci.read("SPACE_LIMIT", space_limit);
}


ostream&
operator<<(ostream& s, const Parameters& parameters)
{
    return s << "min-age:" << parameters.min_age << endl
	     << "space-limit:" << parameters.space_limit;
}


class Cleaner
{
public:

    Cleaner(DBus::Connection& conn, const string& config_name, bool verbose, const Parameters& parameters)
	: conn(conn), config_name(config_name), verbose(verbose), parameters(parameters) {}

    virtual ~Cleaner() {}

    void cleanup();

protected:

    virtual list<XSnapshots::iterator> calculate_candidates(Range::Value value) = 0;

    struct younger_than
    {
	younger_than(time_t t)
	    : t(t) {}
	bool operator()(XSnapshots::const_iterator it)
	    { return it->getDate() > t; }
	const time_t t;
    };

    void filter(list<XSnapshots::iterator>& tmp) const;

    // Removes snapshots younger than parameters.min_age from tmp
    void filter_min_age(list<XSnapshots::iterator>& tmp) const;

    // Removes pre and post snapshots from tmp that do have a corresponding
    // snapshot but which is not included in tmp.
    void filter_pre_post(list<XSnapshots::iterator>& tmp) const;

    void remove(const list<XSnapshots::iterator>& tmp);

    bool is_quota_aware() const;
    bool is_quota_satisfied() const;

    void cleanup_quota_unaware();
    void cleanup_quota_aware();

    DBus::Connection& conn;
    const string& config_name;
    bool verbose;

    const Parameters& parameters;

    XSnapshots snapshots;
};


void
Cleaner::filter(list<XSnapshots::iterator>& tmp) const
{
    filter_min_age(tmp);
    filter_pre_post(tmp);
}


void
Cleaner::filter_min_age(list<XSnapshots::iterator>& tmp) const
{
    time_t now = time(NULL);
    tmp.remove_if(younger_than(now - parameters.min_age));
}


void
Cleaner::filter_pre_post(list<XSnapshots::iterator>& tmp) const
{
    list<XSnapshots::iterator> ret;

    for (list<XSnapshots::iterator>::iterator it1 = tmp.begin(); it1 != tmp.end(); ++it1)
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


void
Cleaner::remove(const list<XSnapshots::iterator>& tmp)
{
    for (list<XSnapshots::iterator>::const_iterator it = tmp.begin(); it != tmp.end(); ++it)
    {
	command_delete_xsnapshots(conn, config_name, { (*it)->getNum() }, verbose);
	snapshots.erase(*it);
    }
}


bool
Cleaner::is_quota_aware() const
{
    if (parameters.is_degenerated())
	return false;

    try
    {
	command_prepare_quota(conn, config_name);
    }
    catch (const DBus::ErrorException& e)
    {
	SN_CAUGHT(e);

	if (strcmp(e.name(), "error.quota") == 0)
	{
	    cerr << "quota not working" << endl;
	    return false;
	}

	SN_RETHROW(e);
    }

    return true;
}


bool
Cleaner::is_quota_satisfied() const
{
    XQuotaData quota_data = command_query_quota(conn, config_name);

    return quota_data.used < parameters.space_limit * quota_data.size;
}


void
Cleaner::cleanup_quota_unaware()
{
    list<XSnapshots::iterator> candidates = calculate_candidates(Range::MAX);

    filter(candidates);

    remove(candidates);
}


void
Cleaner::cleanup_quota_aware()
{
    while (!is_quota_satisfied())
    {
	list<XSnapshots::iterator> candidates = calculate_candidates(Range::MIN);
	if (candidates.empty())
	{
	    // not enough candidates to satisfy quota
	    return;
	}

	// take more and more candidates into a temporary candidates
	// list. this is required since the filter will e.g. remove a pre
	// snapshot candidate if the post snapshot is missing so simply
	// removing the first candidate is not possible.

	for (list<XSnapshots::iterator>::iterator e = candidates.begin(); e != candidates.end(); ++e)
	{
	    list<XSnapshots::iterator> tmp = list<XSnapshots::iterator>(candidates.begin(), next(e));

	    filter(tmp);

	    if (!tmp.empty())
	    {
		remove(tmp);
		// after removing snapshots is_quota_satisfied must be reevaluated
		break;
	    }

	    if (next(e) == candidates.end())
	    {
		// not enough candidates to satisfy quota
		return;
	    }
	}
    }
}


void
Cleaner::cleanup()
{
    snapshots = command_list_xsnapshots(conn, config_name);

    cleanup_quota_unaware();

    if (is_quota_aware())
	cleanup_quota_aware();
}


struct NumberParameters : public Parameters
{
    NumberParameters(DBus::Connection& conn, const string& config_name);

    bool is_degenerated() const;

    friend ostream& operator<<(ostream& s, const NumberParameters& parameters);

    Range limit;
    Range limit_important;
};


NumberParameters::NumberParameters(DBus::Connection& conn, const string& config_name)
    : Parameters(conn, config_name), limit(50), limit_important(10)
{
    XConfigInfo ci = command_get_xconfig(conn, config_name);

    ci.read("NUMBER_MIN_AGE", min_age);

    ci.read("NUMBER_LIMIT", limit);
    ci.read("NUMBER_LIMIT_IMPORTANT", limit_important);
}


ostream&
operator<<(ostream& s, const NumberParameters& parameters)
{
    return s << dynamic_cast<const Parameters&>(parameters) << endl
	     << "limit:" << parameters.limit << endl
	     << "limit-important:" << parameters.limit_important;
}


bool
NumberParameters::is_degenerated() const
{
    return limit.is_degenerated() && limit_important.is_degenerated();
}


class NumberCleaner : public Cleaner
{

public:

    NumberCleaner(DBus::Connection& conn, const string& config_name, bool verbose,
		  const NumberParameters& parameters)
	: Cleaner(conn, config_name, verbose, parameters) {}

private:

    bool
    is_important(XSnapshots::const_iterator it1)
    {
	map<string, string>::const_iterator it2 = it1->getUserdata().find("important");
	return it2 != it1->getUserdata().end() && it2->second == "yes";
    }


    list<XSnapshots::iterator>
    calculate_candidates(Range::Value value)
    {
	const NumberParameters& parameters = dynamic_cast<const NumberParameters&>(Cleaner::parameters);

	list<XSnapshots::iterator> ret;

	for (XSnapshots::iterator it = snapshots.begin(); it != snapshots.end(); ++it)
	{
	    if (it->getCleanup() == "number")
		ret.push_front(it);
	}

	size_t num = 0;
	size_t num_important = 0;

	list<XSnapshots::iterator>::iterator it = ret.begin();
	while (it != ret.end())
	{
	    bool keep = false;

	    if (num_important < parameters.limit_important.value(value) && is_important(*it))
	    {
		++num_important;
		keep = true;
	    }
	    if (num < parameters.limit.value(value))
	    {
		++num;
		keep = true;
	    }

	    if (keep)
		it = ret.erase(it);
	    else
		++it;
	}

	ret.reverse();

	return ret;
    }
};


void
do_cleanup_number(DBus::Connection& conn, const string& config_name, bool verbose)
{
    NumberParameters parameters(conn, config_name);
    NumberCleaner cleaner(conn, config_name, verbose, parameters);
    cleaner.cleanup();
}


struct TimelineParameters : public Parameters
{
    TimelineParameters(DBus::Connection& conn, const string& config_name);

    bool is_degenerated() const;

    Range limit_hourly;
    Range limit_daily;
    Range limit_monthly;
    Range limit_weekly;
    Range limit_yearly;
};


TimelineParameters::TimelineParameters(DBus::Connection& conn, const string& config_name)
    : Parameters(conn, config_name), limit_hourly(10), limit_daily(10), limit_monthly(10),
      limit_weekly(0), limit_yearly(10)
{
    XConfigInfo ci = command_get_xconfig(conn, config_name);

    ci.read("TIMELINE_MIN_AGE", min_age);

    ci.read("TIMELINE_LIMIT_HOURLY", limit_hourly);
    ci.read("TIMELINE_LIMIT_DAILY", limit_daily);
    ci.read("TIMELINE_LIMIT_WEEKLY", limit_weekly);
    ci.read("TIMELINE_LIMIT_MONTHLY", limit_monthly);
    ci.read("TIMELINE_LIMIT_YEARLY", limit_yearly);
}


ostream&
operator<<(ostream& s, const TimelineParameters& parameters)
{
    return s << dynamic_cast<const Parameters&>(parameters) << endl
	     << "limit-hourly:" << parameters.limit_hourly << endl
	     << "limit-daily:" << parameters.limit_daily << endl
	     << "limit-weekly:" << parameters.limit_weekly << endl
	     << "limit-monthly:" << parameters.limit_monthly << endl
	     << "limit-yearly:" << parameters.limit_yearly;
}


bool
TimelineParameters::is_degenerated() const
{
    return limit_hourly.is_degenerated() && limit_daily.is_degenerated() &&
	limit_monthly.is_degenerated() && limit_weekly.is_degenerated() &&
	limit_yearly.is_degenerated();
}


class TimelineCleaner : public Cleaner
{
public:

    TimelineCleaner(DBus::Connection& conn, const string& config_name, bool verbose,
		    const TimelineParameters& parameters)
	: Cleaner(conn, config_name, verbose, parameters) {}

private:

    bool
    is_first(list<XSnapshots::iterator>::const_iterator first,
	     list<XSnapshots::iterator>::const_iterator last,
	     XSnapshots::const_iterator it1,
	     std::function<bool(const struct tm& tmp1, const struct tm& tmp2)> pred)
    {
	time_t t1 = it1->getDate();
	struct tm tmp1;
	localtime_r(&t1, &tmp1);

	for (list<XSnapshots::iterator>::const_iterator it2 = first; it2 != last; ++it2)
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
    is_first_yearly(list<XSnapshots::iterator>::const_iterator first,
		    list<XSnapshots::iterator>::const_iterator last,
		    XSnapshots::const_iterator it1)
    {
	return is_first(first, last, it1, equal_year);
    }

    bool
    is_first_monthly(list<XSnapshots::iterator>::const_iterator first,
		     list<XSnapshots::iterator>::const_iterator last,
		     XSnapshots::const_iterator it1)
    {
	return is_first(first, last, it1, equal_month);
    }

    bool
    is_first_weekly(list<XSnapshots::iterator>::const_iterator first,
		    list<XSnapshots::iterator>::const_iterator last,
		    XSnapshots::const_iterator it1)
    {
	return is_first(first, last, it1, equal_week);
    }

    bool
    is_first_daily(list<XSnapshots::iterator>::const_iterator first,
		   list<XSnapshots::iterator>::const_iterator last,
		   XSnapshots::const_iterator it1)
    {
	return is_first(first, last, it1, equal_day);
    }

    bool
    is_first_hourly(list<XSnapshots::iterator>::const_iterator first,
		    list<XSnapshots::iterator>::const_iterator last,
		    XSnapshots::const_iterator it1)
    {
	return is_first(first, last, it1, equal_hour);
    }


    list<XSnapshots::iterator>
    calculate_candidates(Range::Value value)
    {
	const TimelineParameters& parameters = dynamic_cast<const TimelineParameters&>(Cleaner::parameters);

	list<XSnapshots::iterator> ret;

	for (XSnapshots::iterator it = snapshots.begin(); it != snapshots.end(); ++it)
	{
	    if (it->getCleanup() == "timeline")
		ret.push_front(it);
	}

	size_t num_hourly = 0;
	size_t num_daily = 0;
	size_t num_weekly = 0;
	size_t num_monthly = 0;
	size_t num_yearly = 0;

	list<XSnapshots::iterator>::iterator it = ret.begin();
	while (it != ret.end())
	{
	    bool keep = false;

	    if (num_hourly < parameters.limit_hourly.value(value) && is_first_hourly(it, ret.end(), *it))
	    {
		++num_hourly;
		keep = true;
	    }
	    if (num_daily < parameters.limit_daily.value(value) && is_first_daily(it, ret.end(), *it))
	    {
		++num_daily;
		keep = true;
	    }
	    if (num_weekly < parameters.limit_weekly.value(value) && is_first_weekly(it, ret.end(), *it))
	    {
		++num_weekly;
		keep = true;
	    }
	    if (num_monthly < parameters.limit_monthly.value(value) && is_first_monthly(it, ret.end(), *it))
	    {
		++num_monthly;
		keep = true;
	    }
	    if (num_yearly < parameters.limit_yearly.value(value) && is_first_yearly(it, ret.end(), *it))
	    {
		++num_yearly;
		keep = true;
	    }

	    if (keep)
		it = ret.erase(it);
	    else
		++it;
	}

	ret.reverse();

	return ret;
    }
};


void
do_cleanup_timeline(DBus::Connection& conn, const string& config_name, bool verbose)
{
    TimelineParameters parameters(conn, config_name);
    TimelineCleaner cleaner(conn, config_name, verbose, parameters);
    cleaner.cleanup();
}


struct EmptyPrePostParameters : public Parameters
{
    EmptyPrePostParameters(DBus::Connection& conn, const string& config_name);
};


EmptyPrePostParameters::EmptyPrePostParameters(DBus::Connection& conn, const string& config_name)
    : Parameters(conn, config_name)
{
    XConfigInfo ci = command_get_xconfig(conn, config_name);

    ci.read("EMPTY_PRE_POST_MIN_AGE", min_age);
}


class EmptyPrePostCleaner : public Cleaner
{
public:

    EmptyPrePostCleaner(DBus::Connection& conn, const string& config_name, bool verbose,
			const EmptyPrePostParameters& parameters)
	: Cleaner(conn, config_name, verbose, parameters) {}

private:

    list<XSnapshots::iterator>
    calculate_candidates(Range::Value value)
    {
	list<XSnapshots::iterator> ret;

	for (XSnapshots::iterator it1 = snapshots.begin(); it1 != snapshots.end(); ++it1)
	{
	    if (it1->getType() == PRE)
	    {
		XSnapshots::iterator it2 = snapshots.findPost(it1);

		if (it2 != snapshots.end())
		{
		    command_create_xcomparison(conn, config_name, it1->getNum(), it2->getNum());

		    list<XFile> files = command_get_xfiles(conn, config_name, it1->getNum(), it2->getNum());

		    if (files.empty())
		    {
			ret.push_back(it1);
			ret.push_back(it2);
		    }

		    command_delete_xcomparison(conn, config_name, it1->getNum(), it2->getNum());
		}
	    }
	}

	return ret;
    }
};


void
do_cleanup_empty_pre_post(DBus::Connection& conn, const string& config_name, bool verbose)
{
    EmptyPrePostParameters parameters(conn, config_name);
    EmptyPrePostCleaner cleaner(conn, config_name, verbose, parameters);
    cleaner.cleanup();
}
