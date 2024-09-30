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

#include "../utils/text.h"
#include "../utils/help.h"
#include "../proxy/proxy.h"
#include "GlobalOptions.h"


namespace snapper
{

    using namespace std;


    void
    help_create_config()
    {
	cout << _("  Create config:") << '\n'
	     << _("\tsnapper create-config <subvolume>") << '\n'
	     << '\n'
	     << _("    Options for 'create-config' command:") << '\n';

	print_options({
	    { _("--fstype, -f <fstype>"), _("Manually set filesystem type.") },
	    { _("--template, -t <name>"), _("Name of config template to use.") }
	});
    }


    void
    command_create_config(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers* snappers,
			  ProxySnapper*, Plugins::Report& report)
    {
	const vector<Option> options = {
	    Option("fstype",	required_argument,	'f'),
	    Option("template",	required_argument,	't')
	};

	ParsedOpts opts = get_opts.parse("create-config", options);
	if (get_opts.num_args() != 1)
	{
	    cerr << _("Command 'create-config' needs one argument.") << endl;
	    exit(EXIT_FAILURE);
	}

	string subvolume = realpath(get_opts.pop_arg());
	if (subvolume.empty())
	{
	    cerr << _("Invalid subvolume.") << endl;
	    exit(EXIT_FAILURE);
	}

	string fstype = "";
	string template_name = "default";

	ParsedOpts::const_iterator opt;

	if ((opt = opts.find("fstype")) != opts.end())
	    fstype = opt->second;

	if ((opt = opts.find("template")) != opts.end())
	    template_name = opt->second;

	if (fstype.empty() && !Snapper::detectFstype(subvolume, fstype))
	{
	    cerr << _("Detecting filesystem type failed.") << endl;
	    exit(EXIT_FAILURE);
	}

	snappers->createConfig(global_options.config(), subvolume, fstype, template_name, report);
    }

}
