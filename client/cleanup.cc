/*
 * Copyright (c) [2011-2014] Novell, Inc.
 * Copyright (c) [2016-2020] SUSE LLC
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


#include <iostream>
#include <vector>

#include "dbus/DBusMessage.h"
#include "dbus/DBusConnection.h"
#include "snapper/SnapperTmpl.h"
#include "snapper/Snapper.h"

#include "utils/Range.h"
#include "utils/equal-date.h"
#include "utils/HumanString.h"
#include "cleanup.h"


using namespace std;


struct Parameters
{
    Parameters(const ProxySnapper* snapper);
    virtual ~Parameters() {}

    virtual bool is_degenerated() const { return true; }

    time_t min_age;
    double space_limit;
    double free_limit;

    template<typename Type>
    void read(const ProxyConfig& config, const char* name, Type& value)
    {
	const map<string, string>& raw = config.getAllValues();
	map<string, string>::const_iterator pos = raw.find(name);
	if (pos != raw.end())
	    pos->second >> value;
    }
};


Parameters::Parameters(const ProxySnapper* snapper)
    : min_age(1800), space_limit(0.5), free_limit(0.2)
{
    ProxyConfig config = snapper->getConfig();

    read(config, "SPACE_LIMIT", space_limit);
    read(config, "FREE_LIMIT", free_limit);
}


ostream&
operator<<(ostream& s, const Parameters& parameters)
{
    return s << "min-age:" << parameters.min_age << '\n'
	     << "space-limit:" << parameters.space_limit << '\n'
	     << "free-limit:" << parameters.free_limit;
}


class Cleaner
{
public:

    Cleaner(ProxySnapper* snapper, bool verbose, const Parameters& parameters)
	: snapper(snapper), verbose(verbose), parameters(parameters) {}

    virtual ~Cleaner() {}

    void cleanup();
    void cleanup(std::function<bool()> condition);

protected:

    virtual list<ProxySnapshots::iterator> calculate_candidates(ProxySnapshots& snapshots,
								const Range::Value& value) = 0;

    struct younger_than
    {
	younger_than(time_t t)
	    : t(t) {}
	bool operator()(ProxySnapshots::const_iterator it)
	    { return it->getDate() > t; }
	const time_t t;
    };

    void filter(ProxySnapshots& snapshots, list<ProxySnapshots::iterator>& tmp) const;

    // Removes snapshots that cannot be removed (e.g. btrfs active and default)
    void filter_undeletables(ProxySnapshots& snapshots, list<ProxySnapshots::iterator>& tmp) const;

    // Removes snapshots younger than parameters.min_age from tmp
    void filter_min_age(ProxySnapshots& snapshots, list<ProxySnapshots::iterator>& tmp) const;

    // Removes pre and post snapshots from tmp that do have a corresponding
    // snapshot but which is not included in tmp.
    void filter_pre_post(ProxySnapshots& snapshots, list<ProxySnapshots::iterator>& tmp) const;

    void remove(const list<ProxySnapshots::iterator>& tmp);

    // Should the cleanup with quota space be run?
    bool is_quota_aware() const;

    // Is the quota space condition satisfied?
    bool is_quota_satisfied() const;

    // Should the cleanup with free space be run?
    bool is_free_aware() const;

    // Is the free space condition satisfied?
    bool is_free_satisfied() const;

    void cleanup(ProxySnapshots& snapshots);
    void cleanup(ProxySnapshots& snapshots, std::function<bool()> condition);

    ProxySnapper* snapper;

    const bool verbose;
    const Parameters& parameters;

};


void
Cleaner::filter(ProxySnapshots& snapshots, list<ProxySnapshots::iterator>& tmp) const
{
    filter_undeletables(snapshots, tmp);
    filter_min_age(snapshots, tmp);
    filter_pre_post(snapshots, tmp);
}


void
Cleaner::filter_undeletables(ProxySnapshots& snapshots, list<ProxySnapshots::iterator>& tmp) const
{
    vector<ProxySnapshots::const_iterator> undeletables;

    ProxySnapshots::const_iterator default_snapshot = snapshots.getDefault();
    if (default_snapshot != snapshots.end())
	undeletables.push_back(default_snapshot);

    ProxySnapshots::const_iterator active_snapshot = snapshots.getActive();
    if (active_snapshot != snapshots.end())
	undeletables.push_back(active_snapshot);

    for (ProxySnapshots::const_iterator undeletable : undeletables)
    {
	list<ProxySnapshots::iterator>::iterator keep = find_if(tmp.begin(), tmp.end(),
	    [undeletable](ProxySnapshots::iterator it){ return undeletable->getNum() == it->getNum(); });

	if (keep != tmp.end())
	    tmp.erase(keep);
    }
}


void
Cleaner::filter_min_age(ProxySnapshots& snapshots, list<ProxySnapshots::iterator>& tmp) const
{
    time_t now = time(NULL);
    tmp.remove_if(younger_than(now - parameters.min_age));
}


void
Cleaner::filter_pre_post(ProxySnapshots& snapshots, list<ProxySnapshots::iterator>& tmp) const
{
    list<ProxySnapshots::iterator> ret;

    for (list<ProxySnapshots::iterator>::iterator it1 = tmp.begin(); it1 != tmp.end(); ++it1)
    {
	if ((*it1)->getType() == PRE)
	{
	    ProxySnapshots::const_iterator it2 = snapshots.findPost(*it1);
	    if (it2 != snapshots.end())
	    {
		if (find(tmp.begin(), tmp.end(), it2) == tmp.end())
		    continue;
	    }
	}

	if ((*it1)->getType() == POST)
	{
	    ProxySnapshots::const_iterator it2 = snapshots.findPre(*it1);
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
Cleaner::remove(const list<ProxySnapshots::iterator>& tmp)
{
    for (list<ProxySnapshots::iterator>::const_iterator it = tmp.begin(); it != tmp.end(); ++it)
    {
	snapper->deleteSnapshots({ *it }, verbose);
    }
}


bool
Cleaner::is_quota_aware() const
{
    if (parameters.is_degenerated())
	return false;

    try
    {
	snapper->prepareQuota();
    }
    catch (const QuotaException& e)
    {
	SN_CAUGHT(e);

	cerr << "quota not working (" << e.what() << ")" << endl;
	return false;
    }

    return parameters.space_limit < 1.0;
}


bool
Cleaner::is_quota_satisfied() const
{
    QuotaData quota_data = snapper->queryQuotaData();

    if (quota_data.size == 0)
	return true;

    double fraction = (double)(quota_data.used) / (double)(quota_data.size);

    bool satisfied = fraction < parameters.space_limit;

#ifdef VERBOSE_LOGGING
    cout << byte_to_humanstring(quota_data.size, 2) << ", "
	 << byte_to_humanstring(quota_data.used, 2) << ", "
	 << fraction << ", " << satisfied << '\n';
#endif

    return satisfied;
}


bool
Cleaner::is_free_aware() const
{
    if (parameters.is_degenerated())
	return false;

    try
    {
	snapper->queryFreeSpaceData();
    }
    catch (const FreeSpaceException& e)
    {
	SN_CAUGHT(e);

	cerr << "free space not working (" << e.what() << ")" << endl;
	return false;
    }

    return parameters.free_limit > 0.0;
}


bool
Cleaner::is_free_satisfied() const
{
    FreeSpaceData free_space_data = snapper->queryFreeSpaceData();

    if (free_space_data.size == 0)
	return true;

    double fraction = (double)(free_space_data.free) / (double)(free_space_data.size);

    bool satisfied = fraction > parameters.free_limit;

#ifdef VERBOSE_LOGGING
    cout << byte_to_humanstring(free_space_data.size, 2) << ", "
	 << byte_to_humanstring(free_space_data.free, 2) << ", "
	 << fraction << ", " << satisfied << '\n';
#endif

    return satisfied;
}


void
Cleaner::cleanup(ProxySnapshots& snapshots)
{
    list<ProxySnapshots::iterator> candidates = calculate_candidates(snapshots, Range::MAX);

    filter(snapshots, candidates);

    remove(candidates);
}


void
Cleaner::cleanup(ProxySnapshots& snapshots, std::function<bool()> condition)
{
    while (!condition())
    {
	list<ProxySnapshots::iterator> candidates = calculate_candidates(snapshots, Range::MIN);
	if (candidates.empty())
	{
	    // not enough candidates to satisfy the condition

#ifdef VERBOSE_LOGGING
	    cout << "condition not satisfied" << '\n';
#endif

	    return;
	}

	// take more and more candidates into a temporary candidates
	// list. this is required since the filter will e.g. remove a pre
	// snapshot candidate if the post snapshot is missing so simply
	// removing the first candidate is not possible.

	for (list<ProxySnapshots::iterator>::iterator e = candidates.begin(); e != candidates.end(); ++e)
	{
	    list<ProxySnapshots::iterator> tmp = list<ProxySnapshots::iterator>(candidates.begin(), next(e));

	    filter(snapshots, tmp);

	    if (!tmp.empty())
	    {
		remove(tmp);

		// after removing snapshots the condition must be reevaluated
		break;
	    }

	    if (next(e) == candidates.end())
	    {
		// not enough candidates to satisfy the condition

#ifdef VERBOSE_LOGGING
		cout << "condition not satisfied" << '\n';
#endif

		return;
	    }
	}
    }

#ifdef VERBOSE_LOGGING
    cout << "condition satisfied" << '\n';
#endif
}


void
Cleaner::cleanup()
{
    ProxySnapshots& snapshots = snapper->getSnapshots();

#ifdef VERBOSE_LOGGING
    cout << "cleanup without condition" << '\n';
#endif

    cleanup(snapshots);

    if (is_quota_aware())
    {
#ifdef VERBOSE_LOGGING
	cout << "cleanup with quota condition" << '\n';
#endif

	cleanup(snapshots, std::bind(&Cleaner::is_quota_satisfied, this));
    }
    else
    {
#ifdef VERBOSE_LOGGING
	cout << "no cleanup with quota condition" << '\n';
#endif
    }

    if (is_free_aware())
    {
#ifdef VERBOSE_LOGGING
	cout << "cleanup with free condition" << '\n';
#endif

	cleanup(snapshots, std::bind(&Cleaner::is_free_satisfied, this));
    }
    else
    {
#ifdef VERBOSE_LOGGING
	cout << "no cleanup with free condition" << '\n';
#endif
    }
}


void
Cleaner::cleanup(std::function<bool()> condition)
{
    ProxySnapshots& snapshots = snapper->getSnapshots();

#ifdef VERBOSE_LOGGING
    cout << "cleanup with user condition" << '\n';
#endif

    cleanup(snapshots, condition);
}


struct NumberParameters : public Parameters
{
    NumberParameters(const ProxySnapper* snapper);

    bool is_degenerated() const override;

    Range limit;
    Range limit_important;
};


NumberParameters::NumberParameters(const ProxySnapper* snapper)
    : Parameters(snapper), limit(50), limit_important(10)
{
    ProxyConfig config = snapper->getConfig();

    read(config, "NUMBER_MIN_AGE", min_age);

    read(config, "NUMBER_LIMIT", limit);
    read(config, "NUMBER_LIMIT_IMPORTANT", limit_important);
}


ostream&
operator<<(ostream& s, const NumberParameters& parameters)
{
    return s << dynamic_cast<const Parameters&>(parameters) << '\n'
	     << "limit:" << parameters.limit << '\n'
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

    NumberCleaner(ProxySnapper* snapper, bool verbose, const NumberParameters& parameters)
	: Cleaner(snapper, verbose, parameters) {}

private:

    bool
    is_important(ProxySnapshots::const_iterator it1)
    {
	map<string, string>::const_iterator it2 = it1->getUserdata().find("important");
	return it2 != it1->getUserdata().end() && it2->second == "yes";
    }


    list<ProxySnapshots::iterator>
    calculate_candidates(ProxySnapshots& snapshots, const Range::Value& value) override
    {
	const NumberParameters& parameters = dynamic_cast<const NumberParameters&>(Cleaner::parameters);

	list<ProxySnapshots::iterator> ret;

	for (ProxySnapshots::iterator it = snapshots.begin(); it != snapshots.end(); ++it)
	{
	    if (it->getCleanup() == "number")
		ret.push_front(it);
	}

	size_t num = 0;
	size_t num_important = 0;

	list<ProxySnapshots::iterator>::iterator it = ret.begin();
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
do_cleanup_number(ProxySnapper* snapper, bool verbose)
{
    NumberParameters parameters(snapper);
    NumberCleaner cleaner(snapper, verbose, parameters);
    cleaner.cleanup();
}


void
do_cleanup_number(ProxySnapper* snapper, bool verbose, std::function<bool()> condition)
{
    NumberParameters parameters(snapper);
    NumberCleaner cleaner(snapper, verbose, parameters);
    cleaner.cleanup(condition);
}


struct TimelineParameters : public Parameters
{
    TimelineParameters(const ProxySnapper* snapper);

    bool is_degenerated() const override;

    Range limit_hourly;
    Range limit_daily;
    Range limit_monthly;
    Range limit_weekly;
    Range limit_yearly;
};


TimelineParameters::TimelineParameters(const ProxySnapper* snapper)
    : Parameters(snapper), limit_hourly(10), limit_daily(10), limit_monthly(10),
      limit_weekly(0), limit_yearly(10)
{
    ProxyConfig config = snapper->getConfig();

    read(config, "TIMELINE_MIN_AGE", min_age);

    read(config, "TIMELINE_LIMIT_HOURLY", limit_hourly);
    read(config, "TIMELINE_LIMIT_DAILY", limit_daily);
    read(config, "TIMELINE_LIMIT_WEEKLY", limit_weekly);
    read(config, "TIMELINE_LIMIT_MONTHLY", limit_monthly);
    read(config, "TIMELINE_LIMIT_YEARLY", limit_yearly);
}


ostream&
operator<<(ostream& s, const TimelineParameters& parameters)
{
    return s << dynamic_cast<const Parameters&>(parameters) << '\n'
	     << "limit-hourly:" << parameters.limit_hourly << '\n'
	     << "limit-daily:" << parameters.limit_daily << '\n'
	     << "limit-weekly:" << parameters.limit_weekly << '\n'
	     << "limit-monthly:" << parameters.limit_monthly << '\n'
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

    TimelineCleaner(ProxySnapper* snapper, bool verbose, const TimelineParameters& parameters)
	: Cleaner(snapper, verbose, parameters) {}

private:

    bool
    is_first(list<ProxySnapshots::iterator>::const_iterator first,
	     list<ProxySnapshots::iterator>::const_iterator last,
	     ProxySnapshots::const_iterator it1,
	     std::function<bool(const struct tm& tmp1, const struct tm& tmp2)> pred)
    {
	time_t t1 = it1->getDate();
	struct tm tmp1;
	localtime_r(&t1, &tmp1);

	for (list<ProxySnapshots::iterator>::const_iterator it2 = first; it2 != last; ++it2)
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
    is_first_yearly(list<ProxySnapshots::iterator>::const_iterator first,
		    list<ProxySnapshots::iterator>::const_iterator last,
		    ProxySnapshots::const_iterator it1)
    {
	return is_first(first, last, it1, equal_year);
    }

    bool
    is_first_monthly(list<ProxySnapshots::iterator>::const_iterator first,
		     list<ProxySnapshots::iterator>::const_iterator last,
		     ProxySnapshots::const_iterator it1)
    {
	return is_first(first, last, it1, equal_month);
    }

    bool
    is_first_weekly(list<ProxySnapshots::iterator>::const_iterator first,
		    list<ProxySnapshots::iterator>::const_iterator last,
		    ProxySnapshots::const_iterator it1)
    {
	return is_first(first, last, it1, equal_week);
    }

    bool
    is_first_daily(list<ProxySnapshots::iterator>::const_iterator first,
		   list<ProxySnapshots::iterator>::const_iterator last,
		   ProxySnapshots::const_iterator it1)
    {
	return is_first(first, last, it1, equal_day);
    }

    bool
    is_first_hourly(list<ProxySnapshots::iterator>::const_iterator first,
		    list<ProxySnapshots::iterator>::const_iterator last,
		    ProxySnapshots::const_iterator it1)
    {
	return is_first(first, last, it1, equal_hour);
    }


    list<ProxySnapshots::iterator>
    calculate_candidates(ProxySnapshots& snapshots, const Range::Value& value) override
    {
	const TimelineParameters& parameters = dynamic_cast<const TimelineParameters&>(Cleaner::parameters);

	list<ProxySnapshots::iterator> ret;

	for (ProxySnapshots::iterator it = snapshots.begin(); it != snapshots.end(); ++it)
	{
	    if (it->getCleanup() == "timeline")
		ret.push_front(it);
	}

	size_t num_hourly = 0;
	size_t num_daily = 0;
	size_t num_weekly = 0;
	size_t num_monthly = 0;
	size_t num_yearly = 0;

	list<ProxySnapshots::iterator>::iterator it = ret.begin();
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
do_cleanup_timeline(ProxySnapper* snapper, bool verbose)
{
    TimelineParameters parameters(snapper);
    TimelineCleaner cleaner(snapper, verbose, parameters);
    cleaner.cleanup();
}


void
do_cleanup_timeline(ProxySnapper* snapper, bool verbose, std::function<bool()> condition)
{
    TimelineParameters parameters(snapper);
    TimelineCleaner cleaner(snapper, verbose, parameters);
    cleaner.cleanup(condition);
}


struct EmptyPrePostParameters : public Parameters
{
    EmptyPrePostParameters(const ProxySnapper* snapper);
};


EmptyPrePostParameters::EmptyPrePostParameters(const ProxySnapper* snapper)
    : Parameters(snapper)
{
    ProxyConfig config = snapper->getConfig();

    read(config, "EMPTY_PRE_POST_MIN_AGE", min_age);
}


class EmptyPrePostCleaner : public Cleaner
{
public:

    EmptyPrePostCleaner(ProxySnapper* snapper, bool verbose,
			const EmptyPrePostParameters& parameters)
	: Cleaner(snapper, verbose, parameters) {}

private:

    list<ProxySnapshots::iterator>
    calculate_candidates(ProxySnapshots& snapshots, const Range::Value& value) override
    {
	list<ProxySnapshots::iterator> ret;

	for (ProxySnapshots::iterator it1 = snapshots.begin(); it1 != snapshots.end(); ++it1)
	{
	    if (it1->getType() == PRE)
	    {
		ProxySnapshots::iterator it2 = snapshots.findPost(it1);
		if (it2 != snapshots.end())
		{
		    ProxyComparison comparison = snapper->createComparison(*it1, *it2, false);
		    if (comparison.getFiles().empty())
		    {
			ret.push_back(it1);
			ret.push_back(it2);
		    }
		}
	    }
	}

	return ret;
    }
};


void
do_cleanup_empty_pre_post(ProxySnapper* snapper, bool verbose)
{
    EmptyPrePostParameters parameters(snapper);
    EmptyPrePostCleaner cleaner(snapper, verbose, parameters);
    cleaner.cleanup();
}


void
do_cleanup_empty_pre_post(ProxySnapper* snapper, bool verbose, std::function<bool()> condition)
{
    EmptyPrePostParameters parameters(snapper);
    EmptyPrePostCleaner cleaner(snapper, verbose, parameters);
    cleaner.cleanup(condition);
}
