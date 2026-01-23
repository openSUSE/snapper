/*
 * Copyright (c) [2024-2025] SUSE LLC
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
#include "../utils/TableFormatter.h"
#include "../utils/CsvFormatter.h"
#include "../utils/JsonFormatter.h"
#include "../utils/OutputOptions.h"
#include "../proxy/proxy.h"

#include "BackupConfig.h"
#include "GlobalOptions.h"


namespace snapper
{

    using namespace std;


    void
    help_list_configs()
    {
	cout << "  " << _("List configs:") << '\n'
	     << "\t" << _("snbk list-configs") << '\n'
	     << '\n';
    }


    namespace
    {

	enum class Column
	{
	    NAME, CONFIG, TARGET_MODE, AUTOMATIC, SOURCE_PATH, TARGET_PATH, SSH_HOST,
	    SSH_USER, SSH_PORT, SSH_IDENTITY, TARGET_BTRFS_BIN, TARGET_LS_BIN,
	    TARGET_MKDIR_BIN, TARGET_RM_BIN, TARGET_RMDIR_BIN
	};


	Cell
	header_for(Column column)
	{
	    switch (column)
	    {
		case Column::NAME:
		    return Cell(_("Name"));

		case Column::CONFIG:
		    return Cell(_("Config"));

		case Column::TARGET_MODE:
		    return Cell(_("Target Mode"));

		case Column::AUTOMATIC:
		    return Cell(_("Automatic"));

		case Column::SOURCE_PATH:
		    return Cell(_("Source Path"));

		case Column::TARGET_PATH:
		    return Cell(_("Target Path"));

		case Column::SSH_HOST:
		    return Cell(_("SSH Host"), Id::SSH_HOST);

		case Column::SSH_USER:
		    return Cell(_("SSH User"), Id::SSH_USER);

		case Column::SSH_PORT:
		    return Cell(_("SSH Port"), Id::SSH_PORT);

		case Column::SSH_IDENTITY:
		    return Cell(_("SSH Identity"), Id::SSH_IDENTITY);

		case Column::TARGET_BTRFS_BIN:
		    return Cell(_("Target btrfs bin"));

		case Column::TARGET_LS_BIN:
		    return Cell(_("Target ls Bin"));

		case Column::TARGET_MKDIR_BIN:
		    return Cell(_("Target mkdir bin"));

		case Column::TARGET_RM_BIN:
		    return Cell(_("Target rm bin"));

		case Column::TARGET_RMDIR_BIN:
		    return Cell(_("Target rmdir bin"));
	    }

	    SN_THROW(Exception("invalid column value"));
	    __builtin_unreachable();
	}


	boost::any
	value_for_as_any(Column column, const BackupConfig& backup_config)
	{
	    switch (column)
	    {
		case Column::NAME:
		    return backup_config.name;

		case Column::CONFIG:
		    return backup_config.config;

		case Column::TARGET_MODE:
		    return toString(backup_config.target_mode);

		case Column::AUTOMATIC:
		    return backup_config.automatic;

		case Column::SOURCE_PATH:
		    return backup_config.source_path;

		case Column::TARGET_PATH:
		    return backup_config.target_path;

		case Column::SSH_HOST:
		    if (backup_config.ssh_host.empty())
			return nullptr;
		    return backup_config.ssh_host;

		case Column::SSH_USER:
		    if (backup_config.ssh_user.empty())
			return nullptr;
		    return backup_config.ssh_user;

		case Column::SSH_PORT:
		    if (backup_config.ssh_port == 0)
			return nullptr;
		    return to_string(backup_config.ssh_port);

		case Column::SSH_IDENTITY:
		    if (backup_config.ssh_identity.empty())
			return nullptr;
		    return backup_config.ssh_identity;

		case Column::TARGET_BTRFS_BIN:
		    return backup_config.target_btrfs_bin;

		case Column::TARGET_LS_BIN:
		    return backup_config.target_ls_bin;

		case Column::TARGET_MKDIR_BIN:
		    return backup_config.target_mkdir_bin;

		case Column::TARGET_RM_BIN:
		    return backup_config.target_rm_bin;

		case Column::TARGET_RMDIR_BIN:
		    return backup_config.target_rmdir_bin;
	    }

	    SN_THROW(Exception("invalid column value"));
	    __builtin_unreachable();
	}


	string
	value_for_as_string(const OutputOptions& output_options, Column column, const BackupConfig& backup_config)
	{
	    return any_to_string(output_options, value_for_as_any(column, backup_config));
	}


	json_object*
	value_for_as_json(const OutputOptions& output_options, Column column, const BackupConfig& backup_config)
	{
	    return any_to_json(output_options, value_for_as_any(column, backup_config));
	}


	void
	output_table(const GlobalOptions& global_options, const vector<Column>& columns,
		     const BackupConfigs& backup_configs)
	{
	    OutputOptions output_options(global_options.utc(), global_options.iso(), true);

	    TableFormatter formatter(global_options.table_style());

	    for (Column column : columns)
		formatter.header().push_back(header_for(column));

	    formatter.auto_visibility().push_back(Id::SSH_HOST);
	    formatter.auto_visibility().push_back(Id::SSH_USER);
	    formatter.auto_visibility().push_back(Id::SSH_PORT);
	    formatter.auto_visibility().push_back(Id::SSH_IDENTITY);

	    for (const BackupConfig& backup_config : backup_configs)
	    {
		vector<string> row;

		for (Column column : columns)
		    row.push_back(value_for_as_string(output_options, column, backup_config));

		formatter.rows().push_back(row);
	    }

	    cout << formatter;
	}


	void
	output_csv(const GlobalOptions& global_options, const vector<Column>& columns,
		   const BackupConfigs& backup_configs)
	{
	    OutputOptions output_options(global_options.utc(), global_options.iso(), false);

	    CsvFormatter formatter(global_options.separator(), global_options.headers());

	    for (Column column : columns)
		formatter.header().push_back(toString(column));

	    for (const BackupConfig& backup_config : backup_configs)
	    {
		vector<string> row;

		for (Column column : columns)
		    row.push_back(value_for_as_string(output_options, column, backup_config));

		formatter.rows().push_back(row);
	    }

	    cout << formatter;
	}


	void
	output_json(const GlobalOptions& global_options, const vector<Column>& columns,
		    const BackupConfigs& backup_configs)
	{
	    OutputOptions output_options(global_options.utc(), global_options.iso(), false);

	    JsonFormatter formatter;

	    json_object* json_backup_configs = json_object_new_array();
	    json_object_object_add(formatter.root(), "backup-configs", json_backup_configs);

	    for (const BackupConfig& backup_config : backup_configs)
	    {
		json_object* json_backup_config = json_object_new_object();
		json_object_array_add(json_backup_configs, json_backup_config);

		for (const Column& column : columns)
		{
		    json_object* tmp = value_for_as_json(output_options, column, backup_config);
		    if (tmp)
			json_object_object_add(json_backup_config, toString(column).c_str(), tmp);
		}
	    }

	    cout << formatter;
	}

    }


    void
    command_list_configs(const GlobalOptions& global_options, GetOpts& get_opts, const BackupConfigs& backup_configs,
			 ProxySnappers* snappers)
    {
	ParsedOpts opts = get_opts.parse("list-configs", GetOpts::no_options);

	if (get_opts.has_args())
	{
	    SN_THROW(OptionsException(_("Command 'list-configs' does not take arguments.")));
	}

	const vector<Column> some_columns = { Column::NAME, Column::CONFIG, Column::TARGET_MODE,
	    Column::AUTOMATIC, Column::SOURCE_PATH, Column::TARGET_PATH, Column::SSH_HOST,
	    Column::SSH_USER, Column::SSH_PORT, Column::SSH_IDENTITY };

	const vector<Column> all_columns = { Column::NAME, Column::CONFIG, Column::TARGET_MODE,
	    Column::AUTOMATIC, Column::SOURCE_PATH, Column::TARGET_PATH, Column::SSH_HOST,
	    Column::SSH_USER, Column::SSH_PORT, Column::SSH_IDENTITY, Column::TARGET_BTRFS_BIN,
	    Column::TARGET_LS_BIN, Column::TARGET_MKDIR_BIN, Column::TARGET_RM_BIN,
	    Column::TARGET_RMDIR_BIN
	};

	switch (global_options.output_format())
	{
	    case GlobalOptions::OutputFormat::TABLE:
		output_table(global_options, some_columns, backup_configs);
		break;

	    case GlobalOptions::OutputFormat::CSV:
		output_csv(global_options, all_columns, backup_configs);
		break;

	    case GlobalOptions::OutputFormat::JSON:
		output_json(global_options, all_columns, backup_configs);
		break;
	}
    }


    template <> struct EnumInfo<Column> { static const vector<string> names; };

    const vector<string> EnumInfo<Column>::names({
	"name", "config", "target-mode", "automatic", "source-path", "target-path", "ssh-host",
	"ssh-user", "ssh-port", "ssh-identity", "target-btrfs-bin", "target-ls-bin",
	"target-mkdir-bin", "target-rm-bin", "target-rmdir-bin"
    });

}
