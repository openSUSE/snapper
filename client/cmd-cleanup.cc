/*
 * Copyright (c) [2011-2015] Novell, Inc.
 * Copyright (c) [2016-2020] SUSE LLC
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

#include "utils/text.h"
#include "GlobalOptions.h"
#include "proxy.h"
#include "cleanup.h"


namespace snapper
{

    using namespace std;


    void
    help_cleanup()
    {
	cout << _("  Cleanup snapshots:") << '\n'
	     << _("\tsnapper cleanup <cleanup-algorithm>") << '\n'
	     << endl;
    }


    void
    command_cleanup(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers* snappers, ProxySnapper* snapper)
    {
	ParsedOpts opts = get_opts.parse("cleanup", GetOpts::no_options);
	if (get_opts.num_args() != 1)
	{
	    cerr << _("Command 'cleanup' needs one arguments.") << endl;
	    exit(EXIT_FAILURE);
	}

	string cleanup = get_opts.pop_arg();

	if (cleanup == "number")
	{
	    do_cleanup_number(snapper, global_options.verbose());
	}
	else if (cleanup == "timeline")
	{
	    do_cleanup_timeline(snapper, global_options.verbose());
	}
	else if (cleanup == "empty-pre-post")
	{
	    do_cleanup_empty_pre_post(snapper, global_options.verbose());
	}
	else
	{
	    cerr << sformat(_("Unknown cleanup algorithm '%s'."), cleanup.c_str()) << endl;
	    exit(EXIT_FAILURE);
	}
    }

}
