/*
 * Copyright (c) [2019-2024] SUSE LLC
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


#include <cstring>
#include <langinfo.h>

#include "client/utils/TableFormatter.h"


namespace snapper
{

    using namespace std;


    TableStyle
    TableFormatter::default_style()
    {
	return strcmp(nl_langinfo(CODESET), "UTF-8") == 0 ? TableStyle::Light : TableStyle::Ascii;
    }


    ostream&
    operator<<(ostream& stream, const TableFormatter& table_formatter)
    {
	Table table;
	table.set_style(table_formatter.style);

	TableHeader table_header;

	for (const pair<string, TableAlign>& column : table_formatter._header)
	    table_header.add(column.first, column.second);

	table.setHeader(table_header);
	table.set_abbrev(table_formatter._abbrev);

	for (const vector<string>& row : table_formatter._rows)
	{
	    TableRow table_row;

	    for (const string& value : row)
		table_row.add(value);

	    table.add(table_row);
	}

	return stream << table;
    }

}
