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

#include <snapper/AppUtil.h>

#include "client/Command/ListSnapshots/Options.h"
#include "client/utils/text.h"

using namespace std;

namespace snapper
{
    namespace cli
    {

	namespace
	{
	    const vector<Option> OPTIONS = {
		Option("type",			required_argument,	't'),
		Option("disable-used-space",	no_argument),
		Option("all-configs",		no_argument,		'a'),
		Option("columns", 		required_argument)
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


	Command::ListSnapshots::Options::ListMode
	Command::ListSnapshots::Options::list_mode_value() const
	{
	    if (!has_option("type"))
		return ListMode::ALL;

	    string str = get_argument("type");

	    ListMode list_mode;
	    if (!toValue(str, list_mode, false))
	    {
		string error = sformat(_("Unknown snapshots type %s."), str.c_str()) + '\n' +
		    sformat(_("Use %s, %s or %s."), toString(ListMode::ALL).c_str(),
			    toString(ListMode::SINGLE).c_str(), toString(ListMode::PRE_POST).c_str());

		SN_THROW(OptionsException(error));
	    }

	    return list_mode;
	}


	string Command::ListSnapshots::Options::columns_raw() const
	{
	    return has_option("columns") ? get_argument("columns") : "";
	}


	vector<string>
	Command::ListSnapshots::Options::list_mode_columns(GlobalOptions::OutputFormat format) const
	{
	    vector<string> columns;

	    switch (list_mode())
	    {
		case ListMode::ALL:
		    columns = all_mode_columns(format);
		    break;

		case ListMode::SINGLE:
		    columns = single_mode_columns(format);
		    break;

		case ListMode::PRE_POST:
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

    }

    const vector<string> EnumInfo<cli::Command::ListSnapshots::Options::ListMode>::names({ "all", "single", "pre-post" });

}
