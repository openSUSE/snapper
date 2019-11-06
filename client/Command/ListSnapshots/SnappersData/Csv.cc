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

#include <snapper/AppUtil.h>

#include "client/Command/ListSnapshots/SnappersData/Csv.h"
#include "client/utils/CsvFormatter.h"

using namespace std;

namespace snapper
{
    namespace cli
    {

	Command::ListSnapshots::SnappersData::Csv::Csv(
	    const Command::ListSnapshots& command, const string separator) :
	   SnappersData(command), _separator(separator)
	{}


	std::string Command::ListSnapshots::SnappersData::Csv::output() const
	{
	    vector<string> columns;

	    vector<vector<string>> rows;

	    for (const string& column : selected_columns())
		columns.push_back(column);

	    for (ProxySnapper* snapper : snappers())
	    {
		vector<vector<string>> snapper_rows = this->snapper_rows(snapper);

		rows.insert(rows.end(), snapper_rows.begin(), snapper_rows.end());
	    }

	    CsvFormatter formatter(columns, rows, _separator);

	    return formatter.output();
	}


	vector<vector<string>>
	Command::ListSnapshots::SnappersData::Csv::snapper_rows(const ProxySnapper* snapper) const
	{
	    vector<vector<string>> rows;

	    const ProxySnapshots& snapshots = this->snapshots(snapper);

	    for (const ProxySnapshot* snapshot : selected_snapshots(snapper))
	    {
		vector<string> row;

		for (const string& column : selected_columns())
		{
		    row.push_back(value_for(column, *snapshot, snapshots, snapper));
		}

		rows.push_back(row);
	    }

	    return rows;
	}


	string Command::ListSnapshots::SnappersData::Csv::boolean_text(bool value) const
	{
	    return value ? "yes" : "no";
	}

    }
}
