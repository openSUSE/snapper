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

#include <boost/algorithm/string.hpp>

#include "client/Command/ListConfigs/Options.h"
#include "client/utils/text.h"

using namespace std;

namespace snapper
{
    namespace cli
    {

	namespace
	{

	    const option OPTIONS[] = {
		{ "columns",	required_argument,	0,	0},
		{ 0, 0, 0, 0 }
	    };

	}


	const string Command::ListConfigs::Options::Columns::CONFIG = "config";
	const string Command::ListConfigs::Options::Columns::SUBVOLUME = "subvolume";


	const vector<string> Command::ListConfigs::Options::ALL_COLUMNS = {
	    Columns::CONFIG,
	    Columns::SUBVOLUME
	};


	string Command::ListConfigs::Options::help_text()
	{
	    return _("    Options for 'list-configs' command:\n"
		     "\t--columns <columns>\t\tColumns to show separated by comma.\n"
		     "\t\t\t\t\tPossible columns: config, subvolume.\n");
	}


	Command::ListConfigs::Options::Options(GetOpts& parser) :
	    cli::Options(parser), _columns_option(ALL_COLUMNS)
	{
	    parse_options();

	    _columns_option.set_raw_columns(columns_raw());
	}


	void Command::ListConfigs::Options::parse_options()
	{
	    _options = _parser.parse("list-configs", OPTIONS);
	}


	vector<string> Command::ListConfigs::Options::columns() const
	{
	    if (has_option("columns"))
		return _columns_option.selected_columns();

	    return { Columns::CONFIG, Columns::SUBVOLUME };
	}


	vector<string> Command::ListConfigs::Options::errors() const
	{
	    vector<string> detected_errors;

	    if (!_columns_option.wrong_columns().empty())
		detected_errors.push_back(_columns_option.error());

	    return detected_errors;
	}


	string Command::ListConfigs::Options::columns_raw() const
	{
	    return has_option("columns") ? get_option("columns")->second : "";
	}

    }
}
