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


#include "BackupConfig.h"
#include "GlobalOptions.h"
#include "TheBigThing.h"


namespace snapper
{

    class SnapshotOperation
    {
    public:

	SnapshotOperation(const GlobalOptions& global_options, GetOpts& get_opts,
	                  const BackupConfigs& backup_configs, ProxySnappers* snappers)
	    : global_options(global_options), get_opts(get_opts),
	      backup_configs(backup_configs), snappers(snappers)
	{
	}

	virtual ~SnapshotOperation() = default;

	void operator()();

    protected:

	/** Command name of the snapshot operation. */
	virtual const char* command() const = 0;

	/** Run the optional prerequisite procedures for the snapshot operation. */
	virtual void prerequisite() const {}

	/** Run the snapshot operation for the entire backup config. */
	virtual void run_all(TheBigThings& the_big_things,
	                     const BackupConfig& backup_config, bool quiet,
	                     bool verbose) const = 0;

	/** Run the operation for a specific snapshot. */
	virtual void run_single(TheBigThing& the_big_thing,
	                        const BackupConfig& backup_config,
	                        TheBigThings& the_big_things, bool quiet) const = 0;

	/** Messages related to the snapshot operation. */
	virtual const char* msg_running() const = 0;
	virtual const char* msg_failed() const = 0;
	virtual const char* msg_error_summary() const = 0;

	vector<unsigned int> parse_nums() const;

	const GlobalOptions& global_options;
	GetOpts& get_opts;
	const BackupConfigs& backup_configs;
	ProxySnappers* snappers;
    };

} // namespace snapper
