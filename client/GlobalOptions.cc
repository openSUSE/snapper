/*
 * Copyright (c) [2019-2024] SUSE LLC
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

#include "misc.h"
#include "client/GlobalOptions.h"
#include "client/utils/text.h"
#include "client/utils/TableFormatter.h"
#include "client/utils/CsvFormatter.h"


namespace snapper
{

    using namespace std;


    string
    GlobalOptions::help_text()
    {
	return string(_("    Global options:")) + '\n'
	    + _("\t--quiet, -q\t\t\tSuppress normal output.") + '\n'
	    + _("\t--verbose, -v\t\t\tIncrease verbosity.") + '\n'
	    + _("\t--debug\t\t\t\tTurn on debugging.") + '\n'
	    + _("\t--utc\t\t\t\tDisplay dates and times in UTC.") + '\n'
	    + _("\t--iso\t\t\t\tDisplay dates and times in ISO format.") + '\n'
	    + _("\t--table-style, -t <style>\tTable style (integer).") + '\n'
	    + _("\t--abbreviate\t\t\tAllow to abbreviate table columns.") + '\n'
	    + _("\t--machine-readable <format>\tSet a machine-readable output format (csv, json).") + '\n'
	    + _("\t--csvout\t\t\tSet CSV output format.") + '\n'
	    + _("\t--jsonout\t\t\tSet JSON output format.") + '\n'
	    + _("\t--separator <separator>\t\tCharacter separator for CSV output format.") + '\n'
	    + _("\t--no-headers\t\t\tNo headers for CSV output format.") + '\n'
	    + _("\t--config, -c <name>\t\tSet name of config to use.") + '\n'
	    + _("\t--no-dbus\t\t\tOperate without DBus.") + '\n'
	    + _("\t--root, -r <path>\t\tOperate on target root (works only without DBus).") + '\n'
	    + _("\t--ambit, -a ambit\t\tOperate in the specified ambit.") + '\n'
	    + _("\t--version\t\t\tPrint version and exit.") + '\n';
    }


    GlobalOptions::GlobalOptions(GetOpts& parser)
	: _ambit(Ambit::AUTO)
    {
	const vector<Option> options = {
	    Option("quiet",		no_argument,		'q'),
	    Option("verbose",		no_argument,		'v'),
	    Option("debug",		no_argument),
	    Option("utc",		no_argument),
	    Option("iso",		no_argument),
	    Option("table-style",	required_argument,	't'),
	    Option("abbreviate",	no_argument),
	    Option("machine-readable",	required_argument),
	    Option("csvout",		no_argument),
	    Option("jsonout",		no_argument),
	    Option("separator",		required_argument),
	    Option("no-headers",	no_argument),
	    Option("config",		required_argument,	'c'),
	    Option("no-dbus",		no_argument),
	    Option("root",		required_argument,	'r'),
	    Option("ambit",		required_argument,	'a'),
	    Option("version",		no_argument),
	    Option("help",		no_argument,		'h')
	};

	ParsedOpts opts = parser.parse(options);

	check_options(opts);

	_quiet = opts.has_option("quiet");
	_verbose = opts.has_option("verbose");
	_debug = opts.has_option("debug");
	_utc = opts.has_option("utc");
	_iso = opts.has_option("iso");
	_no_dbus = opts.has_option("no-dbus");
	_version = opts.has_option("version");
	_help = opts.has_option("help");
	_table_style = table_style_value(opts);
	_abbreviate = opts.has_option("abbreviate");
	_output_format = output_format_value(opts);
	_headers = !opts.has_option("no-headers");
	_separator = separator_value(opts);
	_config = config_value(opts);
	_root = root_value(opts);
	_ambit = ambit_value(opts);
    }


    void
    GlobalOptions::check_options(const ParsedOpts& opts) const
    {
	if (opts.has_option("root") && !opts.has_option("no-dbus"))
	{
	    string error = _("root argument can be used only together with no-dbus.\n");

	    SN_THROW(OptionsException(error));
	}
    }


    Style
    GlobalOptions::table_style_value(const ParsedOpts& opts) const
    {
	ParsedOpts::const_iterator it = opts.find("table-style");
	if (it == opts.end())
	    return TableFormatter::auto_style();

	try
	{
	    unsigned long value = stoul(it->second);

	    if (value >= Table::num_styles)
		throw exception();

	    return (Style)(value);
	}
	catch (const exception&)
	{
	    string error = sformat(_("Invalid table style '%s'."), it->second.c_str()) + '\n' +
		sformat(_("Use an integer number from %d to %d."), 0, Table::num_styles - 1);

	    SN_THROW(OptionsException(error));
	}

	return TableFormatter::auto_style();
    }


    GlobalOptions::OutputFormat
    GlobalOptions::output_format_value(const ParsedOpts& opts) const
    {
	if (opts.has_option("csvout"))
	    return OutputFormat::CSV;

	if (opts.has_option("jsonout"))
	    return OutputFormat::JSON;

	ParsedOpts::const_iterator it = opts.find("machine-readable");
	if (it == opts.end())
	    return OutputFormat::TABLE;

	OutputFormat output_format;
	if (!toValue(it->second, output_format, false))
	{
	    string error = sformat(_("Invalid machine readable format '%s'."), it->second.c_str()) + '\n' +
		possible_enum_values<OutputFormat>();

	    SN_THROW(OptionsException(error));
	}

	return output_format;
    }


    string
    GlobalOptions::separator_value(const ParsedOpts& opts) const
    {
	ParsedOpts::const_iterator it = opts.find("separator");
	if (it == opts.end())
	    return CsvFormatter::default_separator;

	return it->second;
    }


    string
    GlobalOptions::config_value(const ParsedOpts& opts) const
    {
	ParsedOpts::const_iterator it = opts.find("config");
	if (it == opts.end())
	    return "root";

	return it->second;
    }


    string
    GlobalOptions::root_value(const ParsedOpts& opts) const
    {
	ParsedOpts::const_iterator it = opts.find("root");
	if (it == opts.end())
	    return "/";

	return it->second;
    }


    GlobalOptions::Ambit
    GlobalOptions::ambit_value(const ParsedOpts& opts) const
    {
	ParsedOpts::const_iterator it = opts.find("ambit");
	if (it == opts.end())
	    return Ambit::AUTO;

	Ambit ambit;
	if (!toValue(it->second, ambit, false))
	{
	    string error = sformat(_("Invalid ambit '%s'."), it->second.c_str()) + '\n' +
		possible_enum_values<Ambit>();

	    SN_THROW(OptionsException(error));
	}

	return ambit;
    }


    const vector<string> EnumInfo<GlobalOptions::OutputFormat>::names({ "table", "csv", "json" });

    const vector<string> EnumInfo<GlobalOptions::Ambit>::names({ "auto", "classic", "transactional" });

}
