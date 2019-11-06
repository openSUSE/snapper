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

#include <utility>

#include "client/Command/ListSnapshots/SnappersData/Json.h"
#include "client/Command/ListSnapshots/Options.h"
#include "client/utils/JsonFormatter.h"

using namespace std;

namespace snapper
{
    namespace cli
    {

	namespace
	{

	    string userdata_json(const map<string, string>& userdata)
	    {
		if (userdata.empty())
		    return "";

		JsonFormatter::Data data;

		for (auto udata : userdata)
		    data.emplace_back(udata.first, udata.second);

		JsonFormatter formatter(data);

		formatter.set_inline(true);

		return formatter.output(3);
	    }

	}


	std::string Command::ListSnapshots::SnappersData::Json::output() const
	{
	    JsonFormatter::Data data;

	    vector<string> keys;

	    for (ProxySnapper* snapper : snappers())
	    {
		data.emplace_back(snapper_key(snapper), snapper_value(snapper));

		keys.push_back(snapper_key(snapper));
	    }

	    JsonFormatter formatter(data);

	    formatter.skip_format_values(keys);

	    return formatter.output();
	}


	string
	Command::ListSnapshots::SnappersData::Json::snapper_key(const ProxySnapper* snapper) const
	{
	    return snapper->configName();
	}


	string
	Command::ListSnapshots::SnappersData::Json::snapper_value(const ProxySnapper* snapper) const
	{
	    vector<string> data;

	    const ProxySnapshots& snapshots = this->snapshots(snapper);

	    for (const ProxySnapshot* snapshot : selected_snapshots(snapper))
	    {
		JsonFormatter::Data snapshot_data;

		vector<string> skip_format;

		for (const string& column : selected_columns())
		{
		    string value = value_for(column, *snapshot, snapshots, snapper);

		    if (column == Options::Columns::USERDATA)
			value = userdata_json(snapshot->getUserdata());

		    if (!is_json_string(column))
		    {
			skip_format.push_back(column);

			if (value.empty())
			    value = "null";
		    }

		    snapshot_data.emplace_back(column, value);
		}

		JsonFormatter formatter(snapshot_data);

		formatter.skip_format_values(skip_format);

		data.push_back(formatter.output(2));
	    }

	    JsonFormatter::List json_list(data);

	    return json_list.output(1);
	}


	bool Command::ListSnapshots::SnappersData::Json::is_json_string(const string& column) const
	{
	    if (column == Options::Columns::NUMBER ||
		column == Options::Columns::DEFAULT ||
		column == Options::Columns::ACTIVE ||
		column == Options::Columns::USED_SPACE ||
		column == Options::Columns::USERDATA ||
		column == Options::Columns::PRE_NUMBER ||
		column == Options::Columns::POST_NUMBER)
		return false;
	    else
		return true;
	}


	string Command::ListSnapshots::SnappersData::Json::boolean_text(bool value) const
	{
	    return value ? "true" : "false";
	}

    }
}
