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
#include "utils/help.h"
#include "GlobalOptions.h"
#include "proxy.h"
#include "MyFiles.h"


namespace snapper
{

    using namespace std;


    void
    help_status()
    {
	cout << _("  Comparing snapshots:") << '\n'
	     << _("\tsnapper status <number1>..<number2>") << '\n'
	     << '\n'
	     << _("    Options for 'status' command:") << '\n';

	print_options({
	    { _("--output, -o <file>"), _("Save status to file.") }
	});
    }


    void
    command_status(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers*,
		   ProxySnapper* snapper, Plugins::Report& report)
    {
	const vector<Option> options = {
	    Option("output",	required_argument,	'o')
	};

	ParsedOpts opts = get_opts.parse("status", options);
	if (get_opts.num_args() != 1)
	{
	    cerr << _("Command 'status' needs one argument.") << endl;

	    if (get_opts.num_args() == 2)
	    {
		cerr << _("Maybe you forgot the delimiter '..' between the snapshot numbers.") << endl
		     << _("See 'man snapper' for further instructions.") << endl;
	    }

	    exit(EXIT_FAILURE);
	}

	ParsedOpts::const_iterator opt;

	ProxySnapshots& snapshots = snapper->getSnapshots();

	pair<ProxySnapshots::const_iterator, ProxySnapshots::const_iterator> range =
	    snapshots.findNums(get_opts.pop_arg());

	ProxyComparison comparison = snapper->createComparison(*range.first, *range.second, false);

	MyFiles files(comparison.getFiles());

	FILE* file = stdout;

	if ((opt = opts.find("output")) != opts.end())
	{
	    file = fopen(opt->second.c_str(), "w");
	    if (!file)
	    {
		cerr << sformat(_("Opening file '%s' failed."), opt->second.c_str()) << endl;
		exit(EXIT_FAILURE);
	    }
	}

	for (Files::const_iterator it = files.begin(); it != files.end(); ++it)
	    fprintf(file, "%s %s\n", statusToString(it->getPreToPostStatus()).c_str(),
		    it->getAbsolutePath(LOC_SYSTEM).c_str());

	if (file != stdout)
	    fclose(file);
    }

}
