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

#include "client/Command/GetConfig/ConfigData.h"
#include "client/Command/GetConfig/Options.h"

using namespace std;

namespace snapper
{
    namespace cli
    {

	Command::GetConfig::ConfigData::ConfigData(const Command::GetConfig& command) :
	    _command(command)
	{}


	Command::GetConfig::ConfigData::~ConfigData()
	{}


	map<string, string> Command::GetConfig::ConfigData::values() const
	{
	    const string config_name = _command.global_options().config();

	    const ProxySnapper* snapper = _command.snappers().getSnapper(config_name);

	    const ProxyConfig config = snapper->getConfig();

	    return config.getAllValues();
	}

    }
}
