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
#include "MyFiles.h"


namespace snapper
{

    using namespace std;


    void
    help_undochange()
    {
	cout << _("  Undo changes:") << '\n'
	     << _("\tsnapper undochange <number1>..<number2> [files]") << '\n'
	     << '\n'
	     << _("    Options for 'undochange' command:") << '\n'
	     << _("\t--input, -i <file>\t\tRead files for which to undo changes from file.") << '\n'
	     << endl;
    }


    void
    command_undochange(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers*,
		       ProxySnapper* snapper, Plugins::Report& report)
    {
	const vector<Option> options = {
	    Option("input",		required_argument,	'i')
	};

	ParsedOpts opts = get_opts.parse("undochange", options);
	if (get_opts.num_args() < 1)
	{
	    cerr << _("Command 'undochange' needs at least one argument.") << endl;
	    exit(EXIT_FAILURE);
	}

	ProxySnapshots& snapshots = snapper->getSnapshots();

	pair<ProxySnapshots::const_iterator, ProxySnapshots::const_iterator> range =
	    snapshots.findNums(get_opts.pop_arg());

	FILE* file = NULL;

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

	if (range.first->isCurrent())
	{
	    cerr << _("Invalid snapshots.") << endl;
	    exit(EXIT_FAILURE);
	}

	ProxyComparison comparison = snapper->createComparison(*range.first, *range.second, true);

	MyFiles files(comparison.getFiles());

	files.bulk_process(file, get_opts, [](File& file) {
	    file.setUndo(true);
	});

	UndoStatistic undo_statistic = files.getUndoStatistic();

	if (undo_statistic.empty())
	{
	    cout << _("nothing to do") << endl;
	    return;
	}

	cout << sformat(_("create:%d modify:%d delete:%d"), undo_statistic.numCreate,
			undo_statistic.numModify, undo_statistic.numDelete) << endl;

	vector<UndoStep> undo_steps = files.getUndoSteps();

	for (vector<UndoStep>::const_iterator it1 = undo_steps.begin(); it1 != undo_steps.end(); ++it1)
	{
	    vector<File>::iterator it2 = files.find(it1->name);
	    if (it2 == files.end())
	    {
		cerr << "internal error" << endl;
		exit(EXIT_FAILURE);
	    }

	    if (it1->action != it2->getAction())
	    {
		cerr << "internal error" << endl;
		exit(EXIT_FAILURE);
	    }

	    if (global_options.verbose())
	    {
		switch (it1->action)
		{
		    case CREATE:
			cout << sformat(_("creating %s"), it2->getAbsolutePath(LOC_SYSTEM).c_str()) << endl;
			break;
		    case MODIFY:
			cout << sformat(_("modifying %s"), it2->getAbsolutePath(LOC_SYSTEM).c_str()) << endl;
			break;
		    case DELETE:
			cout << sformat(_("deleting %s"), it2->getAbsolutePath(LOC_SYSTEM).c_str()) << endl;
			break;
		}
	    }

	    if (!it2->doUndo())
	    {
		switch (it1->action)
		{
		    case CREATE:
			cerr << sformat(_("failed to create %s"), it2->getAbsolutePath(LOC_SYSTEM).c_str()) << endl;
			break;
		    case MODIFY:
			cerr << sformat(_("failed to modify %s"), it2->getAbsolutePath(LOC_SYSTEM).c_str()) << endl;
			break;
		    case DELETE:
			cerr << sformat(_("failed to delete %s"), it2->getAbsolutePath(LOC_SYSTEM).c_str()) << endl;
			break;
		}
	    }
	}
    }

}
