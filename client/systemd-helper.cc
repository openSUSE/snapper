/*
 * Copyright (c) [2014-2015] Novell, Inc.
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

#include "commands.h"
#include "cleanup.h"
#include "errors.h"
#include "misc.h"


using namespace snapper;
using namespace std;


bool do_timeline = false;
bool do_cleanup = false;


void
timeline(DBus::Connection* conn, const map<string, string>& userdata)
{
    list<XConfigInfo> config_infos = command_list_xconfigs(*conn);
    for (const XConfigInfo& config_info : config_infos)
    {
	map<string, string>::const_iterator pos1 = config_info.raw.find("TIMELINE_CREATE");
	if (pos1 != config_info.raw.end() && pos1->second == "yes")
	{
	    command_create_single_xsnapshot(*conn, config_info.config_name, "timeline",
					    "timeline", userdata);
	}
    }
}


void
cleanup(DBus::Connection* conn)
{
    list<XConfigInfo> config_infos = command_list_xconfigs(*conn);
    for (const XConfigInfo& config_info : config_infos)
    {
	map<string, string>::const_iterator pos1 = config_info.raw.find("NUMBER_CLEANUP");
	if (pos1 != config_info.raw.end() && pos1->second == "yes")
	{
	    do_cleanup_number(*conn, config_info.config_name, false);
	}

	map<string, string>::const_iterator pos2 = config_info.raw.find("TIMELINE_CLEANUP");
	if (pos2 != config_info.raw.end() && pos2->second == "yes")
	{
	    do_cleanup_timeline(*conn, config_info.config_name, false);
	}

	map<string, string>::const_iterator pos3 = config_info.raw.find("EMPTY_PRE_POST_CLEANUP");
	if (pos3 != config_info.raw.end() && pos3->second == "yes")
	{
	    do_cleanup_empty_pre_post(*conn, config_info.config_name, false);
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
	DBus::Connection conn(DBUS_BUS_SYSTEM);

	if (do_timeline)
	    timeline(&conn, userdata);

	if (do_cleanup)
	    cleanup(&conn);
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
