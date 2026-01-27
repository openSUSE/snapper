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


#include "../utils/help.h"
#include "../misc.h"
#include "client/utils/text.h"
#include "client/utils/TableFormatter.h"
#include "client/utils/CsvFormatter.h"

#include "GlobalOptions.h"


namespace snapper
{

    using namespace std;


    void
    GlobalOptions::help_global_options()
    {
	cout << "    " << _("Global options:") << '\n';

	print_options({
	    { _("--quiet, -q"), _("Suppress normal output.") },
	    { _("--verbose, -v"), _("Increase verbosity.") },
	    { _("--debug"), _("Turn on debugging.") },
	    { _("--target-mode <target-mode>"), _("Only use backup-config with specified target-mode.") },
	    { _("--automatic"), _("Only use backup-config with automatic set.") },
	    { _("--utc"), _("Display dates and times in UTC.") },
	    { _("--iso"), _("Display dates and times in ISO format.") },
	    { _("--table-style, -t <style>"), _("Table style (integer).") },
	    { _("--machine-readable <format>"), _("Set a machine-readable output format (csv, json).") },
	    { _("--csvout"), _("Set CSV output format.") },
	    { _("--jsonout"), _("Set JSON output format.") },
	    { _("--separator <separator>"), _("Character separator for CSV output format.") },
	    { _("--no-headers"), _("No headers for CSV output format.") },
	    { _("--backup-config, -b <name>"), _("Set name of backup-config to use.") },
	    { _("--no-dbus"), _("Operate without DBus.") },
	    { _("--version"), _("Print version and exit.") }
	});
    }


    GlobalOptions::GlobalOptions(GetOpts& parser)
    {
	const vector<Option> options = {
	    Option("quiet",			no_argument,		'q'),
	    Option("verbose",			no_argument,		'v'),
	    Option("debug",			no_argument),
	    Option("target-mode",		required_argument),
	    Option("automatic",			no_argument),
	    Option("utc",			no_argument),
	    Option("iso",			no_argument),
	    Option("table-style",		required_argument,	't'),
	    Option("machine-readable",		required_argument),
	    Option("csvout",			no_argument),
	    Option("jsonout",			no_argument),
	    Option("separator",			required_argument),
	    Option("no-headers",		no_argument),
	    Option("backup-config",		required_argument,	'b'),
	    Option("no-dbus",			no_argument),
	    Option("version",			no_argument),
	    Option("help",			no_argument,		'h')
	};

	ParsedOpts opts = parser.parse(options);

	check_options(opts);

	_quiet = opts.has_option("quiet");
	_verbose = opts.has_option("verbose");
	_debug = opts.has_option("debug");
	_target_mode = only_target_mode_value(opts);
	_automatic = opts.has_option("automatic");
	_utc = opts.has_option("utc");
	_iso = opts.has_option("iso");
	_no_dbus = opts.has_option("no-dbus");
	_version = opts.has_option("version");
	_help = opts.has_option("help");
	_table_style = table_style_value(opts);
	_output_format = output_format_value(opts);
	_headers = !opts.has_option("no-headers");
	_separator = separator_value(opts);
	_backup_config = backup_config_value(opts);
    }


    void
    GlobalOptions::check_options(const ParsedOpts& opts) const
    {
    }


    std::optional<BackupConfig::TargetMode>
    GlobalOptions::only_target_mode_value(const ParsedOpts& opts) const
    {
	ParsedOpts::const_iterator it = opts.find("target-mode");
	if (it == opts.end())
	    return std::optional<BackupConfig::TargetMode>();

	BackupConfig::TargetMode target_mode;

	if (!toValue(it->second, target_mode, false))
	{
	    string error = sformat(_("Invalid target mode '%s'."), it->second.c_str()) + '\n';
	    SN_THROW(OptionsException(error));
	}

	return target_mode;
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


    std::optional<string>
    GlobalOptions::backup_config_value(const ParsedOpts& opts) const
    {
	ParsedOpts::const_iterator it = opts.find("backup-config");
	if (it == opts.end())
	    return std::optional<string>();

	return it->second;
    }


    const vector<string> EnumInfo<GlobalOptions::OutputFormat>::names({ "table", "csv", "json" });

}
