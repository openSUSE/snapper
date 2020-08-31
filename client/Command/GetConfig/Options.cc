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

#include "client/Command/GetConfig/Options.h"
#include "client/utils/text.h"

using namespace std;

namespace snapper
{
    namespace cli
    {

	namespace
	{

	    const vector<Option> OPTIONS = {
		Option("columns",	required_argument)
	    };

	}


	const string Command::GetConfig::Options::Columns::KEY = "key";
	const string Command::GetConfig::Options::Columns::VALUE = "value";


	const vector<string> Command::GetConfig::Options::ALL_COLUMNS = {
	    Columns::KEY,
	    Columns::VALUE
	};


	string Command::GetConfig::Options::help_text()
	{
	    return _("    Options for 'get-config' command:\n"
		     "\t--columns <columns>\t\tColumns to show separated by comma.\n"
		     "\t\t\t\t\tPossible columns: key, value.\n"
		     "\t\t\t\t\tColumns are not selected when JSON format is used.\n");
	}


	Command::GetConfig::Options::Options(GetOpts& parser) :
	    cli::Options(parser), _columns_option(ALL_COLUMNS)
	{
	    parse_options();

	    _columns_option.set_raw_columns(columns_raw());
	}


	void Command::GetConfig::Options::parse_options()
	{
	    _options = _parser.parse("get-config", OPTIONS);
	}


	vector<string> Command::GetConfig::Options::columns() const
	{
	    if (has_option("columns"))
		return _columns_option.selected_columns();

	    return { Columns::KEY, Columns::VALUE };
	}


	string Command::GetConfig::Options::columns_raw() const
	{
	    return has_option("columns") ? get_argument("columns") : "";
	}

    }
}
