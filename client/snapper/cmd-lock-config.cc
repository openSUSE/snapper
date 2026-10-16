/*
 * Copyright (c) [2016-2026] SUSE LLC
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
#include "../proxy/proxy.h"
#include "GlobalOptions.h"


namespace snapper
{

    using namespace std;


    // For the time being the lock-config command is undocumented.


    void
    help_lock_config()
    {
    }


    void
    command_lock_config(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers* snappers,
			ProxySnapper* snapper, Plugins::Report& report)
    {
	get_opts.parse("lock-config", GetOpts::no_options);
	if (get_opts.has_args())
	{
	    cerr << _("Command 'lock-config' does not take arguments.") << endl;
	    exit(EXIT_FAILURE);
	}

	snapper->lock_config();

	cout << "Press Enter to continue...";
	cin.get();

	snapper->unlock_config();
    }

}
