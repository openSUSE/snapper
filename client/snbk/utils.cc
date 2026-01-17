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


#include <regex>

#include <snapper/AppUtil.h>

#include "../proxy/errors.h"
#include "../utils/GetOpts.h"
#include "../utils/text.h"

#include "utils.h"


namespace snapper
{

    void SnapshotOperation::operator()()
    {
	ParsedOpts opts = get_opts.parse(command(), GetOpts::no_options);

	// Run prerequisite step
	prerequisite();

	// Run the snapshot operation
	vector<unsigned int> nums = parse_nums();

	unsigned int errors = 0;

	for (const BackupConfig& backup_config : backup_configs)
	{
	    if (!global_options.quiet())
		cout << sformat(msg_running(), backup_config.name.c_str()) << endl;

	    try
	    {
		TheBigThings the_big_things(backup_config, snappers,
		                            global_options.verbose());

		if (nums.empty())
		{
		    run_all(the_big_things, backup_config, global_options.quiet(),
		            global_options.quiet());
		}
		else
		{
		    for (unsigned int num : nums)
		    {
			TheBigThings::iterator it = the_big_things.find(num);
			if (it == the_big_things.end())
			{
			    string error =
			        sformat(_("Snapshot number %d not found."), num);
			    SN_THROW(Exception(error));
			}

			run_single(*it, backup_config, the_big_things,
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
		cerr << sformat(msg_failed(), backup_config.name.c_str()) << endl;

		++errors;
	    }
	}

	if (errors != 0)
	{
	    string error = sformat(msg_error_summary(), errors, backup_configs.size());
	    SN_THROW(Exception(error));
	}
    }

    vector<unsigned int> SnapshotOperation::parse_nums() const
    {
	static const regex num_regex("[0-9]+", regex::extended);

	vector<unsigned int> nums;

	while (get_opts.has_args())
	{
	    string arg = get_opts.pop_arg();

	    if (!regex_match(arg, num_regex))
		SN_THROW(Exception(_("Failed to parse number.")));

	    nums.push_back(stoi(arg));
	}

	return nums;
    }

} // namespace snapper
