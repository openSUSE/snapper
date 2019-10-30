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

#include <snapper/AppUtil.h>

#include "client/GlobalOptions.h"
#include "client/utils/text.h"
#include "client/utils/TableFormatter.h"
#include "client/utils/CsvFormatter.h"

using namespace std;

namespace snapper
{
    namespace cli
    {

	namespace
	{

	    const string CSV_OPTION = "csv";

	    const string JSON_OPTION = "json";

	    const string DEFAULT_CONFIG = "root";

	    const string DEFAULT_ROOT = "/";

	    const option OPTIONS[15] = {
		{ "quiet",		no_argument,		0,	'q' },
		{ "verbose",		no_argument,		0,	'v' },
		{ "utc",		no_argument,		0,	0 },
		{ "iso",		no_argument,		0,	0 },
		{ "table-style",	required_argument,	0,	't' },
		{ "machine-readable",	required_argument,	0,	0},
		{ "csvout",		no_argument,		0,	0},
		{ "jsonout",		no_argument,		0,	0},
		{ "separator",		required_argument,	0,	0},
		{ "config",		required_argument,	0,	'c' },
		{ "no-dbus",		no_argument,		0,	0 },
		{ "root",		required_argument,	0,	'r' },
		{ "version",		no_argument,		0,	0 },
		{ "help",		no_argument,		0,	'h' },
		{ 0, 0, 0, 0 }
	    };

	}


	string GlobalOptions::help_text()
	{
	    return string(_("    Global options:")) + '\n'
		+ _("\t--quiet, -q\t\t\tSuppress normal output.") + '\n'
		+ _("\t--verbose, -v\t\t\tIncrease verbosity.") + '\n'
		+ _("\t--utc\t\t\t\tDisplay dates and times in UTC.") + '\n'
		+ _("\t--iso\t\t\t\tDisplay dates and times in ISO format.") + '\n'
		+ _("\t--table-style, -t <style>\tTable style (integer).") + '\n'
		+ _("\t--machine-readable <format>\tSet a machine-readable output format (csv, json).") + '\n'
		+ _("\t--csvout\t\t\tSet CSV output format.") + '\n'
		+ _("\t--jsonout\t\t\tSet JSON output format.") + '\n'
		+ _("\t--separator <separator>\t\tCharacter separator for CSV output format.") + '\n'
		+ _("\t--config, -c <name>\t\tSet name of config to use.") + '\n'
		+ _("\t--no-dbus\t\t\tOperate without DBus.") + '\n'
		+ _("\t--root, -r <path>\t\tOperate on target root (works only without DBus).") + '\n'
		+ _("\t--version\t\t\tPrint version and exit.") + '\n';
	}


	GlobalOptions::GlobalOptions(GetOpts& parser) :
	    Options(parser)
	{
	    parse_options();

	    _quiet = has_option("quiet");
	    _verbose = has_option("verbose");
	    _utc = has_option("utc");
	    _iso = has_option("iso");
	    _no_dbus = has_option("no-dbus");
	    _version = has_option("version");
	    _help = has_option("help");
	    _table_style = table_style_value();
	    _output_format = output_format_value();
	    _separator = separator_value();
	    _config = config_value();
	    _root = root_value();
	}


	void GlobalOptions::parse_options()
	{
	    _options = _parser.parse(OPTIONS);
	}


	vector<string> GlobalOptions::errors() const
	{
	    vector<string> detected_errors;

	    if (wrong_table_style())
		detected_errors.push_back(table_style_error());

	    if (wrong_machine_readable())
		detected_errors.push_back(machine_readable_error());

	    if (missing_no_dbus())
		detected_errors.push_back(missing_no_dbus_error());

	    return detected_errors;
	}


	TableLineStyle GlobalOptions::table_style_value() const
	{
	    if (has_option("table-style") && !wrong_table_style())
		return (TableLineStyle) table_style_raw();

	    return TableFormatter::default_style();
	}


	GlobalOptions::OutputFormat GlobalOptions::output_format_value() const
	{
	    if (has_option("csvout") || machine_readable_value() == CSV_OPTION)
		return OutputFormat::CSV;

	    if (has_option("jsonout") || machine_readable_value() == JSON_OPTION)
		return OutputFormat::JSON;

	    return OutputFormat::TABLE;
	}


	string GlobalOptions::machine_readable_value() const
	{
	    if (has_option("machine-readable"))
		return get_option("machine-readable")->second;

	    return "";
	}


	string GlobalOptions::separator_value() const
	{
	    if (has_option("separator"))
		return get_option("separator")->second;

	    return CsvFormatter::default_separator();
	}


	string GlobalOptions::config_value() const
	{
	    if (has_option("config"))
		return get_option("config")->second;

	    return DEFAULT_CONFIG;
	}


	string GlobalOptions::root_value() const
	{
	    if (has_option("root"))
		return get_option("root")->second;

	    return DEFAULT_ROOT;
	}


	unsigned int GlobalOptions::table_style_raw() const
	{
	    return stoul(get_option("table-style")->second);
	}


	bool GlobalOptions::wrong_table_style() const
	{
	    if (!has_option("table-style"))
		return false;

	    if (table_style_raw() >= Table::numStyles)
		return true;

	    return false;
	}


	bool GlobalOptions::wrong_machine_readable() const
	{
	    if (!has_option("machine-readable"))
		return false;

	    string value = machine_readable_value();

	    if (value != CSV_OPTION && value != JSON_OPTION)
		return true;

	    return false;
	}


	bool GlobalOptions::missing_no_dbus() const
	{
	    return has_option("root") && !has_option("no-dbus");
	}


	string GlobalOptions::table_style_error() const
	{
	    return sformat(_("Invalid table style %d."), table_style_raw()) + " " +
		   sformat(_("Use an integer number from %d to %d."), 0, Table::numStyles - 1);
	}


	string GlobalOptions::machine_readable_error() const
	{
	    string format = machine_readable_value();

	    return sformat(_("Invalid machine readable format %s."), format.c_str()) + " " +
		   sformat(_("Use %s or %s."), CSV_OPTION.c_str(), JSON_OPTION.c_str());
	}


	string GlobalOptions::missing_no_dbus_error() const
	{
	    return _("root argument can be used only together with no-dbus.\n"
		     "Try 'snapper --help' for more information.");
	}

    }
}
