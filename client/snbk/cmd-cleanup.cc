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

#include <snapper/AppUtil.h>

#include "../cleanup.h"
#include "../misc.h"
#include "../proxy/errors.h"
#include "../utils/text.h"

#include "BackupConfig.h"
#include "GlobalOptions.h"
#include "TheBigThing.h"
#include "proxy-snbk.h"


namespace snapper
{

    using namespace std;


    void help_cleanup()
    {
	cout << "  " << _("Cleanup:") << '\n'
	     << "\t" << _("snbk cleanup <cleanup-algorithm>") << '\n'
	     << '\n';
    }


    void command_cleanup(const GlobalOptions& global_options, GetOpts& get_opts,
                         const BackupConfigs& backup_configs, ProxySnappers* snappers)
    {
	// Parse command arguments
	ParsedOpts opts = get_opts.parse("cleanup", GetOpts::no_options);
	if (get_opts.num_args() != 1)
	{
	    SN_THROW(OptionsException(_("Command 'cleanup' needs one argument.")));
	}

	CleanupAlgorithm cleanup_algorithm;
	const char* arg = get_opts.pop_arg();
	if (!toValue(arg, cleanup_algorithm, false))
	{
	    string error = sformat(_("Unknown cleanup algorithm '%s'."), arg) + '\n' +
	                   possible_enum_values<CleanupAlgorithm>();
	    SN_THROW(OptionsException(error));
	}

	// Do snapshot cleanup
	Plugins::Report report;

	unsigned int errors = 0;
	for (const BackupConfig& backup_config : backup_configs)
	{
	    if (!global_options.quiet())
		cout << sformat(_("Running cleanup for backup config '%s'."),
		                backup_config.name.c_str())
		     << endl;


	    try
	    {
		TheBigThings the_big_things(backup_config, snappers,
		                            global_options.verbose());
		switch (backup_config.retention_policy)
		{
		    case BackupConfig::RetentionPolicy::CUSTOM:
		    {
			SnbkCleanup cleanup(backup_config, the_big_things);
			const ProxyConfig& proxy_config = cleanup.get_config();

			if (proxy_config.is_yes("NUMBER_CLEANUP"))
			    cleanup.do_cleanup_number(global_options.verbose(), report);
			if (proxy_config.is_yes("TIMELINE_CLEANUP"))
			    cleanup.do_cleanup_timeline(global_options.verbose(), report);
		    }
		    break;

		    case BackupConfig::RetentionPolicy::DEFAULT:
		    {
			the_big_things.remove(backup_config, global_options.quiet(),
			                      global_options.quiet());
		    }
		    break;
		}
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

		cerr << e.what() << '\n';
		cerr << sformat(_("Running cleanup for backup config '%s' failed."),
		                backup_config.name.c_str())
		     << endl;

		++errors;
	    }
	}

	bool plugin_status = true;
	for (const Plugins::Report::Entry& entry : report.entries)
	{
	    if (entry.exit_status != 0)
	    {
		cerr << sformat(_("Server-side plugin '%s' failed"), entry.name.c_str());
		plugin_status = false;
	    }
	}

	if (errors != 0)
	{
	    string error =
	        sformat(_("Running cleanup failed for %d of %ld backup config.",
	                  "Running cleanup failed for %d of %ld backup configs.",
	                  backup_configs.size()),
	                errors, backup_configs.size());
	    SN_THROW(Exception(error));
	}

	if (!plugin_status)
	    SN_THROW(Exception(_("One or more server-side plugins have failed.")));
    }

} // namespace snapper
