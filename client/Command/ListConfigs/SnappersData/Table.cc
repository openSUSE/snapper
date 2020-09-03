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

#include "client/Command/ListConfigs/SnappersData/Table.h"
#include "client/Command/ListConfigs/Options.h"
#include "client/utils/TableFormatter.h"
#include "client/utils/text.h"

using namespace std;

namespace snapper
{
    namespace cli
    {

	Command::ListConfigs::SnappersData::Table::Table(
	    const Command::ListConfigs& command, TableStyle style) :
	    SnappersData(command), _style(style)
	{}


	string Command::ListConfigs::SnappersData::Table::output() const
	{
	    vector<pair<string, TableAlign>> columns;

	    vector<vector<string>> rows;

	    for (const string& column : _command.options().columns())
		columns.emplace_back(label_for(column), TableAlign::LEFT);

	    for (ProxySnapper* snapper : snappers())
	    {
		vector<string> row;

		for (auto attribute : _command.options().columns())
		    row.push_back(value_for(attribute, snapper));

		rows.push_back(row);
	    }

	    TableFormatter formatter(columns, rows, _style);

	    return formatter.str();
	}


	string Command::ListConfigs::SnappersData::Table::label_for(const string& column) const
	{
	    if (column == Options::Columns::CONFIG)
		return _("Config");

	    if (column == Options::Columns::SUBVOLUME)
		return _("Subvolume");

	    return "";
	}

    }
}
