/*
 * Copyright (c) [2014-2015] Novell, Inc.
 * Copyright (c) 2016 SUSE LLC
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

#include <stdlib.h>
#include <iostream>

#include "utils/text.h"
#include "utils/GetOpts.h"

#include "proxy.h"
#include "cleanup.h"
#include "errors.h"
#include "misc.h"


using namespace snapper;
using namespace std;


void
timeline(ProxySnappers* snappers, const map<string, string>& userdata)
{
    map<string, ProxyConfig> configs = snappers->getConfigs();
    for (const map<string, ProxyConfig>::value_type value : configs)
    {
	try
	{
	    const map<string, string>& raw = value.second.getAllValues();

	    map<string, string>::const_iterator pos1 = raw.find("TIMELINE_CREATE");
	    if (pos1 != raw.end() && pos1->second == "yes")
	    {
		ProxySnapper* snapper = snappers->getSnapper(value.first);

		SCD scd;
		scd.description = "timeline";
		scd.cleanup = "timeline";
		scd.userdata = userdata;

		snapper->createSingleSnapshot(scd);
	    }
	}
	catch (const DBus::ErrorException& e)
	{
	    cerr << "Error processing config '" << value.first << "': " << error_description(e) << endl;
	}
	catch (const exception& e)
	{
	    cerr << _("Failure") << " processing config '" << value.first << "' (" << e.what() << ")." << endl;
	    exit(EXIT_FAILURE);
	}
    }
}


void
cleanup(ProxySnappers* snappers)
{
    map<string, ProxyConfig> configs = snappers->getConfigs();
    for (const map<string, ProxyConfig>::value_type value : configs)
    {
	const map<string, string>& raw = value.second.getAllValues();

	ProxySnapper* snapper = snappers->getSnapper(value.first);

	map<string, string>::const_iterator pos1 = raw.find("NUMBER_CLEANUP");
	if (pos1 != raw.end() && pos1->second == "yes")
	{
	    do_cleanup_number(snapper, false);
	}

	map<string, string>::const_iterator pos2 = raw.find("TIMELINE_CLEANUP");
	if (pos2 != raw.end() && pos2->second == "yes")
	{
	    do_cleanup_timeline(snapper, false);
	}

	map<string, string>::const_iterator pos3 = raw.find("EMPTY_PRE_POST_CLEANUP");
	if (pos3 != raw.end() && pos3->second == "yes")
	{
	    do_cleanup_empty_pre_post(snapper, false);
	}
    }
}


int
main(int argc, char** argv)
{
    setlocale(LC_ALL, "");

    const struct option options[] = {
	{ "timeline",		no_argument,		0,	0 },
	{ "cleanup",		no_argument,		0,	0 },
	{ "userdata",		required_argument,	0,	'u' },
	{ 0, 0, 0, 0 }
    };

    bool do_timeline = false;
    bool do_cleanup = false;

    map<string, string> userdata;

    GetOpts getopts;

    getopts.init(argc, argv);

    GetOpts::parsed_opts opts = getopts.parse(options);

    GetOpts::parsed_opts::const_iterator opt;

    if (opts.find("timeline") != opts.end())
	do_timeline = true;

    if (opts.find("cleanup") != opts.end())
	do_cleanup = true;

    if ((opt = opts.find("userdata")) != opts.end())
	userdata = read_userdata(opt->second);

    try
    {
	ProxySnappers snappers(ProxySnappers::createDbus());

	if (do_timeline)
	    timeline(&snappers, userdata);

	if (do_cleanup)
	    cleanup(&snappers);
    }
    catch (const DBus::ErrorException& e)
    {
	cerr << error_description(e) << endl;
	exit(EXIT_FAILURE);
    }
    catch (const DBus::FatalException& e)
    {
	cerr << _("Failure") << " (" << e.what() << ")." << endl;
	exit(EXIT_FAILURE);
    }
}
