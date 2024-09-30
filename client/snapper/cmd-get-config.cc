/*
 * Copyright (c) [2011-2015] Novell, Inc.
 * Copyright (c) [2016-2020] SUSE LLC
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


#include <iostream>

#include "../utils/text.h"
#include "../utils/help.h"
#include "../proxy/proxy.h"
#include "GlobalOptions.h"
#include "../misc.h"
#include "../utils/TableFormatter.h"
#include "../utils/CsvFormatter.h"
#include "../utils/JsonFormatter.h"


namespace snapper
{

    using namespace std;


    void
    help_get_config()
    {
	cout << _("  Get config:") << '\n'
	     << _("\tsnapper get-config") << '\n'
	     << '\n'
	     << _("    Options for 'get-config' command:") << '\n';

	print_options({
	    { _("--columns <columns>"), _("Columns to show separated by comma.") }
	});
    }


    namespace
    {

	enum class Column
	{
	    KEY, VALUE
	};


	Cell
	header_for(Column column)
	{
	    switch (column)
	    {
		case Column::KEY:
		    return Cell(_("Key"));

		case Column::VALUE:
		    return Cell(_("Value"));
	    }

	    SN_THROW(Exception("invalid column value"));
	    __builtin_unreachable();
	}


	string
	value_for_as_string(Column column, const pair<const string, string>& value)
	{
	    switch (column)
	    {
		case Column::KEY:
		    return value.first;

		case Column::VALUE:
		    return value.second;
	    }

	    SN_THROW(Exception("invalid column value"));
	    __builtin_unreachable();
	}


	void
	output_table(const vector<Column>& columns, Style style, ProxySnapper* snapper)
	{
	    TableFormatter formatter(style);

	    for (Column column : columns)
		formatter.header().push_back(header_for(column));

	    ProxyConfig config = snapper->getConfig();
	    for (const map<string, string>::value_type& value : config.getAllValues())
	    {
		vector<string> row;

		for (Column column : columns)
		    row.push_back(value_for_as_string(column, value));

		formatter.rows().push_back(row);
	    }

	    cout << formatter;
	}


	void
	output_csv(const GlobalOptions& global_options, const vector<Column>& columns, const string& separator,
		   ProxySnapper* snapper)
	{
	    CsvFormatter formatter(separator, global_options.headers());

	    for (Column column : columns)
		formatter.header().push_back(toString(column));

	    ProxyConfig config = snapper->getConfig();
	    for (const map<string, string>::value_type& value : config.getAllValues())
	    {
		vector<string> row;

		for (Column column : columns)
		    row.push_back(value_for_as_string(column, value));

		formatter.rows().push_back(row);
	    }

	    cout << formatter;
	}


	void
	output_json(const vector<Column>& columns, ProxySnapper* snapper)
	{
	    JsonFormatter formatter;

	    ProxyConfig config = snapper->getConfig();
	    for (const map<string, string>::value_type& value : config.getAllValues())
	    {
		json_object_object_add(formatter.root(), value_for_as_string(Column::KEY, value).c_str(),
				       json_object_new_string(value_for_as_string(Column::VALUE, value).c_str()));
	    }

	    cout << formatter;
	}

    }


    void
    command_get_config(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers*,
		       ProxySnapper* snapper, Plugins::Report& report)
    {
	const vector<Option> options = {
	    Option("columns",	required_argument)
	};

	ParsedOpts opts = get_opts.parse("get-config", options);

	vector<Column> columns = { Column::KEY, Column::VALUE };

	ParsedOpts::const_iterator opt;

	if ((opt = opts.find("columns")) != opts.end())
	{
	    columns = parse_columns<Column>(opt->second);
	}

	if (get_opts.has_args())
	{
	    SN_THROW(OptionsException(_("Command 'get-config' does not take arguments.")));
	}

	switch (global_options.output_format())
	{
	    case GlobalOptions::OutputFormat::TABLE:
		output_table(columns, global_options.table_style(), snapper);
		break;

	    case GlobalOptions::OutputFormat::CSV:
		output_csv(global_options, columns, global_options.separator(), snapper);
		break;

	    case GlobalOptions::OutputFormat::JSON:
		output_json(columns, snapper);
		break;
	}
    }


    template <> struct EnumInfo<Column> { static const vector<string> names; };

    const vector<string> EnumInfo<Column>::names({ "key", "value" });

}
