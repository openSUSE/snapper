/*
 * Copyright (c) [2019] SUSE LLC
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

#include "client/Command/ListConfigs.h"
#include "client/Command/ListConfigs/Options.h"
#include "client/Command/ListConfigs/SnappersData.h"

using namespace std;

namespace snapper
{
    namespace cli
    {

	Command::ListConfigs::SnappersData::SnappersData(const Command::ListConfigs& command) :
	    _command(command)
	{}


	Command::ListConfigs::SnappersData::~SnappersData()
	{}


	string Command::ListConfigs::SnappersData::value_for(const string& column,
	    ProxySnapper* snapper) const
	{
	    if (column == Options::Columns::CONFIG)
		return snapper->configName();

	    if (column == Options::Columns::SUBVOLUME)
		return snapper->getConfig().getSubvolume();

	    return "";
	}


	vector<ProxySnapper*> Command::ListConfigs::SnappersData::snappers() const
	{
	    vector<ProxySnapper*> snappers;

	    for (auto config_name : configs())
	    {
		ProxySnapper* snapper = _command.snappers().getSnapper(config_name);

		snappers.push_back(snapper);
	    }

	    return snappers;
	}


	vector<string> Command::ListConfigs::SnappersData::configs() const
	{
	    vector<string> names;

	    for (map<string, ProxyConfig>::value_type it : _command.snappers().getConfigs())
		names.push_back(it.first);

	    return names;
	}

    }
}
