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

#include <snapper/AppUtil.h>
#include <snapper/Filesystem.h>
#include <snapper/PluginsImpl.h>

#include "utils/text.h"
#include "GlobalOptions.h"
#include "proxy.h"
#include "misc.h"


namespace snapper
{

    using namespace std;


#ifdef ENABLE_ROLLBACK

    void
    help_rollback()
    {
	cout << _("  Rollback:") << '\n'
	     << _("\tsnapper rollback [number]") << '\n'
	     << '\n'
	     << _("    Options for 'rollback' command:") << '\n'
	     << _("\t--print-number, -p\t\tPrint number of second created snapshot.") << '\n'
	     << _("\t--description, -d <description>\tDescription for snapshots.") << '\n'
	     << _("\t--cleanup-algorithm, -c <algo>\tCleanup algorithm for snapshots.") << '\n'
	     << _("\t--userdata, -u <userdata>\tUserdata for snapshots.") << '\n'
	     << endl;
    }


    void
    command_rollback(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers*, ProxySnapper* snapper,
		     Plugins::Report& report)
    {
	const vector<Option> options = {
	    Option("print-number",		no_argument,		'p'),
	    Option("description",		required_argument,	'd'),
	    Option("cleanup-algorithm",		required_argument,	'c'),
	    Option("userdata",			required_argument,	'u')
	};

	ParsedOpts opts = get_opts.parse("rollback", options);
	if (get_opts.has_args() && get_opts.num_args() != 1)
	{
	    cerr << _("Command 'rollback' takes either one or no argument.") << endl;
	    exit(EXIT_FAILURE);
	}

	const string default_description1 = "rollback backup";
	const string default_description2 = "writable copy";

	bool print_number = false;

	SCD scd1;
	scd1.description = default_description1;
	scd1.cleanup = "number";
	scd1.userdata["important"] = "yes";

	SCD scd2;
	scd2.description = default_description2;

	ParsedOpts::const_iterator opt;

	if ((opt = opts.find("print-number")) != opts.end())
	    print_number = true;

	if ((opt = opts.find("description")) != opts.end())
	    scd1.description = scd2.description = opt->second;

	if ((opt = opts.find("cleanup-algorithm")) != opts.end())
	    scd1.cleanup = scd2.cleanup = opt->second;

	if ((opt = opts.find("userdata")) != opts.end())
	{
	    scd1.userdata = read_userdata(opt->second, scd1.userdata);
	    scd2.userdata = read_userdata(opt->second);
	}

	ProxyConfig config = snapper->getConfig();

	const Filesystem* filesystem = get_filesystem(config, global_options.root());
	if (filesystem->fstype() != "btrfs")
	{
	    cerr << _("Command 'rollback' only available for btrfs.") << endl;
	    exit(EXIT_FAILURE);
	}

	const string subvolume = config.getSubvolume();
	if (subvolume != "/")
	{
	    cerr << sformat(_("Command 'rollback' cannot be used on a non-root subvolume %s."),
			    subvolume.c_str()) << endl;
	    exit(EXIT_FAILURE);
	}

	ProxySnapshots& snapshots = snapper->getSnapshots();

	ProxySnapshots::iterator previous_default = snapshots.getDefault();

	if (global_options.ambit() == GlobalOptions::Ambit::AUTO)
	{
	    if (previous_default == snapshots.end())
	    {
		cerr << _("Cannot detect ambit since default subvolume is unknown.") << '\n'
		     << _("This can happen if the system was not set up for rollback.") << '\n'
		     << _("The ambit can be specified manually using the --ambit option.") << endl;
		exit(EXIT_FAILURE);
	    }

	    if (filesystem->isSnapshotReadOnly(previous_default->getNum()))
		global_options.set_ambit(GlobalOptions::Ambit::TRANSACTIONAL);
	    else
		global_options.set_ambit(GlobalOptions::Ambit::CLASSIC);
	}

	if (!global_options.quiet())
	    cout << sformat(_("Ambit is %s."), toString(global_options.ambit()).c_str()) << endl;

	if (previous_default != snapshots.end() && scd1.description == default_description1)
	    scd1.description += sformat(" of #%d", previous_default->getNum());

	switch (global_options.ambit())
	{
	    case GlobalOptions::Ambit::CLASSIC:
	    {
		ProxySnapshots::const_iterator snapshot1 = snapshots.end();
		ProxySnapshots::const_iterator snapshot2 = snapshots.end();

		if (get_opts.num_args() == 0)
		{
		    if (!global_options.quiet())
			cout << _("Creating read-only snapshot of default subvolume.") << flush;

		    scd1.read_only = true;
		    snapshot1 = snapper->createSingleSnapshotOfDefault(scd1, report);

		    if (!global_options.quiet())
			cout << " " << sformat(_("(Snapshot %d.)"), snapshot1->getNum()) << endl;

		    if (!global_options.quiet())
			cout << _("Creating read-write snapshot of current subvolume.") << flush;

		    ProxySnapshots::const_iterator active = snapshots.getActive();
		    if (active != snapshots.end() && scd2.description == default_description2)
			scd2.description += sformat(" of #%d", active->getNum());

		    scd2.read_only = false;
		    snapshot2 = snapper->createSingleSnapshot(snapshots.getCurrent(), scd2, report);

		    if (!global_options.quiet())
			cout << " " << sformat(_("(Snapshot %d.)"), snapshot2->getNum()) << endl;
		}
		else
		{
		    ProxySnapshots::const_iterator tmp = snapshots.findNum(get_opts.pop_arg());

		    if (!global_options.quiet())
			cout << _("Creating read-only snapshot of current system.") << flush;

		    snapshot1 = snapper->createSingleSnapshot(scd1, report);

		    if (!global_options.quiet())
			cout << " " << sformat(_("(Snapshot %d.)"), snapshot1->getNum()) << endl;

		    if (!global_options.quiet())
			cout << sformat(_("Creating read-write snapshot of snapshot %d."), tmp->getNum()) << flush;

		    if (tmp != snapshots.end() && scd2.description == default_description2)
			scd2.description += sformat(" of #%d", tmp->getNum());

		    scd2.read_only = false;
		    snapshot2 = snapper->createSingleSnapshot(tmp, scd2, report);

		    if (!global_options.quiet())
			cout << " " << sformat(_("(Snapshot %d.)"), snapshot2->getNum()) << endl;
		}

		if (previous_default != snapshots.end() && previous_default->getCleanup().empty())
		{
		    SMD smd = previous_default->getSmd();
		    smd.cleanup = "number";
		    snapper->modifySnapshot(previous_default, smd, report);
		}

		if (!global_options.quiet())
		    cout << sformat(_("Setting default subvolume to snapshot %d."), snapshot2->getNum()) << endl;

		filesystem->setDefault(snapshot2->getNum(), report);

		Plugins::rollback(filesystem->snapshotDir(snapshot1->getNum()),
				  filesystem->snapshotDir(snapshot2->getNum()), report);
		Plugins::rollback(Plugins::Stage::POST_ACTION, subvolume, filesystem, snapshot1->getNum(),
				  snapshot2->getNum(), report);

		if (print_number)
		    cout << snapshot2->getNum() << endl;
	    }
	    break;

	    case GlobalOptions::Ambit::TRANSACTIONAL:
	    {
		// see bsc #1172273

		if (previous_default == snapshots.end())
		{
		    cerr << _("Cannot do rollback since default subvolume is unknown.") << endl;
		    exit(EXIT_FAILURE);
		}

		ProxySnapshots::iterator snapshot = snapshots.end();

		if (get_opts.num_args() == 0)
		{
		    snapshot = snapshots.getActive();
		}
		else
		{
		    snapshot = snapshots.findNum(get_opts.pop_arg());
		}

		if (previous_default == snapshot)
		{
		    cerr << _("Active snapshot is already default snapshot.") << endl;
		    exit(EXIT_FAILURE);
		}

		SMD smd = snapshot->getSmd();
		smd.cleanup = "";
		snapper->modifySnapshot(snapshot, smd, report);

		if (!global_options.quiet())
		    cout << sformat(_("Setting default subvolume to snapshot %d."), snapshot->getNum()) << endl;

		filesystem->setDefault(snapshot->getNum(), report);

		Plugins::rollback(filesystem->snapshotDir(previous_default->getNum()),
				  filesystem->snapshotDir(snapshot->getNum()), report);
		Plugins::rollback(Plugins::Stage::POST_ACTION, subvolume, filesystem, previous_default->getNum(),
				  snapshot->getNum(), report);
	    }
	    break;

	    case GlobalOptions::Ambit::AUTO:
	    {
		cerr << "internal error: ambit is auto" << endl;
		exit(EXIT_FAILURE);
	    }
	    break;
	}
    }

#endif

}
