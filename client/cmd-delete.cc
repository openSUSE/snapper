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
#include "proxy/proxy.h"
#include "GlobalOptions.h"


namespace snapper
{

    using namespace std;


    void
    help_delete()
    {
	cout << _("  Delete snapshot:") << '\n'
	     << _("\tsnapper delete <number>") << '\n'
	     << '\n'
	     << _("    Options for 'delete' command:") << '\n';

	print_options({
	    { _("--sync, -s"), _("Sync after deletion.") }
	});
    }


    void
    filter_undeletables(ProxySnapshots& snapshots, vector<ProxySnapshots::iterator>& nums)
    {
	auto filter = [&snapshots, &nums](ProxySnapshots::const_iterator undeletable, const char* message)
	    {
		if (undeletable == snapshots.end())
		    return;

		unsigned int num = undeletable->getNum();

		vector<ProxySnapshots::iterator>::iterator keep = find_if(nums.begin(), nums.end(),
		    [num](ProxySnapshots::iterator it){ return num == it->getNum(); });

		if (keep != nums.end())
		{
		    cerr << sformat(message, num) << endl;
		    nums.erase(keep);
		}
	    };

	ProxySnapshots::const_iterator current_snapshot = snapshots.begin();
	filter(current_snapshot, _("Cannot delete snapshot %d since it is the current system."));

	ProxySnapshots::const_iterator active_snapshot = snapshots.getActive();
	filter(active_snapshot, _("Cannot delete snapshot %d since it is the currently mounted snapshot."));

	ProxySnapshots::const_iterator default_snapshot = snapshots.getDefault();
	filter(default_snapshot, _("Cannot delete snapshot %d since it is the next to be mounted snapshot."));
    }


    void
    command_delete(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers*,
		   ProxySnapper* snapper, Plugins::Report& report)
    {
	const vector<Option> options = {
	    Option("sync",	no_argument,	's')
	};

	ParsedOpts opts = get_opts.parse("delete", options);

	bool sync = false;

	ParsedOpts::const_iterator opt;

	if ((opt = opts.find("sync")) != opts.end())
	    sync = true;

	if (!get_opts.has_args())
	{
	    SN_THROW(OptionsException(_("Command 'delete' needs at least one argument.")));
	}

	ProxySnapshots& snapshots = snapper->getSnapshots();

	vector<ProxySnapshots::iterator> nums;

	while (get_opts.has_args())
	{
	    string arg = get_opts.pop_arg();

	    if (arg.find_first_of("-") == string::npos)
	    {
		ProxySnapshots::iterator tmp = snapshots.findNum(arg);
		nums.push_back(tmp);
	    }
	    else
	    {
		pair<ProxySnapshots::iterator, ProxySnapshots::iterator> range =
		    snapshots.findNums(arg, "-");

		if (range.first->getNum() > range.second->getNum())
		    swap(range.first, range.second);

		for (unsigned int i = range.first->getNum(); i <= range.second->getNum(); ++i)
		{
		    ProxySnapshots::iterator x = snapshots.find(i);
		    if (x != snapshots.end())
		    {
			if (find_if(nums.begin(), nums.end(), [i](ProxySnapshots::iterator it)
								  { return it->getNum() == i; }) == nums.end())
			    nums.push_back(x);
		    }
		}
	    }
	}

	filter_undeletables(snapshots, nums);

	snapper->deleteSnapshots(nums, global_options.verbose(), report);

	if (sync)
	    snapper->syncFilesystem();
    }

}
