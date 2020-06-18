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

#include <boost/algorithm/string.hpp>

#include "client/Command/ColumnsOption.h"
#include "client/utils/text.h"

using namespace std;

namespace snapper
{
    namespace cli
    {

	vector<string>
	Command::ColumnsOption::selected_columns() const
	{
	    vector<string> columns;

	    boost::split(columns, _raw_columns, boost::is_any_of(","), boost::token_compress_on);

	    vector<string> unknowns;
	    for (const string& column : columns)
	    {
		if (find(_all_columns.begin(), _all_columns.end(), column) == _all_columns.end())
		    unknowns.push_back(column);
	    }

	    if (!unknowns.empty())
	    {
		string error = _("Unknown column:", "Unknown columns:", unknowns.size()) + string(" ") +
		    boost::algorithm::join(unknowns, ", ") + '\n' +
		    _("Allowed columns are:") + string(" ") + boost::join(_all_columns, ", ");

		SN_THROW(OptionsException(error));
	    }

	    return columns;
	}

    }
}
