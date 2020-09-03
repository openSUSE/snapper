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


namespace snapper
{

    using namespace std;


    void
    help_create()
    {
	cout << _("  Create snapshot:") << '\n'
	     << _("\tsnapper create") << '\n'
	     << '\n'
	     << _("    Options for 'create' command:") << '\n'
	     << _("\t--type, -t <type>\t\tType for snapshot.") << '\n'
	     << _("\t--pre-number <number>\t\tNumber of corresponding pre snapshot.") << '\n'
	     << _("\t--print-number, -p\t\tPrint number of created snapshot.") << '\n'
	     << _("\t--description, -d <description>\tDescription for snapshot.") << '\n'
	     << _("\t--cleanup-algorithm, -c <algo>\tCleanup algorithm for snapshot.") << '\n'
	     << _("\t--userdata, -u <userdata>\tUserdata for snapshot.") << '\n'
	     << _("\t--command <command>\t\tRun command and create pre and post snapshots.") << endl
	     << _("\t--read-only\t\t\tCreate read-only snapshot.") << '\n'
	     << _("\t--read-write\t\t\tCreate read-write snapshot.") << '\n'
	     << _("\t--from\t\t\t\tCreate a snapshot from the specified snapshot.") << '\n'
	     << endl;
    }


    namespace
    {

	enum class CreateType { SINGLE, PRE, POST, PRE_POST };

    }


    void
    command_create(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers*, ProxySnapper* snapper)
    {
	const vector<Option> options = {
	    Option("type",			required_argument,	't'),
	    Option("pre-number",		required_argument),
	    Option("print-number",		no_argument,		'p'),
	    Option("description",		required_argument,	'd'),
	    Option("cleanup-algorithm",		required_argument,	'c'),
	    Option("userdata",			required_argument,	'u'),
	    Option("command",			required_argument),
	    Option("read-only",			no_argument),
	    Option("read-write",		no_argument),
	    Option("from",			required_argument)
	};

	ParsedOpts opts = get_opts.parse("create", options);

	const ProxySnapshots& snapshots = snapper->getSnapshots();

	CreateType type = CreateType::SINGLE;
	ProxySnapshots::const_iterator snapshot1 = snapshots.end();
	ProxySnapshots::const_iterator snapshot2 = snapshots.end();
	bool print_number = false;
	SCD scd;
	string command;
	ProxySnapshots::const_iterator parent = snapshots.getCurrent();

	ParsedOpts::const_iterator opt;

	if ((opt = opts.find("type")) != opts.end())
	{
	    if (!toValue(opt->second, type, false))
	    {
		string error = sformat(_("Unknown type '%s'."), opt->second.c_str()) + '\n' +
		    possible_enum_values<CreateType>();

		SN_THROW(OptionsException(error));
	    }
	}

	if ((opt = opts.find("pre-number")) != opts.end())
	    snapshot1 = snapshots.findNum(opt->second);

	if ((opt = opts.find("print-number")) != opts.end())
	    print_number = true;

	if ((opt = opts.find("description")) != opts.end())
	    scd.description = opt->second;

	if ((opt = opts.find("cleanup-algorithm")) != opts.end())
	    scd.cleanup = opt->second;

	if ((opt = opts.find("userdata")) != opts.end())
	    scd.userdata = read_userdata(opt->second);

	if ((opt = opts.find("command")) != opts.end())
	{
	    command = opt->second;
	    type = CreateType::PRE_POST;
	}

	if ((opt = opts.find("read-only")) != opts.end())
	    scd.read_only = true;

	if ((opt = opts.find("read-write")) != opts.end())
	    scd.read_only = false;

	if ((opt = opts.find("from")) != opts.end())
	    parent = snapshots.findNum(opt->second);

	if (type == CreateType::POST && snapshot1 == snapshots.end())
	{
	    SN_THROW(OptionsException(_("Missing or invalid pre-number.")));
	}

	if (type == CreateType::PRE_POST && command.empty())
	{
	    SN_THROW(OptionsException(_("Missing command option.")));
	}

	if (type != CreateType::SINGLE && !scd.read_only)
	{
	    SN_THROW(OptionsException(_("Option --read-write only supported for snapshots of type single.")));
	}

	if (type != CreateType::SINGLE && parent != snapshots.getCurrent())
	{
	    SN_THROW(OptionsException(_("Option --from only supported for snapshots of type single.")));
	}

	if (get_opts.has_args())
	{
	    SN_THROW(OptionsException(_("Command 'create' does not take arguments.")));
	}

	switch (type)
	{
	    case CreateType::SINGLE: {
		snapshot1 = snapper->createSingleSnapshot(parent, scd);
		if (print_number)
		    cout << snapshot1->getNum() << endl;
	    } break;

	    case CreateType::PRE: {
		snapshot1 = snapper->createPreSnapshot(scd);
		if (print_number)
		    cout << snapshot1->getNum() << endl;
	    } break;

	    case CreateType::POST: {
		snapshot2 = snapper->createPostSnapshot(snapshot1, scd);
		if (print_number)
		    cout << snapshot2->getNum() << endl;
	    } break;

	    case CreateType::PRE_POST: {
		snapshot1 = snapper->createPreSnapshot(scd);
		system(command.c_str());
		snapshot2 = snapper->createPostSnapshot(snapshot1, scd);
		if (print_number)
		    cout << snapshot1->getNum() << ".." << snapshot2->getNum() << endl;
	    } break;
	}
    }


    template <> struct EnumInfo<CreateType> { static const vector<string> names; };

    const vector<string> EnumInfo<CreateType>::names({ "single", "pre", "post", "pre-post" });

}
