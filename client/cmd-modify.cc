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

#include "utils/text.h"
#include "GlobalOptions.h"
#include "proxy.h"
#include "misc.h"


namespace snapper
{

    using namespace std;


    void
    help_modify()
    {
	cout << _("  Modify snapshot:") << '\n'
	     << _("\tsnapper modify <number>") << '\n'
	     << '\n'
	     << _("    Options for 'modify' command:") << '\n'
	     << _("\t--description, -d <description>\tDescription for snapshot.") << '\n'
	     << _("\t--cleanup-algorithm, -c <algo>\tCleanup algorithm for snapshot.") << '\n'
	     << _("\t--userdata, -u <userdata>\tUserdata for snapshot.") << '\n'
	     << endl;
    }


    void
    command_modify(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers* snappers, ProxySnapper* snapper)
    {
	const vector<Option> options = {
	    Option("description",		required_argument,	'd'),
	    Option("cleanup-algorithm",		required_argument,	'c'),
	    Option("userdata",			required_argument,	'u')
	};

	ParsedOpts opts = get_opts.parse("modify", options);
	if (!get_opts.has_args())
	{
	    cerr << _("Command 'modify' needs at least one argument.") << endl;
	    exit(EXIT_FAILURE);
	}

	ProxySnapshots& snapshots = snapper->getSnapshots();

	while (get_opts.has_args())
	{
	    ProxySnapshots::iterator snapshot = snapshots.findNum(get_opts.pop_arg());

	    SMD smd = snapshot->getSmd();

	    ParsedOpts::const_iterator opt;

	    if ((opt = opts.find("description")) != opts.end())
		smd.description = opt->second;

	    if ((opt = opts.find("cleanup-algorithm")) != opts.end())
		smd.cleanup = opt->second;

	    if ((opt = opts.find("userdata")) != opts.end())
		smd.userdata = read_userdata(opt->second, snapshot->getUserdata());

	    snapper->modifySnapshot(snapshot, smd);
	}
    }

}
