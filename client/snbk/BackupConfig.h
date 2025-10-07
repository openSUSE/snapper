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

#ifndef SNAPPER_BACKUP_CONFIG_H
#define SNAPPER_BACKUP_CONFIG_H


#include "config.h"

#include <string>
#include <vector>

#include <snapper/Enum.h>

#include "Shell.h"


namespace snapper
{

    using std::string;
    using std::vector;


    class BackupConfig
    {
    public:

	enum class TargetMode
	{
	    LOCAL, SSH_PUSH
	};

	BackupConfig(const string& name);

	const string name;

	string config;

	TargetMode target_mode = TargetMode::LOCAL;

	string source_path;
	string target_path;

	bool automatic = false;

	string ssh_host;
	unsigned int ssh_port = 0;
	string ssh_user;
	string ssh_identity;

	Shell get_source_shell() const;
	Shell get_target_shell() const;

	bool send_compressed_data = true;
	vector<string> send_options;
	vector<string> receive_options;

	string target_btrfs_bin = BTRFS_BIN;
	string target_findmnt_bin = FINDMNT_BIN;
	string target_mkdir_bin = MKDIR_BIN;
	string target_realpath_bin = REALPATH_BIN;
	string target_rm_bin = RM_BIN;
	string target_rmdir_bin = RMDIR_BIN;

    private:

	vector<string> ssh_options() const;

    };


    using BackupConfigs = vector<BackupConfig>;


    template <> struct EnumInfo<BackupConfig::TargetMode> { static const vector<string> names; };


    vector<string>
    read_backup_config_names();

}


#endif
