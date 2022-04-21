/*
 * Copyright (c) [2012-2015] Novell, Inc.
 * Copyright (c) [2018-2022] SUSE LLC
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


MetaSnapper::MetaSnapper(ConfigInfo& config_info)
    : config_info(config_info)
{
    set_permissions();
}


MetaSnapper::~MetaSnapper()
{
    delete snapper;
    snapper = nullptr;
}


void
MetaSnapper::setConfigInfo(const map<string, string>& raw)
{
    for (map<string, string>::const_iterator it = raw.begin(); it != raw.end(); ++it)
	config_info.set_value(it->first, it->second);

    getSnapper()->setConfigInfo(raw);

    if (raw.find(KEY_ALLOW_USERS) != raw.end() || raw.find(KEY_ALLOW_GROUPS) != raw.end())
	set_permissions();
}


void
MetaSnapper::set_permissions()
{
    allowed_uids.clear();

    vector<string> users;
    if (config_info.get_value(KEY_ALLOW_USERS, users))
    {
	for (const string& user : users)
	{
	    uid_t tmp;
	    if (get_user_uid(user.c_str(), tmp))
		allowed_uids.push_back(tmp);
	}
    }

    sort(allowed_uids.begin(), allowed_uids.end());
    allowed_uids.erase(unique(allowed_uids.begin(), allowed_uids.end()), allowed_uids.end());

    allowed_gids.clear();

    vector<string> groups;
    if (config_info.get_value(KEY_ALLOW_GROUPS, groups))
    {
	for (const string& group : groups)
	{
	    gid_t tmp;
	    if (get_group_gid(group.c_str(), tmp))
		allowed_gids.push_back(tmp);
	}
    }

    sort(allowed_gids.begin(), allowed_gids.end());
    allowed_gids.erase(unique(allowed_gids.begin(), allowed_gids.end()), allowed_gids.end());
}


Snapper*
MetaSnapper::getSnapper()
{
    if (!snapper)
	snapper = new Snapper(config_info.get_config_name(), "/");

    update_use_time();

    return snapper;
}


void
MetaSnapper::unload()
{
    delete snapper;
    snapper = nullptr;
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


void
MetaSnappers::unload()
{
    for (MetaSnapper& meta_snapper : entries)
	meta_snapper.unload();
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
