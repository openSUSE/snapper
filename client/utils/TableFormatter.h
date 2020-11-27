/*
 * Copyright (c) [2019-2020] SUSE LLC
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

#ifndef SNAPPER_CLI_TABLE_FORMATTER_H
#define SNAPPER_CLI_TABLE_FORMATTER_H


#include <string>
#include <vector>
#include <ostream>

#include "client/utils/Table.h"


namespace snapper
{

    using namespace std;


    class TableFormatter
    {

    public:

	static const TableStyle default_style;

	TableFormatter(TableStyle style) : style(style) {}

	TableFormatter(const TableFormatter&) = delete;

	TableFormatter& operator=(const TableFormatter&) = delete;

	vector<pair<string, TableAlign>>& header() { return _header; }
	vector<vector<string>>& rows() { return _rows; }

	friend ostream& operator<<(ostream& stream, const TableFormatter& table_formatter);

    private:

	const TableStyle style;

	vector<pair<string, TableAlign>> _header;
	vector<vector<string>> _rows;

    };

}

#endif
