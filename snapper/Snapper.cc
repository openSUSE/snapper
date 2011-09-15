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
#include <mntent.h>
#include <boost/algorithm/string.hpp>

#include "config.h"
#include "snapper/Snapper.h"
#include "snapper/Comparison.h"
#include "snapper/AppUtil.h"
#include "snapper/Enum.h"
#include "snapper/Filesystem.h"
#include "snapper/SnapperTmpl.h"
#include "snapper/SystemCmd.h"
#include "snapper/SnapperDefines.h"
#include "snapper/File.h"
#include "snapper/AsciiFile.h"
#include "snapper/Exception.h"


namespace snapper
{
    using namespace std;


    Snapper::Snapper(const string& config_name, bool disable_filters)
	: config_name(config_name), config(NULL), subvolume("/"), filesystem(NULL),
	  snapshots(this), compare_callback(NULL), undo_callback(NULL)
    {
	y2mil("Snapper constructor");
	y2mil("libsnapper version " VERSION);
	y2mil("config_name:" << config_name << " disable_filters:" << disable_filters);

	try
	{
	    config = new SysconfigFile(CONFIGSDIR "/" + config_name);
	}
	catch (const FileNotFoundException& e)
	{
	    throw ConfigNotFoundException();
	}

	if (!config->getValue("SUBVOLUME", subvolume))
	    throw InvalidConfigException();

	string fstype = "btrfs";
	config->getValue("FSTYPE", fstype);
	filesystem = Filesystem::create(fstype, subvolume);

	y2mil("subvolume:" << subvolume << " filesystem:" << filesystem->name());

	if (!disable_filters)
	    loadIgnorePatterns();

	snapshots.initialize();
    }


    Snapper::~Snapper()
    {
	y2mil("Snapper destructor");

	for (Snapshots::iterator it = snapshots.begin(); it != snapshots.end(); ++it)
	    it->flushInfo();

	delete filesystem;
	delete config;
    }


    void
    Snapper::loadIgnorePatterns()
    {
	const list<string> files = glob(FILTERSDIR "/*.txt", GLOB_NOSORT);
	for (list<string>::const_iterator it = files.begin(); it != files.end(); ++it)
	{
	    try
	    {
		AsciiFileReader asciifile(*it);

		string line;
		while (asciifile.getline(line))
		    ignore_patterns.push_back(line);
	    }
	    catch (const FileNotFoundException& e)
	    {
	    }
	}

	y2mil("number of ignore patterns:" << ignore_patterns.size());
    }


    // Directory of which snapshots are made, e.g. "/" or "/home".
    string
    Snapper::subvolumeDir() const
    {
	return subvolume;
    }


    // Directory that contains the per snapshot directory with info files.
    // For btrfs e.g. "/.snapshots" or "/home/.snapshots".
    // For ext4 e.g. "/.snapshots-info" or "/home/.snapshots-info".
    string
    Snapper::infosDir() const
    {
	return filesystem->infosDir();
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
	return snapshots.createPostSnapshot(pre);
    }


    void
    Snapper::deleteSnapshot(Snapshots::iterator snapshot)
    {
	snapshots.deleteSnapshot(snapshot);
    }


    void
    Snapper::startBackgroundComparsion(Snapshots::const_iterator snapshot1,
				       Snapshots::const_iterator snapshot2)
    {
	if (snapshot1 == snapshots.end() || snapshot1->isCurrent())
	    throw IllegalSnapshotException();

	if (snapshot2 == snapshots.end() || snapshot2->isCurrent())
	    throw IllegalSnapshotException();

	y2mil("num1:" << snapshot1->getNum() << " num2:" << snapshot2->getNum());

	snapshot1->mountFilesystemSnapshot();
	snapshot2->mountFilesystemSnapshot();

	bool invert = snapshot1->getNum() > snapshot2->getNum();

	if (invert)
	    swap(snapshot1, snapshot2);

	string dir1 = snapshot1->snapshotDir();
	string dir2 = snapshot2->snapshotDir();

	string output = snapshot2->infoDir() + "/filelist-" + decString(snapshot1->getNum()) +
	    ".txt";

	SystemCmd(NICEBIN " -n 19 " IONICEBIN " -c 3 " COMPAREDIRSBIN " " + quote(dir1) + " " +
		  quote(dir2) + " " + quote(output));
    }


    struct younger_than
    {
	younger_than(time_t t)
	    : t(t) {}
	bool operator()(Snapshots::const_iterator it)
	    { return it->getDate() > t; }
	const time_t t;
    };


    // Removes snapshots younger than min_age from tmp
    void
    Snapper::filter1(list<Snapshots::iterator>& tmp, time_t min_age)
    {
	tmp.remove_if(younger_than(time(NULL) - min_age));
    }


    // Removes pre and post snapshots from tmp that do have a corresponding
    // snapshot but which is not included in tmp.
    void
    Snapper::filter2(list<Snapshots::iterator>& tmp)
    {
	list<Snapshots::iterator> ret;

	for (list<Snapshots::iterator>::const_iterator it1 = tmp.begin(); it1 != tmp.end(); ++it1)
	{
	    if ((*it1)->getType() == PRE)
	    {
		Snapshots::const_iterator it2 = snapshots.findPost(*it1);
		if (it2 != snapshots.end())
		{
		    if (find(tmp.begin(), tmp.end(), it2) == tmp.end())
			continue;
		}
	    }

	    if ((*it1)->getType() == POST)
	    {
		Snapshots::const_iterator it2 = snapshots.findPre(*it1);
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
    Snapper::doCleanupNumber()
    {
	time_t min_age = 1800;
	size_t limit = 50;

	string val;
	if (config->getValue("NUMBER_MIN_AGE", val))
	    val >> min_age;
	if (config->getValue("NUMBER_LIMIT", val))
	    val >> limit;

	y2mil("min_age:" << min_age << " limit:" << limit);

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

	    filter1(tmp, min_age);
	    filter2(tmp);

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
	time_t min_age = 1800;
	size_t limit_hourly = 10;
	size_t limit_daily = 10;
	size_t limit_monthly = 10;
	size_t limit_yearly = 10;

	string val;
	if (config->getValue("TIMELINE_MIN_AGE", val))
	    val >> min_age;
	if (config->getValue("TIMELINE_LIMIT_HOURLY", val))
	    val >> limit_hourly;
	if (config->getValue("TIMELINE_LIMIT_DAILY", val))
	    val >> limit_daily;
	if (config->getValue("TIMELINE_LIMIT_MONTHLY", val))
	    val >> limit_monthly;
	if (config->getValue("TIMELINE_LIMIT_YEARLY", val))
	    val >> limit_yearly;

	y2mil("min_age:" << min_age <<" limit_hourly:" << limit_hourly << " limit_daily:" <<
	      limit_daily << " limit_monthly:" << limit_monthly << " limit_yearly:" <<
	      limit_yearly);

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

	filter1(tmp, min_age);
	filter2(tmp);

	y2mil("deleting " << tmp.size() << " snapshots");

	for (list<Snapshots::iterator>::iterator it = tmp.begin(); it != tmp.end(); ++it)
	    deleteSnapshot(*it);

	return true;
    }


    bool
    Snapper::doCleanupEmptyPrePost()
    {
	time_t min_age = 1800;

	string val;
	if (config->getValue("EMPTY_PRE_POST_MIN_AGE", val))
	    val >> min_age;

	y2mil("min_age:" << min_age);

	list<Snapshots::iterator> tmp;

	for (Snapshots::iterator it1 = snapshots.begin(); it1 != snapshots.end(); ++it1)
	{
	    if (it1->getType() == PRE)
	    {
		Snapshots::iterator it2 = snapshots.findPost(it1);

		if (it2 != snapshots.end())
		{
		    Comparison comparison(this, it1, it2);
		    if (comparison.getFiles().empty())
		    {
			tmp.push_back(it1);
			tmp.push_back(it2);
		    }
		}
	    }
	}

	filter1(tmp, min_age);
	filter2(tmp);

	y2mil("deleting " << tmp.size() << " snapshots");

	for (list<Snapshots::iterator>::iterator it = tmp.begin(); it != tmp.end(); ++it)
	    deleteSnapshot(*it);

	return true;
    }


    list<ConfigInfo>
    Snapper::getConfigs()
    {
	y2mil("Snapper get-configs");
	y2mil("libsnapper version " VERSION);

	list<ConfigInfo> config_infos;

	try
	{
	    SysconfigFile sysconfig(SYSCONFIGFILE);
	    vector<string> config_names;
	    sysconfig.getValue("SNAPPER_CONFIGS", config_names);

	    for (vector<string>::const_iterator it = config_names.begin(); it != config_names.end(); ++it)
	    {
		try
		{
		    SysconfigFile config(CONFIGSDIR "/" + *it);

		    string subvolume = "/";
		    config.getValue("SUBVOLUME", subvolume);
		    config_infos.push_back(ConfigInfo(*it, subvolume));
		}
		catch (const FileNotFoundException& e)
		{
		    y2err("config '" << *it << "' not found");
		}
	    }
	}
	catch (const FileNotFoundException& e)
	{
	    throw ListConfigsFailedException("sysconfig file not found");
	}

	return config_infos;
    }


    void
    Snapper::addConfig(const string& config_name, const string& subvolume,
		       const string& fstype, const string& template_name)
    {
	y2mil("Snapper add-config");
	y2mil("libsnapper version " VERSION);
	y2mil("config_name:" << config_name << " subvolume:" << subvolume <<
	      " fstype:" << fstype << " template_name:" << template_name);

	if (config_name.empty() || config_name.find_first_of(" \t") != string::npos)
	{
	    throw AddConfigFailedException("illegal config name");
	}

	if (!boost::starts_with(subvolume, "/") || !checkDir(subvolume))
	{
	    throw AddConfigFailedException("illegal subvolume");
	}

	if (access(string(CONFIGTEMPLATEDIR "/" + template_name).c_str(), R_OK) != 0)
	{
	    throw AddConfigFailedException("cannot access template config");
	}

	auto_ptr<Filesystem> filesystem;
	try
	{
	    filesystem.reset(Filesystem::create(fstype, subvolume));
	}
	catch (const InvalidConfigException& e)
	{
	    throw AddConfigFailedException("invalid filesystem type");
	}

	try
	{
	    SysconfigFile sysconfig(SYSCONFIGFILE);
	    vector<string> config_names;
	    sysconfig.getValue("SNAPPER_CONFIGS", config_names);
	    if (find(config_names.begin(), config_names.end(), config_name) != config_names.end())
	    {
		throw AddConfigFailedException("config already exists");
	    }

	    config_names.push_back(config_name);
	    sysconfig.setValue("SNAPPER_CONFIGS", config_names);
	}
	catch (const FileNotFoundException& e)
	{
	    throw AddConfigFailedException("sysconfig file not found");
	}

	SystemCmd cmd1(CPBIN " " + quote(CONFIGTEMPLATEDIR "/" + template_name) + " " +
		       quote(CONFIGSDIR "/" + config_name));
	if (cmd1.retcode() != 0)
	{
	    throw AddConfigFailedException("copying config template failed");
	}

	try
	{
	    SysconfigFile config(CONFIGSDIR "/" + config_name);
	    config.setValue("SUBVOLUME", subvolume);
	    config.setValue("FSTYPE", fstype);
	}
	catch (const FileNotFoundException& e)
	{
	    throw AddConfigFailedException("modifying config failed");
	}

	filesystem->addConfig();
    }


    static bool
    is_subpath(const string& a, const string& b)
    {
	if (b == "/")
	    return true;

	size_t len = b.length();

	if (len > a.length())
	    return false;

	return (len == a.length() || a[len] == '/') && a.compare(0, len, b) == 0;
    }


    bool
    Snapper::detectFstype(const string& subvolume, string& fstype)
    {
	y2mil("subvolume:" << subvolume);

	if (!boost::starts_with(subvolume, "/") || !checkDir(subvolume))
	    return false;

	FILE* f = setmntent("/etc/mtab", "r");
	if (!f)
	{
	    y2err("setmntent failed");
	    return false;
	}

	fstype.clear();

	string best_match;

	struct mntent* m;
	while ((m = getmntent(f)))
	{
	    if (strcmp(m->mnt_type, "rootfs") == 0)
		continue;

	    if (strlen(m->mnt_dir) >= best_match.length() && is_subpath(subvolume, m->mnt_dir))
	    {
		best_match = m->mnt_dir;
		fstype = m->mnt_type;
	    }
	}

	endmntent(f);

	if (fstype == "ext4dev")
	    fstype = "ext4";

	y2mil("fstype:" << fstype);

	return !best_match.empty();
    }

}
