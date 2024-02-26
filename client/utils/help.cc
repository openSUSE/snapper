/*
 * Copyright (c) 2024 SUSE LLC
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


#include <iostream>

#include "help.h"
#include "Table.h"
#include "text.h"


namespace snapper
{

    void
    print_options(std::initializer_list<std::initializer_list<string>> init)
    {
	Table table({ Cell(_("Name"), Id::NAME), Cell(_("Description"), Id::DESCRIPTION) });
	table.set_show_header(false);
	table.set_show_grid(false);
	table.set_global_indent(8);
	table.set_min_width(Id::NAME, 28);

	for (const std::initializer_list<string>& row_init : init)
	    table.add(Table::Row(table, row_init));

	cout << table << '\n';
    }

}
