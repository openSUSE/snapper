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

#include <algorithm>

#include <boost/algorithm/string.hpp>

#include "client/Command/ColumnsOption.h"
#include "client/utils/text.h"

using namespace std;

namespace snapper
{
    namespace cli
    {

	vector<string> Command::ColumnsOption::selected_columns() const
	{
	    if (_raw_columns.empty())
		return {};

	    vector<string> columns;

	    boost::split(columns, _raw_columns, boost::is_any_of(","), boost::token_compress_on);

	    return columns;
	}


	string Command::ColumnsOption::error() const
	{
	    if (wrong_columns().empty())
		return "";

	    return _("Unknown columns: ") + boost::algorithm::join(wrong_columns(), ", ");
	}


	vector<string> Command::ColumnsOption::wrong_columns() const
	{
	    vector<string> wrong;

	    for (auto column : selected_columns())
	    {
		if (wrong_column(column))
		    wrong.push_back(column);
	    }

	    return wrong;
	}


	bool Command::ColumnsOption::wrong_column(const std::string& column) const
	{
	    return find(_all_columns.begin(), _all_columns.end(), column) == _all_columns.end();
	}

    }
}
