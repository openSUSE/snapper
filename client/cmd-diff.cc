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
#include "misc.h"
#include "MyFiles.h"


namespace snapper
{

    using namespace std;


    void
    help_diff()
    {
	cout << _("  Comparing snapshots:") << '\n'
	     << _("\tsnapper diff <number1>..<number2> [files]") << '\n'
	     << '\n'
	     << _("    Options for 'diff' command:") << '\n'
	     << _("\t--input, -i <file>\t\tRead files to diff from file.") << '\n'
	     << _("\t--diff-cmd <command>\t\tCommand used for comparing files.") << '\n'
	     << _("\t--extensions, -x <options>\tExtra options passed to the diff command.") << '\n'
	     << endl;
    }


    void
    command_diff(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers*,
		 ProxySnapper* snapper, Plugins::Report& report)
    {
	const vector<Option> options = {
	    Option("input",		required_argument,	'i'),
	    Option("diff-cmd",		required_argument),
	    Option("extensions",	required_argument,	'x'),
	};

	ParsedOpts opts = get_opts.parse("diff", options);
	if (get_opts.num_args() < 1)
	{
	    cerr << _("Command 'diff' needs at least one argument.") << endl;
	    exit(EXIT_FAILURE);
	}

	FILE* file = NULL;
	Differ differ;

	ParsedOpts::const_iterator opt;

	if ((opt = opts.find("input")) != opts.end())
	{
	    file = fopen(opt->second.c_str(), "r");
	    if (!file)
	    {
		cerr << sformat(_("Opening file '%s' failed."), opt->second.c_str()) << endl;
		exit(EXIT_FAILURE);
	    }
	}

	if ((opt = opts.find("diff-cmd")) != opts.end())
	    differ.command = opt->second;

	if ((opt = opts.find("extensions")) != opts.end())
	    differ.extensions = opt->second;

	ProxySnapshots& snapshots = snapper->getSnapshots();

	pair<ProxySnapshots::const_iterator, ProxySnapshots::const_iterator> range =
	    snapshots.findNums(get_opts.pop_arg());

	ProxyComparison comparison = snapper->createComparison(*range.first, *range.second, true);

	MyFiles files(comparison.getFiles());

	files.bulk_process(file, get_opts, [differ](const File& file) {
	    differ.run(file.getAbsolutePath(LOC_PRE), file.getAbsolutePath(LOC_POST));
	});
    }

}
