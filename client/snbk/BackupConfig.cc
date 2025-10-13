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


#include <cstring>

#include <snapper/SnapperDefines.h>
#include <snapper/AppUtil.h>

#include "BackupConfig.h"
#include "JsonFile.h"


namespace snapper
{

    using namespace std;


    const vector<string> EnumInfo<BackupConfig::TargetMode>::names({ "local", "ssh-push" });


    BackupConfig::BackupConfig(const string& name)
	: name(name)
    {
	JsonFile json_file(BACKUP_CONFIGS_DIR "/" + name + ".json");

	if (!get_child_value(json_file.get_root(), "config", config))
	    SN_THROW(Exception(sformat("config entry not found in '%s'", name.c_str())));

	string tmp1;
	if (!get_child_value(json_file.get_root(), "target-mode", tmp1))
	    SN_THROW(Exception(sformat("target-mode entry not found in '%s'", name.c_str())));
	if (!toValue(tmp1, target_mode, false))
	    SN_THROW(Exception(sformat("unknown target-mode '%s' in '%s'", tmp1.c_str(), name.c_str())));

	if (!get_child_value(json_file.get_root(), "source-path", source_path))
	    SN_THROW(Exception(sformat("source-path entry not found in '%s'", name.c_str())));

	if (!get_child_value(json_file.get_root(), "target-path", target_path))
	    SN_THROW(Exception(sformat("target-path entry not found in '%s'", name.c_str())));

	get_child_value(json_file.get_root(), "automatic", automatic);

	if (target_mode == TargetMode::SSH_PUSH)
	{
	    if (!get_child_value(json_file.get_root(), "ssh-host", ssh_host))
		SN_THROW(Exception(sformat("ssh-host entry not found in '%s'", name.c_str())));

	    get_child_value(json_file.get_root(), "ssh-port", ssh_port);
	    get_child_value(json_file.get_root(), "ssh-user", ssh_user);
	    get_child_value(json_file.get_root(), "ssh-identity", ssh_identity);
	}

	get_child_value(json_file.get_root(), "send-compressed-data", send_compressed_data);
	get_child_nodes(json_file.get_root(), "send-options", send_options);
	get_child_nodes(json_file.get_root(), "receive-options", receive_options);

	get_child_value(json_file.get_root(), "target-btrfs-bin", target_btrfs_bin);
	get_child_value(json_file.get_root(), "target-ls-bin", target_ls_bin);
	get_child_value(json_file.get_root(), "target-mkdir-bin", target_mkdir_bin);
	get_child_value(json_file.get_root(), "target-rm-bin", target_rm_bin);
	get_child_value(json_file.get_root(), "target-rmdir-bin", target_rmdir_bin);
    }


    Shell
    BackupConfig::get_source_shell() const
    {
	Shell source_shell;

	return source_shell;
    }


    Shell
    BackupConfig::get_target_shell() const
    {
	Shell target_shell;

	if (target_mode == TargetMode::SSH_PUSH)
	{
	    target_shell.mode = Shell::Mode::SSH;
	    target_shell.ssh_options = ssh_options();
	}

	return target_shell;
    }


    vector<string>
    BackupConfig::ssh_options() const
    {
	vector<string> options = { ssh_host };

	if (ssh_port != 0)
	    options.insert(options.end(), { "-p", to_string(ssh_port) });

	if (!ssh_user.empty())
	    options.insert(options.end(), { "-l", ssh_user });

	if (!ssh_identity.empty())
	    options.insert(options.end(), { "-i", ssh_identity });

	return options;
    }


    vector<string>
    read_backup_config_names()
    {
	const vector<string> filenames = glob(BACKUP_CONFIGS_DIR "/" "*.json", 0);
	const size_t l1 = strlen(BACKUP_CONFIGS_DIR "/");
	const size_t l2 = strlen(".json");

	vector<string> names;

	for (const string& filename : filenames)
	    names.push_back(filename.substr(l1, filename.size() - l1 - l2));

	return names;
    }

}
