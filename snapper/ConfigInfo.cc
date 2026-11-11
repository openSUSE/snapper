/*
 * Copyright (c) [2011-2015] Novell, Inc.
 * Copyright (c) [2016-2025] SUSE LLC
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


    static bool
    is_boolean_key(const string& key)
    {
	static const vector<string> boolean_keys = {
	    "BACKGROUND_COMPARISON", "EMPTY_PRE_POST_CLEANUP", "NUMBER_CLEANUP", "SPECIAL_CMP",
	    KEY_SYNC_ACL, "TIMELINE_CLEANUP", KEY_TIMELINE_CREATE, "UNIQUE_NUMBERS"
	};

	return contains(boolean_keys, key);
    }


    void
    ConfigInfo::check_key_value(const string& key, const string& value) const
    {
	check_key(key);

	if (is_boolean_key(key) && value != "yes" && value != "no")
	    SN_THROW(InvalidConfigdataException());
    }


    void
    ConfigInfo::set_value(const string& key, const string& value)
    {
	check_key_value(key, value);

	SysconfigFile::set_value(key, value);
    }

}
