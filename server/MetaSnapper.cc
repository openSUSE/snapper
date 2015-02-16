/*
 * Copyright (c) [2012-2015] Novell, Inc.
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
#include <boost/algorithm/string.hpp>

#include <snapper/Log.h>
#include <snapper/AppUtil.h>
#include <snapper/SnapperDefines.h>

#include "MetaSnapper.h"


MetaSnappers meta_snappers;


RefCounter::RefCounter()
    : counter(0), last_used(steady_clock::now())
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
	last_used = steady_clock::now();

    return counter;
}


void
RefCounter::update_use_time()
{
    boost::lock_guard<boost::mutex> lock(mutex);

    last_used = steady_clock::now();
}


int
RefCounter::use_count() const
{
    boost::lock_guard<boost::mutex> lock(mutex);

    return counter;
}


milliseconds
RefCounter::unused_for() const
{
    boost::lock_guard<boost::mutex> lock(mutex);

    if (counter != 0)
	return milliseconds(0);

    return duration_cast<milliseconds>(steady_clock::now() - last_used);
}


MetaSnapper::MetaSnapper(ConfigInfo& config_info)
    : config_info(config_info), snapper(NULL)
{
    set_permissions();
}


MetaSnapper::~MetaSnapper()
{
    delete snapper;
}


void
MetaSnapper::setConfigInfo(const map<string, string>& raw)
{
    for (map<string, string>::const_iterator it = raw.begin(); it != raw.end(); ++it)
	config_info.setValue(it->first, it->second);

    getSnapper()->setConfigInfo(raw);

    if (raw.find(KEY_ALLOW_USERS) != raw.end() || raw.find(KEY_ALLOW_GROUPS) != raw.end())
	set_permissions();
}


void
MetaSnapper::set_permissions()
{
    uids.clear();

    vector<string> users;
    if (config_info.getValue(KEY_ALLOW_USERS, users))
    {
	for (vector<string>::const_iterator it = users.begin(); it != users.end(); ++it)
	{
	    uid_t tmp;
	    if (get_user_uid(it->c_str(), tmp))
		uids.push_back(tmp);
	}
    }

    sort(uids.begin(), uids.end());
    uids.erase(unique(uids.begin(), uids.end()), uids.end());

    gids.clear();

    vector<string> groups;
    if (config_info.getValue(KEY_ALLOW_GROUPS, groups))
    {
	for (vector<string>::const_iterator it = groups.begin(); it != groups.end(); ++it)
	{
	    gid_t tmp;
	    if (get_group_gid(it->c_str(), tmp))
		gids.push_back(tmp);
	}
    }

    sort(gids.begin(), gids.end());
    gids.erase(unique(gids.begin(), gids.end()), gids.end());
}


Snapper*
MetaSnapper::getSnapper()
{
    if (!snapper)
	snapper = new Snapper(config_info.getConfigName(), "/");

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
    list<ConfigInfo> config_infos = Snapper::getConfigs("/");

    for (list<ConfigInfo>::iterator it = config_infos.begin(); it != config_infos.end(); ++it)
    {
	entries.emplace_back(*it);
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
    Snapper::createConfig(config_name, "/", subvolume, fstype, template_name);

    ConfigInfo config_info = Snapper::getConfig(config_name, "/");

    entries.emplace_back(config_info);
}


void
MetaSnappers::deleteConfig(iterator it)
{
    Snapper::deleteConfig(it->configName(), "/");

    entries.erase(it);
}
