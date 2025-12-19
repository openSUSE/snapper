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
#include <regex>

#include <snapper/AppUtil.h>

#include "../proxy/errors.h"
#include "../utils/text.h"

#include "BackupConfig.h"
#include "GlobalOptions.h"
#include "TheBigThing.h"
#include "utils.h"


namespace snapper
{

    using namespace std;


    void
    help_restore()
    {
	cout << "  " << _("Restore:") << '\n'
	     << "\t" << _("snbk restore [numbers]") << '\n'
	     << '\n'
	     << "\t" << _("Notice:") << '\n'
	     << "\t" << _("- A backup-config must be specified to run this command.")
		     << '\n'
	     << "\t" << _("- Restart the `snapperd` service to make the restored "
			  "snapshots visible immediately.") << '\n'
	     << '\n';
    }


    void
    command_restore(const GlobalOptions& global_options, GetOpts& get_opts,
		    BackupConfigs& backup_configs, ProxySnappers* snappers)
    {
	ParsedOpts opts = get_opts.parse("restore", GetOpts::no_options);

	if (backup_configs.size() != 1)
	{
	    SN_THROW(OptionsException(_("A backup-config must be specified to run this "
					"command.")));
	}

	vector<unsigned int> nums = parse_nums(get_opts);

	unsigned int errors = 0;

	for (const BackupConfig& backup_config : backup_configs)
	{
	    if (!global_options.quiet())
		cout << sformat(_("Running restore for backup config '%s'."),
				backup_config.name.c_str()) << endl;

	    try
	    {
		TheBigThings the_big_things(backup_config, snappers,
					    global_options.verbose());

		if (nums.empty())
		{
		    the_big_things.restore(backup_config, global_options.quiet(),
					   global_options.quiet());
		}
		else
		{
		    for (unsigned int num : nums)
		    {
			TheBigThings::iterator it = the_big_things.find(num);
			if (it == the_big_things.end())
			{
			    string error = sformat(_("Snapshot number %d not found."),
						   num);
			    SN_THROW(Exception(error));
			}

			it->restore(backup_config, the_big_things,
				    global_options.quiet());
		    }
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

		cerr << sformat(_("Running restore for backup config '%s' failed."),
				backup_config.name.c_str()) << endl;

		++errors;
	    }
	}

	if (errors != 0)
	{
	    string error = sformat(
		_("Running restore failed for %d of %ld backup config.",
		  "Running restore failed for %d of %ld backup configs.",
		  backup_configs.size()), errors, backup_configs.size()
	    );

	    SN_THROW(Exception(error));
	}
    }

}
