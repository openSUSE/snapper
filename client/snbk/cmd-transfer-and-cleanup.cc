/*
 * Copyright (c) 2026 SUSE LLC
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

#include "proxy-snbk.h"
#include "utils.h"


namespace snapper
{
    namespace
    {
	class SnapshotTransferAndCleanup : public SnapshotOperation
	{
	public:

	    SnapshotTransferAndCleanup(const GlobalOptions& global_options,
	                               GetOpts& get_opts,
	                               const BackupConfigs& backup_configs,
	                               ProxySnappers* snappers)
	        : SnapshotOperation(global_options, get_opts, backup_configs, snappers)
	    {
	    }

	protected:

	    const char* command() const override { return "transfer-and-cleanup"; }

	    void prerequisite() const override
	    {
		if (get_opts.has_args())
		{
		    SN_THROW(OptionsException(
		        _("Command 'transfer-and-cleanup' does not take arguments.")));
		}
	    }

	    void run_all(TheBigThings& the_big_things, const BackupConfig& backup_config,
	                 bool quiet, bool verbose) const override
	    {
		switch (backup_config.retention_policy)
		{
		    case BackupConfig::RetentionPolicy::CUSTOM:
			run_custom(the_big_things, backup_config, quiet, verbose);
			break;

		    case BackupConfig::RetentionPolicy::DEFAULT:
			run_default(the_big_things, backup_config, quiet, verbose);
			break;
		}
	    }

	    void run_single(TheBigThing& the_big_thing, const BackupConfig& backup_config,
	                    TheBigThings& the_big_things, bool quiet) const override
	    {
		SN_THROW(Exception(_("Command 'transfer-and-cleanup' does not support "
		                     "operating on a single snapshot.")));
	    }

	    const char* msg_running() const override
	    { return _("Running transfer and cleanup for backup config '%s'."); }

	    const char* msg_failed() const override
	    { return _("Running transfer and cleanup for backup config '%s' failed."); }

	    const char* msg_error_summary() const override
	    {
		return _(
		    "Running transfer and cleanup failed for %d of %ld backup config.",
		    "Running transfer and cleanup failed for %d of %ld backup configs.",
		    backup_configs.size());
	    }

	private:

	    void run_custom(TheBigThings& the_big_things,
	                    const BackupConfig& backup_config, bool quiet,
	                    bool verbose) const
	    {
		// Remove the invalid snapshots
		for (TheBigThing& the_big_thing : the_big_things)
		{
		    if (the_big_thing.source_state ==
		            TheBigThing::SourceState::READ_ONLY &&
		        the_big_thing.target_state == TheBigThing::TargetState::INVALID)
			the_big_thing.remove(backup_config, quiet);
		}

		// Run the enabled cleanup algorithms in emulation mode
		SnbkCleanup cleanup_emu(backup_config, the_big_things, true);
		const ProxyConfig& proxy_config = cleanup_emu.get_config();

		Plugins::Report report;
		if (proxy_config.is_yes("NUMBER_CLEANUP"))
		    cleanup_emu.do_cleanup_number(verbose, report);
		if (proxy_config.is_yes("TIMELINE_CLEANUP"))
		    cleanup_emu.do_cleanup_timeline(verbose, report);

		// Cache snapshots that will be removed by the cleanup algorithms
		const unordered_set<ProxySnapshots::const_iterator>& snapshots_deleted =
		    cleanup_emu.get_cleanable().snapshots_deleted;

		// Transfer the missing snapshots that will be kept by the cleanup
		// algorithms
		const ProxySnapshots& snapshots =
		    cleanup_emu.get_cleanable().get_snapshots();
		for (ProxySnapshots::const_iterator it = snapshots.begin();
		     it != snapshots.end(); ++it)
		{
		    // Skip if the snapshot will be removed by the cleanup algorithms
		    if (snapshots_deleted.count(it))
			continue;

		    // Transfer the snapshot if it is missing on the target
		    TheBigThings::iterator it_lookup = the_big_things.find(it->getNum());
		    if (it_lookup == the_big_things.end())
			SN_THROW(Exception(_("Cannot find the requested snapshot.")));
		    if (it_lookup->target_state == TheBigThing::TargetState::MISSING)
			it_lookup->transfer(backup_config, the_big_things, quiet);
		}

		// Delete the valid and legacy snapshots selected by the cleanup
		// algorithms on the target device
		for (const ProxySnapshots::const_iterator& it : snapshots_deleted)
		{
		    TheBigThings::iterator it_lookup = the_big_things.find(it->getNum());
		    if (it_lookup == the_big_things.end())
			SN_THROW(Exception(_("Cannot find the requested snapshot.")));

		    if (it_lookup->target_state == TheBigThing::TargetState::VALID ||
		        it_lookup->target_state == TheBigThing::TargetState::LEGACY)
			it_lookup->remove(backup_config, quiet);
		}
	    }

	    void run_default(TheBigThings& the_big_things,
	                     const BackupConfig& backup_config, bool quiet,
	                     bool verbose) const
	    {
		the_big_things.transfer(backup_config, quiet, verbose);
		the_big_things.remove(backup_config, quiet, verbose);
	    }
	};

    } // namespace

    using namespace std;


    void help_transfer_and_cleanup()
    {
	cout << "  " << _("Transfer and cleanup:") << '\n'
	     << "\t" << _("snbk transfer-and-cleanup") << '\n'
	     << '\n';
    }


    void command_transfer_and_cleanup(const GlobalOptions& global_options,
                                      GetOpts& get_opts,
                                      const BackupConfigs& backup_configs,
                                      ProxySnappers* snappers)
    {
	SnapshotTransferAndCleanup snapshot_operation(global_options, get_opts,
	                                              backup_configs, snappers);
	snapshot_operation();
    }

} // namespace snapper
