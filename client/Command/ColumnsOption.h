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

#ifndef SNAPPER_CLI_COMMAND_COLUMNS_OPTION_H
#define SNAPPER_CLI_COMMAND_COLUMNS_OPTION_H

#include <string>
#include <vector>

#include "client/Command.h"

namespace snapper
{
    namespace cli
    {

	class Command::ColumnsOption
	{

	public:

	    ColumnsOption(const std::vector<std::string>& all_columns) :
		_all_columns(all_columns), _raw_columns()
	    {}

	    void set_raw_columns(const string& raw_columns)
	    {
		_raw_columns = raw_columns;
	    }

	    std::vector<std::string> selected_columns() const;

	    std::string error() const;

	    std::vector<std::string> wrong_columns() const;

	private:

	    bool wrong_column(const std::string& column) const;

	    const std::vector<std::string>& _all_columns;

	    std::string _raw_columns;

	};

    }
}

#endif
