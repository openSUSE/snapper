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

#include <sstream>

#include "client/utils/TableFormatter.h"

using namespace std;

namespace snapper
{
    namespace cli
    {

	namespace
	{
	    const TableLineStyle DEFAULT_STYLE = Table::defaultStyle;
	}


	TableLineStyle TableFormatter::default_style()
	{
	    return DEFAULT_STYLE;
	}


	TableFormatter::TableFormatter(
	    vector<pair<string, TableAlign>> columns,
	    vector<vector<string>> rows) :
	    _columns(columns), _rows(rows), _style(default_style())
	{}


	TableFormatter::TableFormatter(
	    vector<pair<string, TableAlign>> columns,
	    vector<vector<string>> rows,
	    TableLineStyle style) :
	    _columns(columns), _rows(rows), _style(style)
	{}


	string TableFormatter::output() const
	{
	    Table::defaultStyle = _style;

	    Table table;

	    TableHeader header;

	    for (auto column : _columns)
		header.add(column.first, column.second);

	     table.setHeader(header);

	    for (auto row : _rows)
	    {
		TableRow table_row;

		for (auto value : row)
		    table_row.add(value);

		table.add(table_row);
	    }

	    ostringstream stream;

	    stream << table;

	    return stream.str();
	}

    }
}