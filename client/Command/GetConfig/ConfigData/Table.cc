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

#include "client/Command/GetConfig/ConfigData/Table.h"
#include "client/Command/GetConfig/Options.h"
#include "client/utils/TableFormatter.h"
#include "client/utils/text.h"

using namespace std;

namespace snapper
{
    namespace cli
    {

	Command::GetConfig::ConfigData::Table::Table(
	    const Command::GetConfig& command, TableLineStyle style) :
	    ConfigData(command), _style(style)
	{}


	string Command::GetConfig::ConfigData::Table::output() const
	{
	    vector<pair<string, TableAlign>> columns;

	    vector<vector<string>> rows;

	    for (const string& column : _command.options().columns())
		columns.emplace_back(label_for(column), TableAlign::LEFT);

	    for (const map<string, string>::value_type& value : values())
	    {
		vector<string> row;

		for (const string& column : _command.options().columns())
		{
		    if (column == Options::Columns::KEY)
			row.push_back(value.first);
		    else
			row.push_back(value.second);
		}

		rows.push_back(row);
	    }

	    TableFormatter formatter(columns, rows, _style);

	    return formatter.output();
	}


	string Command::GetConfig::ConfigData::Table::label_for(const string& column) const
	{
	    if (column == Options::Columns::KEY)
		return _("Key");

	    if (column == Options::Columns::VALUE)
		return _("Value");

	    return "";
	}
    }
}
