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

#include "client/Command/ListConfigs/SnappersData/Json.h"
#include "client/Command/ListConfigs/Options.h"
#include "client/utils/JsonFormatter.h"

using namespace std;

namespace snapper
{
    namespace cli
    {

	namespace
	{

	    const string SNAPPERS_KEY = "configs";

	}


	std::string Command::ListConfigs::SnappersData::Json::output() const
	{
	    JsonFormatter::Data data;

	    data.emplace_back(SNAPPERS_KEY, snappers_json());

	    JsonFormatter formatter(data);

	    formatter.skip_format_values({ SNAPPERS_KEY });

	    return formatter.str();
	}


	string
	Command::ListConfigs::SnappersData::Json::snappers_json() const
	{
	    vector<string> data;

	    for (ProxySnapper* snapper : snappers())
	    {
		data.push_back(snapper_json(snapper));
	    }

	    JsonFormatter::List json_list(data);

	    return json_list.str(1);
	}


	string Command::ListConfigs::SnappersData::Json::snapper_json(ProxySnapper* snapper) const
	{
	    JsonFormatter::Data data;

	    for (const string& column : _command.options().columns())
		data.emplace_back(column, value_for(column, snapper));

	    JsonFormatter formatter(data);

	    return formatter.str(2);
	}

    }
}
