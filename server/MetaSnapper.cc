/*
 * Copyright (c) [2012-2013] Novell, Inc.
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


#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <boost/algorithm/string.hpp>

#include <snapper/Log.h>

#include "MetaSnapper.h"


MetaSnappers meta_snappers;


RefCounter::RefCounter()
    : counter(0), last_used(monotonic_time())
{
}


int
RefCounter::inc_use_count()
{
    boost::lock_guard<boost::mutex> lock(mutex);

    return ++counter;
}


int
RefCounter::dec_use_count()
{
    boost::lock_guard<boost::mutex> lock(mutex);

    assert(counter > 0);

    if (--counter == 0)
	last_used = monotonic_time();

    return counter;
}


void
RefCounter::update_use_time()
{
    boost::lock_guard<boost::mutex> lock(mutex);

    last_used = monotonic_time();
}


int
RefCounter::use_count() const
{
    boost::lock_guard<boost::mutex> lock(mutex);

    return counter;
}


int
RefCounter::unused_for() const
{
    boost::lock_guard<boost::mutex> lock(mutex);

    if (counter != 0)
	return 0;

    struct timespec tmp;
    clock_gettime(CLOCK_MONOTONIC, &tmp);

    return tmp.tv_sec - last_used;
}


time_t
RefCounter::monotonic_time()
{
    struct timespec tmp;
    clock_gettime(CLOCK_MONOTONIC, &tmp);
    return tmp.tv_sec;
}


bool
get_user_uid(const char* username, uid_t& uid)
{
    struct passwd pwd;
    struct passwd* result;

    long bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
    char buf[bufsize];

    if (getpwnam_r(username, &pwd, buf, bufsize, &result) != 0 || result != &pwd)
    {
	y2war("couldn't find username '" << username << "'");
	return false;
    }

    memset(pwd.pw_passwd, 0, strlen(pwd.pw_passwd));

    uid = pwd.pw_uid;

    return true;
}


bool
get_group_uids(const char* groupname, vector<uid_t>& uids)
{
    struct group grp;
    struct group* result;

    long bufsize = sysconf(_SC_GETGR_R_SIZE_MAX);
    char buf[bufsize];

    if (getgrnam_r(groupname, &grp, buf, bufsize, &result) != 0 || result != &grp)
    {
	y2war("couldn't find groupname '" << groupname << "'");
	return false;
    }

    memset(grp.gr_passwd, 0, strlen(grp.gr_passwd));

    uids.clear();

    for (char** p = grp.gr_mem; *p != NULL; ++p)
    {
	uid_t uid;
	if (get_user_uid(*p, uid))
	    uids.push_back(uid);
    }

    return true;
}


MetaSnapper::MetaSnapper(const ConfigInfo& config_info)
    : config_info(config_info), snapper(NULL)
{
    vector<string> users;
    if (config_info.getValue("ALLOW_USERS", users))
    {
	for (vector<string>::const_iterator it = users.begin(); it != users.end(); ++it)
	{
	    uid_t tmp;
	    if (get_user_uid(it->c_str(), tmp))
		uids.push_back(tmp);
	}
    }

    vector<string> groups;
    if (config_info.getValue("ALLOW_GROUPS", groups))
    {
	for (vector<string>::const_iterator it = groups.begin(); it != groups.end(); ++it)
	{
	    vector<uid_t> tmp;
	    if (get_group_uids(it->c_str(), tmp))
		uids.insert(uids.end(), tmp.begin(), tmp.end());
	}
    }

    sort(uids.begin(), uids.end());
    uids.erase(unique(uids.begin(), uids.end()), uids.end());
}


MetaSnapper::~MetaSnapper()
{
    delete snapper;
}


Snapper*
MetaSnapper::getSnapper()
{
    if (!snapper)
	snapper = new Snapper(config_info.getConfigName());

    update_use_time();

    return snapper;
}


void
MetaSnapper::unload()
{
    delete snapper;
    snapper = NULL;
}


MetaSnappers::MetaSnappers()
{
}


MetaSnappers::~MetaSnappers()
{
}


void
MetaSnappers::init()
{
    list<ConfigInfo> config_infos = Snapper::getConfigs();

    for (list<ConfigInfo>::const_iterator it = config_infos.begin(); it != config_infos.end(); ++it)
    {
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 4)
	entries.emplace_back(*it);
#else
	entries.push_back(*it);
#endif
    }
}


MetaSnappers::iterator
MetaSnappers::find(const string& config_name)
{
    for (iterator it = entries.begin(); it != entries.end(); ++it)
	if (it->configName() == config_name)
	    return it;

    throw UnknownConfig();
}


void
MetaSnappers::createConfig(const string& config_name, const string& subvolume,
			   const string& fstype, const string& template_name)
{
    Snapper::createConfig(config_name, subvolume, fstype, template_name);

    ConfigInfo config_info = Snapper::getConfig(config_name);

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 4)
    entries.emplace_back(config_info);
#else
    entries.push_back(config_info);
#endif
}


void
MetaSnappers::deleteConfig(iterator it)
{
    Snapper::deleteConfig(it->configName());

    entries.erase(it);
}
