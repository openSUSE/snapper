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

#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "client/Command/ListSnapshots/Options.h"
#include "client/utils/text.h"

using namespace std;

namespace snapper
{
    namespace cli
    {

	namespace
	{
	    const option OPTIONS[] = {
		{ "type",		required_argument,	0,	't' },
		{ "disable-used-space",	no_argument,		0,	0 },
		{ "all-configs",	no_argument,		0,	'a' },
		{ "columns", 		required_argument,	0,	0},
		{ 0, 0, 0, 0 }
	    };
	}


	string Command::ListSnapshots::Options::help_text()
	{
	    return _("    Options for 'list' command:\n"
		     "\t--type, -t <type>\t\tType of snapshots to list.\n"
		     "\t--disable-used-space\t\tDisable showing used space.\n"
		     "\t--all-configs, -a\t\tList snapshots from all accessible configs.\n"
		     "\t--columns <columns>\t\tColumns to show separated by comma.\n"
		     "\t\t\t\t\tPossible columns: config, subvolume, number, default, active,\n"
		     "\t\t\t\t\ttype, date, user, used-space, cleanup, description, userdata,\n"
		     "\t\t\t\t\tpre-number, post-number, post-date.\n");
	}


	const string Command::ListSnapshots::Options::Columns::CONFIG = "config";
	const string Command::ListSnapshots::Options::Columns::SUBVOLUME = "subvolume";
	const string Command::ListSnapshots::Options::Columns::NUMBER = "number";
	const string Command::ListSnapshots::Options::Columns::DEFAULT = "default";
	const string Command::ListSnapshots::Options::Columns::ACTIVE = "active";
	const string Command::ListSnapshots::Options::Columns::TYPE = "type";
	const string Command::ListSnapshots::Options::Columns::DATE = "date";
	const string Command::ListSnapshots::Options::Columns::USER = "user";
	const string Command::ListSnapshots::Options::Columns::USED_SPACE = "used-space";
	const string Command::ListSnapshots::Options::Columns::CLEANUP = "cleanup";
	const string Command::ListSnapshots::Options::Columns::DESCRIPTION = "description";
	const string Command::ListSnapshots::Options::Columns::USERDATA = "userdata";
	const string Command::ListSnapshots::Options::Columns::PRE_NUMBER = "pre-number";
	const string Command::ListSnapshots::Options::Columns::POST_NUMBER = "post-number";
	const string Command::ListSnapshots::Options::Columns::POST_DATE = "post-date";


	const vector<string> Command::ListSnapshots::Options::ALL_COLUMNS = {
	    Columns::CONFIG,
	    Columns::SUBVOLUME,
	    Columns::NUMBER,
	    Columns::DEFAULT,
	    Columns::ACTIVE,
	    Columns::TYPE,
	    Columns::DATE,
	    Columns::USER,
	    Columns::USED_SPACE,
	    Columns::CLEANUP,
	    Columns::DESCRIPTION,
	    Columns::USERDATA,
	    Columns::PRE_NUMBER,
	    Columns::POST_NUMBER,
	    Columns::POST_DATE
	};


	Command::ListSnapshots::Options::Options(GetOpts& parser) :
	    cli::Options(parser), _columns_option(ALL_COLUMNS)
	{
	    parse_options();

	    _disable_used_space = has_option("disable-used-space");
	    _all_configs = has_option("all-configs");
	    _list_mode = list_mode_value();

	    _columns_option.set_raw_columns(columns_raw());
	}


	void Command::ListSnapshots::Options::parse_options()
	{
	    _options = _parser.parse("list", OPTIONS);
	}


	vector<string>
	Command::ListSnapshots::Options::columns(GlobalOptions::OutputFormat format) const
	{
	    if (has_option("columns"))
		return _columns_option.selected_columns();

	    return list_mode_columns(format);
	}


	vector<string> Command::ListSnapshots::Options::errors() const
	{
	    vector<string> detected_errors;

	    if (wrong_type())
		detected_errors.push_back(type_error());

	    if (wrong_columns())
		detected_errors.push_back(columns_error());

	    return detected_errors;
	}


	Command::ListSnapshots::Options::ListMode
	Command::ListSnapshots::Options::list_mode_value() const
	{
	    string type = has_option("type") ? get_option("type")->second : "";

	    if (type == "all")
		return LM_ALL;

	    if (type == "single")
		return LM_SINGLE;

	    if (type == "pre-post")
		return LM_PRE_POST;

	    return LM_ALL;
	}


	string Command::ListSnapshots::Options::columns_raw() const
	{
	    return has_option("columns") ? get_option("columns")->second : "";
	}


	vector<string>
	Command::ListSnapshots::Options::list_mode_columns(GlobalOptions::OutputFormat format) const
	{
	    vector<string> columns;

	    switch (list_mode())
	    {
		case LM_ALL:
		    columns = all_mode_columns(format);
		    break;

		case LM_SINGLE:
		    columns = single_mode_columns(format);
		    break;

		case LM_PRE_POST:
		    columns = pre_post_mode_columns(format);
		    break;
	    }

	    if (disable_used_space())
	    {
		columns.erase(
		    remove(columns.begin(), columns.end(), Columns::USED_SPACE), columns.end());
	    }

	    return columns;
	}


	vector<string>
	Command::ListSnapshots::Options::all_mode_columns(GlobalOptions::OutputFormat format) const
	{
	    if (format == GlobalOptions::OutputFormat::TABLE)
		return {
		    Columns::NUMBER,
		    Columns::TYPE,
		    Columns::PRE_NUMBER,
		    Columns::DATE,
		    Columns::USER,
		    Columns::USED_SPACE,
		    Columns::CLEANUP,
		    Columns::DESCRIPTION,
		    Columns::USERDATA
		};

	    if (format == GlobalOptions::OutputFormat::CSV)
		return {
		    Columns::CONFIG,
		    Columns::SUBVOLUME,
		    Columns::NUMBER,
		    Columns::DEFAULT,
		    Columns::ACTIVE,
		    Columns::TYPE,
		    Columns::PRE_NUMBER,
		    Columns::DATE,
		    Columns::USER,
		    Columns::USED_SPACE,
		    Columns::CLEANUP,
		    Columns::DESCRIPTION,
		    Columns::USERDATA
		};

	    if (format == GlobalOptions::OutputFormat::JSON)
		return {
		    Columns::SUBVOLUME,
		    Columns::NUMBER,
		    Columns::DEFAULT,
		    Columns::ACTIVE,
		    Columns::TYPE,
		    Columns::PRE_NUMBER,
		    Columns::DATE,
		    Columns::USER,
		    Columns::USED_SPACE,
		    Columns::CLEANUP,
		    Columns::DESCRIPTION,
		    Columns::USERDATA
		};

	    throw logic_error("unknown output format");
	}


	vector<string>
	Command::ListSnapshots::Options::single_mode_columns(GlobalOptions::OutputFormat format) const
	{
	    if (format == GlobalOptions::OutputFormat::TABLE)
		return {
		    Columns::NUMBER,
		    Columns::DATE,
		    Columns::USER,
		    Columns::USED_SPACE,
		    Columns::DESCRIPTION,
		    Columns::USERDATA
		};

	    if (format == GlobalOptions::OutputFormat::CSV)
		return {
		    Columns::CONFIG,
		    Columns::SUBVOLUME,
		    Columns::NUMBER,
		    Columns::DEFAULT,
		    Columns::ACTIVE,
		    Columns::DATE,
		    Columns::USER,
		    Columns::USED_SPACE,
		    Columns::DESCRIPTION,
		    Columns::USERDATA
		};

	    if (format == GlobalOptions::OutputFormat::JSON)
		return {
		    Columns::SUBVOLUME,
		    Columns::NUMBER,
		    Columns::DEFAULT,
		    Columns::ACTIVE,
		    Columns::DATE,
		    Columns::USER,
		    Columns::USED_SPACE,
		    Columns::DESCRIPTION,
		    Columns::USERDATA
		};

	    throw logic_error("unknown output format");
	}


	vector<string>
	Command::ListSnapshots::Options::pre_post_mode_columns(GlobalOptions::OutputFormat format) const
	{
	    if (format == GlobalOptions::OutputFormat::TABLE)
		return {
		    Columns::NUMBER,
		    Columns::POST_NUMBER,
		    Columns::DATE,
		    Columns::POST_DATE,
		    Columns::DESCRIPTION,
		    Columns::USERDATA
		};

	    if (format == GlobalOptions::OutputFormat::CSV)
		return {
		    Columns::CONFIG,
		    Columns::SUBVOLUME,
		    Columns::NUMBER,
		    Columns::DEFAULT,
		    Columns::ACTIVE,
		    Columns::POST_NUMBER,
		    Columns::DATE,
		    Columns::POST_DATE,
		    Columns::DESCRIPTION,
		    Columns::USERDATA
		};

	    if (format == GlobalOptions::OutputFormat::JSON)
		return {
		    Columns::SUBVOLUME,
		    Columns::NUMBER,
		    Columns::DEFAULT,
		    Columns::ACTIVE,
		    Columns::POST_NUMBER,
		    Columns::DATE,
		    Columns::POST_DATE,
		    Columns::DESCRIPTION,
		    Columns::USERDATA
		};

	    throw logic_error("unknown output format");
	}


	bool Command::ListSnapshots::Options::wrong_type() const
	{
	    if (!has_option("type"))
		return false;

	    string type = get_option("type")->second;

	    return type != "all" && type != "single" && type != "pre-post";
	}


	bool Command::ListSnapshots::Options::wrong_columns() const
	{
	    return !_columns_option.wrong_columns().empty();
	}


	string Command::ListSnapshots::Options::type_error() const
	{
	    return _("Unknown type of snapshots.");
	}


	string Command::ListSnapshots::Options::columns_error() const
	{
	    return _columns_option.error();
	}

    }
}
