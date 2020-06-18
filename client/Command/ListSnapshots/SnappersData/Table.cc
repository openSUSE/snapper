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

#include <snapper/AppUtil.h>

#include "client/Command/ListSnapshots/SnappersData/Table.h"
#include "client/Command/ListSnapshots/Options.h"
#include "client/utils/TableFormatter.h"
#include "client/utils/text.h"

using namespace std;

namespace snapper
{
    namespace cli
    {

	Command::ListSnapshots::SnappersData::Table::Table(
	    const Command::ListSnapshots& command, TableLineStyle style) :
	    SnappersData(command), _style(style)
	{}


	string Command::ListSnapshots::SnappersData::Table::output() const
	{
	    string output;

	    vector<ProxySnapper*> snappers = this->snappers();

	    for (ProxySnapper* snapper : snappers)
	    {
		if (snappers.size() > 1)
		{
		    if (snapper != *snappers.begin())
			output += "\n";

		    output += snapper_description(snapper) + "\n";
		}

		output += snapper_table(snapper);
	    }

	    return output;
	}


	string Command::ListSnapshots::SnappersData::Table::label_for(const string& column) const
	{
	    Options::ListMode list_mode = command().options().list_mode();

	    if (column == Options::Columns::CONFIG)
		return _("Config");

	    if (column == Options::Columns::SUBVOLUME)
		return _("Subvolume");

	    if (column == Options::Columns::NUMBER)
		return list_mode == Options::ListMode::PRE_POST ? _("Pre #") : _("#");

	    if (column == Options::Columns::DEFAULT)
		return _("Default");

	    if (column == Options::Columns::ACTIVE)
		return _("Active");

	    if (column == Options::Columns::TYPE)
		return _("Type");

	    if (column == Options::Columns::DATE)
		return list_mode == Options::ListMode::PRE_POST ? _("Pre Date") : _("Date");

	    if (column == Options::Columns::USER)
		return _("User");

	    if (column == Options::Columns::USED_SPACE)
		return _("Used Space");

	    if (column == Options::Columns::CLEANUP)
		return _("Cleanup");

	    if (column == Options::Columns::DESCRIPTION)
		return _("Description");

	    if (column == Options::Columns::USERDATA)
		return _("Userdata");

	    if (column == Options::Columns::PRE_NUMBER)
		return _("Pre #");

	    if (column == Options::Columns::POST_NUMBER)
		return _("Post #");

	    if (column == Options::Columns::POST_DATE)
		return _("Post Date");

	    return "";
	}


	string Command::ListSnapshots::SnappersData::Table::number_value(const ProxySnapshot& snapshot,
	    const ProxySnapshots& snapshots) const
	{
	    bool is_default = is_default_snapshot(snapshot, snapshots);
	    bool is_active = is_active_snapshot(snapshot, snapshots);

	    static const char sign[2][2] = { { ' ', '-' }, { '+', '*' } };

	    return Command::ListSnapshots::SnappersData::number_value(snapshot, snapshots) +
		sign[is_default][is_active];
	}


	string Command::ListSnapshots::SnappersData::Table::boolean_text(bool value) const
	{
	    return value ? _("yes") : _("no");
	}


	string Command::ListSnapshots::SnappersData::Table::snapper_description(
	    const ProxySnapper* snapper) const
	{
	    return sformat(_("Config: %s, subvolume: %s"), config_value(snapper).c_str(),
		subvolume_value(snapper).c_str());
	}


	string Command::ListSnapshots::SnappersData::Table::snapper_table(
	    const ProxySnapper* snapper) const
	{
	    const ProxySnapshots& snapshots = this->snapshots(snapper);

	    vector<pair<string, TableAlign>> columns;

	    vector<vector<string>> rows;

	    for (const string& column : selected_columns())
	    {
		if (skip_column(column, snapper))
		    continue;

		columns.emplace_back(label_for(column), column_alignment(column));
	    }

	    for (const ProxySnapshot* snapshot : selected_snapshots(snapper))
	    {
		vector<string> row;

		for (const string& column : selected_columns())
		{
		    if (skip_column(column, snapper))
			continue;

		    row.push_back(value_for(column, *snapshot, snapshots, snapper));
		}

		rows.push_back(row);
	    }

	    TableFormatter formatter(columns, rows, _style);

	    return formatter.output();
	}


	bool Command::ListSnapshots::SnappersData::Table::skip_column(const string& column,
	    const ProxySnapper* snapper) const
	{
	    return  column == Options::Columns::USED_SPACE && !calculated_used_space(snapper);
	}


	TableAlign
	Command::ListSnapshots::SnappersData::Table::column_alignment(const string& column) const
	{
	    if (column == Options::Columns::NUMBER ||
		column == Options::Columns::PRE_NUMBER ||
		column == Options::Columns::POST_NUMBER ||
		column == Options::Columns::USED_SPACE)
		return TableAlign::RIGHT;
	    else
		return TableAlign::LEFT;
	}

    }
}
