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

	    const string DEFAULT_CONFIG = "root";

	    const string DEFAULT_ROOT = "/";

	    const vector<Option> OPTIONS = {
		Option("quiet",			no_argument,		'q'),
		Option("verbose",		no_argument,		'v'),
		Option("utc",			no_argument),
		Option("iso",			no_argument),
		Option("table-style",		required_argument,	't'),
		Option("machine-readable",	required_argument),
		Option("csvout",		no_argument),
		Option("jsonout",		no_argument),
		Option("separator",		required_argument),
		Option("config",		required_argument,	'c'),
		Option("no-dbus",		no_argument),
		Option("root",			required_argument,	'r'),
		Option("ambit",			required_argument,	'a'),
		Option("version",		no_argument),
		Option("help",			no_argument,		'h')
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
		+ _("\t--ambit, -a ambit\t\tOperate in the specified ambit.") + '\n'
		+ _("\t--version\t\t\tPrint version and exit.") + '\n';
	}


	GlobalOptions::GlobalOptions(GetOpts& parser)
	    : Options(parser), _ambit(Ambit::AUTO)
	{
	    parse_options();
	    check_options();

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
	    _ambit = ambit_value();
	}


	void GlobalOptions::parse_options()
	{
	    _options = _parser.parse(OPTIONS);
	}


	void
	GlobalOptions::check_options() const
	{
	    if (has_option("root") && !has_option("no-dbus"))
	    {
		string error =_("root argument can be used only together with no-dbus.");

		SN_THROW(OptionsException(error));
	    }
	}


	TableLineStyle
	GlobalOptions::table_style_value() const
	{
	    if (!has_option("table-style"))
		return TableFormatter::default_style();

	    string str = get_argument("table-style");

	    unsigned long value = stoul(str);
	    if (value >= Table::numStyles)
	    {
		string error = sformat(_("Invalid table style %s."), str.c_str()) + '\n' +
		    sformat(_("Use an integer number from %d to %d."), 0, Table::numStyles - 1);

		SN_THROW(OptionsException(error));
	    }

	    return (TableLineStyle)(value);
	}


	GlobalOptions::OutputFormat
	GlobalOptions::output_format_value() const
	{
	    if (has_option("csvout"))
		return OutputFormat::CSV;

	    if (has_option("jsonout"))
		return OutputFormat::JSON;

	    if (!has_option("machine-readable"))
		return OutputFormat::TABLE;

	    string str = get_argument("machine-readable");

	    OutputFormat output_format;
	    if (!toValue(str, output_format, false))
	    {
		string error = sformat(_("Invalid machine readable format %s."), str.c_str()) + '\n' +
		    sformat(_("Use %s, %s or %s."), toString(OutputFormat::TABLE).c_str(),
			    toString(OutputFormat::CSV).c_str(), toString(OutputFormat::JSON).c_str());

		SN_THROW(OptionsException(error));
	    }

	    return output_format;
	}


	string
	GlobalOptions::separator_value() const
	{
	    if (has_option("separator"))
		return get_argument("separator");

	    return CsvFormatter::default_separator();
	}


	string
	GlobalOptions::config_value() const
	{
	    if (has_option("config"))
		return get_argument("config");

	    return DEFAULT_CONFIG;
	}


	string
	GlobalOptions::root_value() const
	{
	    if (has_option("root"))
		return get_argument("root");

	    return DEFAULT_ROOT;
	}


	GlobalOptions::Ambit
	GlobalOptions::ambit_value() const
	{
	    if (!has_option("ambit"))
		return Ambit::AUTO;

	    string str = get_argument("ambit");

	    Ambit ambit;
	    if (!toValue(str, ambit, false))
	    {
		string error = sformat(_("Invalid ambit %s."), str.c_str()) + '\n' +
		    sformat(_("Use %s, %s or %s."), toString(Ambit::AUTO).c_str(),
			    toString(Ambit::CLASSIC).c_str(), toString(Ambit::TRANSACTIONAL).c_str());

		SN_THROW(OptionsException(error));
	    }

	    return ambit;
	}

    }

    const vector<string> EnumInfo<cli::GlobalOptions::OutputFormat>::names({ "table", "csv", "json" });

    const vector<string> EnumInfo<cli::GlobalOptions::Ambit>::names({ "auto", "classic", "transactional" });

}
