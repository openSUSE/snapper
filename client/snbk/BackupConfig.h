/*
 * Copyright (c) [2024-2026] SUSE LLC
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

#include <map>
#include <string>
#include <vector>

#include <snapper/Enum.h>

#include "Shell.h"


namespace snapper
{

    using std::map;
    using std::string;
    using std::vector;


    class BackupConfig
    {
    public:

	enum class TargetMode
	{
	    LOCAL, SSH_PUSH, SSH_PULL
	};

	enum class RetentionPolicy
	{
	    /**
	     * The default policy means keeping the snapshots on the backup device
	     * identical to the source device. If the retention policy (`retention-policy`
	     * in the config) is set to `default`, the retention config
	     * (`retention-config` in the config) is ignored.
	     */
	    DEFAULT,

	    /**
	     * The custom policy means the snapshot retention can be different from the
	     * source device. The behavior can be controlled via `retention-config` in the
	     * config. The parameters share the same names as `snapper` options (e.g.,
	     * NUMBER_LIMIT, TIMELINE_LIMIT_HOURLY, ...).
	     */
	    CUSTOM
	};

	BackupConfig(const string& name);

	const string name;

	string config;

	TargetMode target_mode = TargetMode::LOCAL;

	bool is_local_source() const;
	bool is_local_target() const;

	string target_path;

	bool automatic = false;

	string ssh_host;
	unsigned int ssh_port = 0;
	string ssh_user;
	string ssh_identity;
	bool ssh_master_control = true;

	Shell get_source_shell() const;
	Shell get_target_shell() const;

	bool send_compressed_data = true;
	vector<string> send_options;
	vector<string> receive_options;

	string source_btrfs_bin = BTRFS_BIN;
	string source_cat_bin = CAT_BIN;
	string source_mkdir_bin = MKDIR_BIN;
	string source_snapper_bin = SNAPPER_BIN;

	string target_btrfs_bin = BTRFS_BIN;
	string target_cat_bin = CAT_BIN;
	string target_ls_bin = LS_BIN;
	string target_mkdir_bin = MKDIR_BIN;
	string target_rm_bin = RM_BIN;
	string target_rmdir_bin = RMDIR_BIN;

	RetentionPolicy retention_policy = RetentionPolicy::DEFAULT;
	map<string, string> retention_config;

    private:

	vector<string> ssh_options() const;

    };


    using BackupConfigs = vector<BackupConfig>;


    template <> struct EnumInfo<BackupConfig::TargetMode> { static const vector<string> names; };

    template <> struct EnumInfo<BackupConfig::RetentionPolicy>
    {
	static const vector<string> names;
    };


    vector<string>
    read_backup_config_names();

}


#endif
