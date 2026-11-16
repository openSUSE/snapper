/*
 * Copyright (c) [2011-2015] Novell, Inc.
 * Copyright (c) [2016-2026] SUSE LLC
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


#include "config.h"

#include <regex>

#include "snapper/ConfigInfo.h"
#include "snapper/Exception.h"
#include "snapper/SnapperDefines.h"
#include "snapper/AppUtil.h"
#include "snapper/Snapper.h"
#include "snapper/SnapperTmpl.h"


namespace snapper
{
    using namespace std;


    ConfigInfo::ConfigInfo(const string& config_name, const string& root_prefix)
	: SysconfigFile(prepend_root_prefix(root_prefix, CONFIGS_DIR "/" + config_name)),
	  config_name(config_name), root_prefix(root_prefix), subvolume("/")
    {
	if (!get_value(KEY_SUBVOLUME, subvolume))
	    SN_THROW(InvalidConfigException());
    }


    void
    ConfigInfo::check_key(const string& key) const
    {
	if (key == KEY_SUBVOLUME || key == KEY_FSTYPE)
	    SN_THROW(InvalidConfigdataException());

	try
	{
	    SysconfigFile::check_key(key);
	}
	catch (const InvalidKeyException& e)
	{
	    SN_CAUGHT(e);

	    SN_THROW(InvalidConfigdataException());
	}
    }


    namespace
    {

	bool
	check_boolean(const string& value)
	{
	    return value == "yes" || value == "no";
	}


	bool
	check_number(const string& value)
	{
	    static const regex number_regex("[0-9]+", regex::extended);

	    return regex_match(value, number_regex);
	}


	bool
	check_range(const string& value)
	{
	    static const regex range_regex("[0-9]+(-[0-9]+)?");

	    return regex_match(value, range_regex);
	}


	using registry_t = map<string, bool (*)(const string&)>;

	const registry_t registry =
	{
	    { "BACKGROUND_COMPARISON", &check_boolean },
	    { "EMPTY_PRE_POST_CLEANUP",	&check_boolean },
	    { "NUMBER_CLEANUP",	&check_boolean },
	    { "SPECIAL_CMP", &check_boolean },
	    { "TIMELINE_CLEANUP", &check_boolean },
	    { "UNIQUE_NUMBERS", &check_boolean },
	    { KEY_SYNC_ACL, &check_boolean },
	    { KEY_TIMELINE_CREATE, &check_boolean },

	    { "EMPTY_PRE_POST_MIN_AGE", &check_number },
	    { "NUMBER_MIN_AGE", &check_number },
	    { "TIMELINE_MIN_AGE", &check_number },

	    { "NUMBER_LIMIT", &check_range },
	    { "NUMBER_LIMIT_IMPORTANT", &check_range },
	    { "TIMELINE_LIMIT_HOURLY", &check_range },
	    { "TIMELINE_LIMIT_DAILY", &check_range },
	    { "TIMELINE_LIMIT_WEEKLY", &check_range },
	    { "TIMELINE_LIMIT_MONTHLY", &check_range },
	    { "TIMELINE_LIMIT_QUARTERLY", &check_range },
	    { "TIMELINE_LIMIT_YEARLY", &check_range },
	};

    }


    void
    ConfigInfo::check_key_value(const string& key, const string& value) const
    {
	check_key(key);

	const registry_t::const_iterator it = registry.find(key);
	if (it != registry.end())
	{
	    if (!it->second(value))
		SN_THROW(InvalidConfigdataException());
	}
    }


    void
    ConfigInfo::set_value(const string& key, const string& value)
    {
	check_key_value(key, value);

	SysconfigFile::set_value(key, value);
    }

}
