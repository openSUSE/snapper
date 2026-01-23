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


#include <iostream>

#include "../utils/text.h"

#include "utils.h"


namespace snapper
{
    namespace
    {
	class SnapshotTransfer : public SnapshotOperation
	{
	public:

	    SnapshotTransfer(const GlobalOptions& global_options, GetOpts& get_opts,
	                     const BackupConfigs& backup_configs, ProxySnappers* snappers)
	        : SnapshotOperation(global_options, get_opts, backup_configs, snappers)
	    {
	    }

	protected:

	    const char* command() const override { return "transfer"; }

	    void run_all(TheBigThings& the_big_things, const BackupConfig& backup_config,
	                 bool quiet, bool verbose) const override
	    {
		the_big_things.transfer(backup_config, quiet, verbose);
	    }

	    void run_single(TheBigThing& the_big_thing, const BackupConfig& backup_config,
	                    TheBigThings& the_big_things, bool quiet) const override
	    {
		the_big_thing.transfer(backup_config, the_big_things, quiet);
	    }

	    const char* msg_running() const override
	    {
		return _("Running transfer for backup config '%s'.");
	    }

	    const char* msg_failed() const override
	    {
		return _("Running transfer for backup config '%s' failed.");
	    }

	    const char* msg_error_summary() const override
	    {
		return _("Running transfer failed for %d of %ld backup config.",
		         "Running transfer failed for %d of %ld backup configs.",
		         backup_configs.size());
	    }
	};

    } // namespace

    using namespace std;


    void
    help_transfer()
    {
	cout << "  " << _("Transfer:") << '\n'
	     << "\t" << _("snbk transfer [numbers]") << '\n'
	     << '\n';
    }


    void
    command_transfer(const GlobalOptions& global_options, GetOpts& get_opts, const BackupConfigs& backup_configs,
		     ProxySnappers* snappers)
    {
	SnapshotTransfer snapshot_operation(global_options, get_opts, backup_configs,
	                                    snappers);
	snapshot_operation();
    }

}
