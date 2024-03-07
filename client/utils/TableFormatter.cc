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


#include "client/utils/TableFormatter.h"


namespace snapper
{

    using namespace std;


    ostream&
    operator<<(ostream& stream, const TableFormatter& table_formatter)
    {
	Table table(table_formatter._header);

	table.set_style(table_formatter.style);

	for (Id id : table_formatter._abbrev)
	    if (table.has_id(id))
		table.set_abbreviate(id, true);

	for (const vector<string>& row : table_formatter._rows)
	{
	    Table::Row table_row(table);

	    for (const string& value : row)
		table_row.add(value);

	    table.add(table_row);
	}

	return stream << table;
    }

}
