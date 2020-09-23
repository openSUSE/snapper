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

#include "utils/text.h"
#include "GlobalOptions.h"
#include "proxy.h"
#include "misc.h"
#include "utils/TableFormatter.h"
#include "utils/CsvFormatter.h"
#include "utils/JsonFormatter.h"


namespace snapper
{

    using namespace std;


    void
    help_list_configs()
    {
	cout << _("  List configs:") << '\n'
	     << _("\tsnapper list-configs") << '\n'
	     << '\n'
	     << _("    Options for 'list-configs' command:\n"
		  "\t--columns <columns>\t\tColumns to show separated by comma.\n"
		  "\t\t\t\t\tPossible columns: config, subvolume.\n")
	     << endl;
    }


    namespace
    {

	enum class Column
	{
	    CONFIG, SUBVOLUME
	};


	pair<string, TableAlign>
	header_for(Column column)
	{
	    switch (column)
	    {
		case Column::CONFIG:
		    return make_pair(_("Config"), TableAlign::LEFT);

		case Column::SUBVOLUME:
		    return make_pair(_("Subvolume"), TableAlign::LEFT);
	    }

	    SN_THROW(Exception("invalid column value"));
	    __builtin_unreachable();
	}


	string
	value_for_as_string(Column column, const pair<const string, ProxyConfig>& value)
	{
	    switch (column)
	    {
		case Column::CONFIG:
		    return value.first;

		case Column::SUBVOLUME:
		    return value.second.getSubvolume();
	    }

	    SN_THROW(Exception("invalid column value"));
	    __builtin_unreachable();
	}


	void
	output_table(const vector<Column>& columns, TableStyle style, ProxySnappers* snappers)
	{
	    TableFormatter formatter(style);

	    for (Column column : columns)
		formatter.header().push_back(header_for(column));

	    map<string, ProxyConfig> configs = snappers->getConfigs();
	    for (const map<string, ProxyConfig>::value_type value : configs)
	    {
		vector<string> row;

		for (Column column : columns)
		    row.push_back(value_for_as_string(column, value));

		formatter.rows().push_back(row);
	    }

	    cout << formatter;
	}


	void
	output_csv(const vector<Column>& columns, const string& separator, ProxySnappers* snappers)
	{
	    CsvFormatter formatter(separator);

	    for (Column column : columns)
		formatter.header().push_back(toString(column));

	    map<string, ProxyConfig> configs = snappers->getConfigs();
	    for (const map<string, ProxyConfig>::value_type value : configs)
	    {
		vector<string> row;

		for (Column column : columns)
		    row.push_back(value_for_as_string(column, value));

		formatter.rows().push_back(row);
	    }

	    cout << formatter;
	}


	void
	output_json(const vector<Column>& columns, ProxySnappers* snappers)
	{
	    JsonFormatter formatter;

	    json_object* json_configs = json_object_new_array();
	    json_object_object_add(formatter.root(), "configs", json_configs);

	    map<string, ProxyConfig> configs = snappers->getConfigs();
	    for (const map<string, ProxyConfig>::value_type value : configs)
	    {
		json_object* json_config = json_object_new_object();
		json_object_array_add(json_configs, json_config);

		for (const Column& column : columns)
		    json_object_object_add(json_config, toString(column).c_str(),
					   json_object_new_string(value_for_as_string(column, value).c_str()));
	    }

	    cout << formatter;
	}

    }


    void
    command_list_configs(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers* snappers, ProxySnapper*)
    {
	const vector<Option> options = {
	    Option("columns",	required_argument)
	};

	ParsedOpts opts = get_opts.parse("list-configs", options);

	vector<Column> columns = { Column::CONFIG, Column::SUBVOLUME };

	ParsedOpts::const_iterator opt;

	if ((opt = opts.find("columns")) != opts.end())
	{
	    columns = parse_columns<Column>(opt->second);
	}

	if (get_opts.has_args())
	{
	    SN_THROW(OptionsException(_("Command 'list-configs' does not take arguments.")));
	}

	switch (global_options.output_format())
	{
	    case GlobalOptions::OutputFormat::TABLE:
		output_table(columns, global_options.table_style(), snappers);
		break;

	    case GlobalOptions::OutputFormat::CSV:
		output_csv(columns, global_options.separator(), snappers);
		break;

	    case GlobalOptions::OutputFormat::JSON:
		output_json(columns, snappers);
		break;
	}
    }


    template <> struct EnumInfo<Column> { static const vector<string> names; };

    const vector<string> EnumInfo<Column>::names({ "config", "subvolume" });

}
