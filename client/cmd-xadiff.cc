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


#include "config.h"

#include <iostream>

#include <snapper/XAttributes.h>

#include "utils/text.h"
#include "GlobalOptions.h"
#include "proxy.h"
#include "MyFiles.h"


namespace snapper
{

    using namespace std;


#ifdef ENABLE_XATTRS

    void
    help_xadiff()
    {
	cout << _("  Comparing snapshots extended attributes:") << '\n'
	     << _("\tsnapper xadiff <number1>..<number2> [files]") << '\n'
	     << endl;
    }

    void
    print_xadiff(const string loc_pre, const string loc_post)
    {
	try
	{
	    XAModification xa_mod = XAModification(XAttributes(loc_pre), XAttributes(loc_post));

	    if (!xa_mod.empty())
	    {
		cout << "--- " << loc_pre << endl << "+++ " << loc_post << endl;
		xa_mod.dumpDiffReport(cout);
	    }
	}
	catch (const XAttributesException& e)
	{
	    SN_CAUGHT(e);
	}
    }

    void
    command_xadiff(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers* snappers, ProxySnapper* snapper)
    {
	ParsedOpts opts = get_opts.parse("xadiff", GetOpts::no_options);
	if (get_opts.num_args() < 1)
	{
	    cerr << _("Command 'xadiff' needs at least one argument.") << endl;
	    exit(EXIT_FAILURE);
	}

	ProxySnapshots& snapshots = snapper->getSnapshots();

	pair<ProxySnapshots::const_iterator, ProxySnapshots::const_iterator> range =
	    snapshots.findNums(get_opts.pop_arg());

	ProxyComparison comparison = snapper->createComparison(*range.first, *range.second, true);

	MyFiles files(comparison.getFiles());

	if (get_opts.num_args() == 0)
	{
	    for (Files::const_iterator it1 = files.begin(); it1 != files.end(); ++it1)
		if (it1->getPreToPostStatus() & XATTRS)
		    print_xadiff(it1->getAbsolutePath(LOC_PRE), it1->getAbsolutePath(LOC_POST));
	}
	else
	{
	    while (get_opts.num_args() > 0)
	    {
		string name = get_opts.pop_arg();

		Files::const_iterator it1 = files.findAbsolutePath(name);
		if (it1 == files.end())
		    continue;

		if (it1->getPreToPostStatus() & XATTRS)
		    print_xadiff(it1->getAbsolutePath(LOC_PRE), it1->getAbsolutePath(LOC_POST));
	    }
	}
    }

#endif

}
