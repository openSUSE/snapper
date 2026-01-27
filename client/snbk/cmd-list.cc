/*
 * Copyright (c) 2024 SUSE LLC
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

#include "snapper/AppUtil.h"
#include "snapper/SnapperDefines.h"

#include "../utils/text.h"
#include "../utils/TableFormatter.h"
#include "../utils/CsvFormatter.h"
#include "../utils/JsonFormatter.h"
#include "../utils/OutputOptions.h"

// a collision with client/proxy/errors.h
#ifdef error_description
#undef error_description
#endif

#include "../proxy/proxy.h"
#include "../proxy/errors.h"

#include "BackupConfig.h"
#include "GlobalOptions.h"
#include "TheBigThing.h"


namespace snapper
{

    using namespace std;


    void
    help_list()
    {
	cout << "  " << _("List:") << '\n'
	     << "\t" << _("snbk list") << '\n'
	     << '\n';
    }


    namespace
    {

	enum class Column
	{
	    NAME, NUMBER, DATE, SOURCE_STATE, SOURCE_UUID, SOURCE_PARENT_UUID, SOURCE_RECEIVED_UUID,
	    SOURCE_CREATION_TIME, TARGET_STATE, TARGET_UUID, TARGET_PARENT_UUID, TARGET_RECEIVED_UUID,
	    TARGET_CREATION_TIME
	};


	Cell
	header_for(Column column)
	{
	    switch (column)
	    {
		case Column::NAME:
		    return Cell(_("Name"));

		case Column::NUMBER:
		    return Cell(_("#"), Id::NUMBER, Align::RIGHT);

		case Column::DATE:
		    return Cell(_("Date"));

		case Column::SOURCE_STATE:
		    return Cell(_("Source State"));

		case Column::SOURCE_UUID:
		    return Cell(_("(Source) UUID"));

		case Column::SOURCE_PARENT_UUID:
		    return Cell(_("(Source) Parent UUID"));

		case Column::SOURCE_RECEIVED_UUID:
		    return Cell(_("(Source) Received UUID"));

		case Column::SOURCE_CREATION_TIME:
		    return Cell(_("(Source) Creation Time"));

		case Column::TARGET_STATE:
		    return Cell(_("Target State"));

		case Column::TARGET_UUID:
		    return Cell(_("(Target) UUID"));

		case Column::TARGET_PARENT_UUID:
		    return Cell(_("(Target) Parent UUID"));

		case Column::TARGET_RECEIVED_UUID:
		    return Cell(_("(Target) Received UUID"));

		case Column::TARGET_CREATION_TIME:
		    return Cell(_("(Target) Creation Time"));
	    }

	    SN_THROW(Exception("invalid column value"));
	    __builtin_unreachable();
	}


	std::any
	value_for_as_any(const OutputOptions& output_options, Column column, const BackupConfig& backup_config,
			 const TheBigThing& the_big_thing)
	{
	    const string::size_type uuid_cutoff = 14;

	    switch (column)
	    {
		case Column::NAME:
		    return backup_config.name;

		case Column::NUMBER:
		    return the_big_thing.num;

		case Column::DATE:
		    if (the_big_thing.date == 0)
			return nullptr;
		    return datetime(the_big_thing.date, output_options.utc, output_options.iso);

		case Column::SOURCE_STATE:
		    if (the_big_thing.source_state == TheBigThing::SourceState::MISSING)
			return nullptr;
		    return toString(the_big_thing.source_state);

		case Column::SOURCE_UUID:
		    if (the_big_thing.source_uuid.empty())
			return nullptr;
		    return the_big_thing.source_uuid.substr(0, uuid_cutoff);

		case Column::SOURCE_PARENT_UUID:
		    if (the_big_thing.source_parent_uuid.empty())
			return nullptr;
		    return the_big_thing.source_parent_uuid.substr(0, uuid_cutoff);

		case Column::SOURCE_RECEIVED_UUID:
		    if (the_big_thing.source_received_uuid.empty())
			return nullptr;
		    return the_big_thing.source_received_uuid.substr(0, uuid_cutoff);

		case Column::SOURCE_CREATION_TIME:
		    return the_big_thing.source_creation_time;

		case Column::TARGET_STATE:
		    if (the_big_thing.target_state == TheBigThing::TargetState::MISSING)
			return nullptr;
		    return toString(the_big_thing.target_state);

		case Column::TARGET_UUID:
		    if (the_big_thing.target_uuid.empty())
			return nullptr;
		    return the_big_thing.target_uuid.substr(0, uuid_cutoff);

		case Column::TARGET_PARENT_UUID:
		    if (the_big_thing.target_parent_uuid.empty())
			return nullptr;
		    return the_big_thing.target_parent_uuid.substr(0, uuid_cutoff);

		case Column::TARGET_RECEIVED_UUID:
		    if (the_big_thing.target_received_uuid.empty())
			return nullptr;
		    return the_big_thing.target_received_uuid.substr(0, uuid_cutoff);

		case Column::TARGET_CREATION_TIME:
		    return the_big_thing.target_creation_time;
	    }

	    SN_THROW(Exception("invalid column value"));
	    __builtin_unreachable();
	}


	string
	value_for_as_string(const OutputOptions& output_options, Column column, const BackupConfig& backup_config,
			    const TheBigThing& the_big_thing)
	{
	    return any_to_string(output_options, value_for_as_any(output_options, column, backup_config,
								  the_big_thing));
	}


	json_object*
	value_for_as_json(const OutputOptions& output_options, Column column, const BackupConfig& backup_config,
			  const TheBigThing& the_big_thing)
	{
	    return any_to_json(output_options, value_for_as_any(output_options, column, backup_config,
								the_big_thing));
	}


	// The loop over the backup-configs could be moved from the three output_*
	// functions to command_list. One disadvantage is that the output for the table
	// mode is delayed until all backup-configs have been probed.


	void
	output_table(const GlobalOptions& global_options, const vector<Column>& columns,
		     const BackupConfigs& backup_configs, ProxySnappers* snappers)
	{
	    OutputOptions output_options(global_options.utc(), global_options.iso(), true);

	    unsigned int errors = 0;

	    bool first_table = true;

	    for (const BackupConfig& backup_config : backup_configs)
	    {
		if (!first_table)
		    cout << endl;

		if (backup_configs.size() > 1)
		{
		    cout << "Backup-config:" << backup_config.name << ", config:" << backup_config.config
			 << ", source-path:" << backup_config.source_path << ", target-mode:"
			 << toString(backup_config.target_mode) << endl;
		}

		try
		{
		    TheBigThings the_big_things(backup_config, snappers, global_options.verbose());

		    TableFormatter formatter(global_options.table_style());

		    for (Column column : columns)
			formatter.header().push_back(header_for(column));

		    for (const TheBigThing& the_big_thing : the_big_things)
		    {
			vector<string> row;

			for (Column column : columns)
			    row.push_back(value_for_as_string(output_options, column, backup_config, the_big_thing));

			formatter.rows().push_back(row);
		    }

		    first_table = false;

		    cout << formatter;
		}
		catch (const DBus::ErrorException& e)
		{
		    SN_CAUGHT(e);

		    cerr << error_description(e) << endl;

		    ++errors;
		}
		catch (const Exception& e)
		{
		    SN_CAUGHT(e);

		    cerr << e.what() << endl;

		    ++errors;
		}
	    }

	    if (errors != 0)
	    {
		string error = sformat(_("Running list failed for %d of %ld backup config.",
					 "Running list failed for %d of %ld backup configs.",
					 backup_configs.size()), errors, backup_configs.size());

		SN_THROW(Exception(error));
	    }
	}


	void
	output_csv(const GlobalOptions& global_options, const vector<Column>& columns,
		   const BackupConfigs& backup_configs, ProxySnappers* snappers)
	{
	    OutputOptions output_options(global_options.utc(), global_options.iso(), false);

	    CsvFormatter formatter(global_options.separator(), global_options.headers());

	    for (Column column : columns)
		formatter.header().push_back(toString(column));

	    for (const BackupConfig& backup_config : backup_configs)
	    {
		TheBigThings the_big_things(backup_config, snappers, global_options.verbose());

		for (const TheBigThing& the_big_thing : the_big_things)
		{
		    vector<string> row;

		    for (Column column : columns)
			row.push_back(value_for_as_string(output_options, column, backup_config, the_big_thing));

		    formatter.rows().push_back(row);
		}
	    }

	    cout << formatter;
	}


	void
	output_json(const GlobalOptions& global_options, const vector<Column>& columns,
		    const BackupConfigs& backup_configs, ProxySnappers* snappers)
	{
	    OutputOptions output_options(global_options.utc(), global_options.iso(), false);

	    JsonFormatter formatter;

	    json_object* json_snapshots = json_object_new_array();
	    json_object_object_add(formatter.root(), "snapshots", json_snapshots);

	    for (const BackupConfig& backup_config : backup_configs)
	    {
		TheBigThings the_big_things(backup_config, snappers, global_options.verbose());

		for (const TheBigThing& the_big_thing : the_big_things)
		{
		    json_object* json_snapshot = json_object_new_object();
		    json_object_array_add(json_snapshots, json_snapshot);

		    for (const Column& column : columns)
		    {
			json_object* tmp =  value_for_as_json(output_options, column, backup_config, the_big_thing);
			if (tmp)
			    json_object_object_add(json_snapshot, toString(column).c_str(), tmp);
		    }
		}
	    }

	    cout << formatter;
	}

    }


    void
    command_list(const GlobalOptions& global_options, GetOpts& get_opts, const BackupConfigs& backup_configs,
		 ProxySnappers* snappers)
    {
	ParsedOpts opts = get_opts.parse("list", GetOpts::no_options);

	vector<Column> columns = { Column::NUMBER, Column::DATE, Column::SOURCE_STATE, Column::TARGET_STATE };

	if (global_options.output_format() != GlobalOptions::OutputFormat::TABLE)
	    columns.insert(columns.begin(), Column::NAME);

	if (get_opts.has_args())
	{
	    SN_THROW(OptionsException(_("Command 'list' does not take arguments.")));
	}

	switch (global_options.output_format())
	{
	    case GlobalOptions::OutputFormat::TABLE:
		output_table(global_options, columns, backup_configs, snappers);
		break;

	    case GlobalOptions::OutputFormat::CSV:
		output_csv(global_options, columns, backup_configs, snappers);
		break;

	    case GlobalOptions::OutputFormat::JSON:
		output_json(global_options, columns, backup_configs, snappers);
		break;
	}
    }


    template <> struct EnumInfo<Column> { static const vector<string> names; };

    const vector<string> EnumInfo<Column>::names({
	"name", "number", "date", "source-state", "source-uuid", "source-parent-uuid", "source-received-uuid",
	"source-creation-time", "target-state", "target-uuid", "target-parent-uuid", "target-received-uuid",
	"target-creation-time"
    });

}
