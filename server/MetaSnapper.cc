/*
 * Copyright (c) 2012 Novell, Inc.
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


#include <sys/types.h>
#include <pwd.h>
#include <boost/algorithm/string.hpp>

#include <snapper/Log.h>

#include "MetaSnapper.h"


uid_t
get_uid(const string& username)
{
    long bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
    char* buf = (char*) malloc(bufsize);
    if (!buf)
	throw;

    struct passwd pwd;
    struct passwd* result;

    if (getpwnam_r(username.c_str(), &pwd, buf, bufsize, &result) != 0)
	throw;

    if (!result)
	throw;

    uid_t r = pwd.pw_uid;

    free(buf);

    return r;
}


MetaSnapper::MetaSnapper(const ConfigInfo& config_info)
    : config_info(config_info), snapper(NULL)
{
    map<string, string>::const_iterator pos1 = config_info.raw.find("USERS");
    if (pos1 != config_info.raw.end())
    {
	string tmp = pos1->second;
	boost::trim(tmp, locale::classic());
	if (!tmp.empty())
	{
	    vector<string> users;
	    boost::split(users, tmp, boost::is_any_of(" \t"), boost::token_compress_on);
	    for (vector<string>::const_iterator it = users.begin(); it != users.end(); ++it)
	    {
		uid_t uid = get_uid(*it);
		uids.push_back(uid);
	    }
	}
    }

    map<string, string>::const_iterator pos2 = config_info.raw.find("GROUPS");
    if (pos2 != config_info.raw.end())
    {
	string tmp = pos2->second;
	boost::trim(tmp, locale::classic());

        if (!tmp.empty())
        {
	    vector<string> groups;
	    boost::split(groups, tmp, boost::is_any_of(" \t"), boost::token_compress_on);
	    for (vector<string>::const_iterator it = groups.begin(); it != groups.end(); ++it)
	    {

	    }
	}
    }
}


MetaSnapper::~MetaSnapper()
{
    delete snapper;
}


Snapper*
MetaSnapper::getSnapper()
{
    if (!snapper)
	snapper = new Snapper(config_info.config_name);

    return snapper;
}


MetaSnappers::MetaSnappers()
{
    list<ConfigInfo> config_infos = Snapper::getConfigs();

    for (list<ConfigInfo>::const_iterator it = config_infos.begin(); it != config_infos.end(); ++it)
    {
	MetaSnapper meta_snapper(*it);
	entries.push_back(meta_snapper);
    }
}


MetaSnappers::~MetaSnappers()
{
}


MetaSnappers::iterator
MetaSnappers::find(const string& config_name)
{
    for (iterator it = entries.begin(); it != entries.end(); ++it)
	if (it->configName() == config_name)
	    return it;

    throw;
}


MetaSnappers meta_snappers;

