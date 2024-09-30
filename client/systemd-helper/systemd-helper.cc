/*
 * Copyright (c) [2014-2015] Novell, Inc.
 * Copyright (c) [2016-2024] SUSE LLC
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

#include <cstdlib>
#include <iostream>

#include <snapper/SystemCmd.h>
#include <snapper/SnapperDefines.h>

#include "../utils/text.h"
#include "../utils/GetOpts.h"
#include "../proxy/proxy.h"
#include "../proxy/errors.h"

#include "../cleanup.h"
#include "../misc.h"


using namespace snapper;
using namespace std;


// cout and cerr are visible with 'journalctl' and 'systemctl status
// snapper-<name>.service'.


Plugins::Report report;


bool
call_with_error_check(std::function<void()> func)
{
    try
    {
	func();
	return true;
    }
    catch (const DBus::ErrorException& e)
    {
	cerr << error_description(e) << endl;
	return false;
    }
    catch (const DBus::FatalException& e)
    {
	cerr << "failure (" << e.what() << ")." << endl;
	return false;
    }
}


bool
timeline(ProxySnappers* snappers, const map<string, string>& userdata)
{
    bool ok = true;

    map<string, ProxyConfig> configs = snappers->getConfigs();
    for (const map<string, ProxyConfig>::value_type& value : configs)
    {
	const ProxyConfig& proxy_config = value.second;

	if (!proxy_config.is_yes("TIMELINE_CREATE"))
	    continue;

	cout << "Running timeline for '" << value.first << "'." << endl;

	ProxySnapper* snapper = nullptr;

	if (!call_with_error_check([&snappers, &snapper, &value](){ snapper = snappers->getSnapper(value.first); }))
	{
	    cerr << "timeline for '" << value.first << "' failed." << endl;
	    ok = false;
	    continue;
	}

	SCD scd;
	scd.description = "timeline";
	scd.cleanup = "timeline";
	scd.userdata = userdata;

	if (!call_with_error_check([snapper, scd](){ snapper->createSingleSnapshot(scd, report); }))
	{
	    cerr << "timeline for '" << value.first << "' failed." << endl;
	    ok = false;
	}
    }

    return ok;
}


bool
cleanup(ProxySnappers* snappers)
{
    const bool verbose = true;

    bool ok = true;

    map<string, ProxyConfig> configs = snappers->getConfigs();
    for (const map<string, ProxyConfig>::value_type& value : configs)
    {
	const ProxyConfig& proxy_config = value.second;

	bool do_number = proxy_config.is_yes("NUMBER_CLEANUP");
	bool do_timeline = proxy_config.is_yes("TIMELINE_CLEANUP");
	bool do_empty_pre_post = proxy_config.is_yes("EMPTY_PRE_POST_CLEANUP");

	if (!do_number && !do_timeline && !do_empty_pre_post)
	    continue;

	cout << "Running cleanup for '" << value.first << "'." << endl;

	ProxySnapper* snapper = nullptr;

	if (!call_with_error_check([&snappers, &snapper, &value](){ snapper = snappers->getSnapper(value.first); }))
	{
	    cerr << "cleanup for '" << value.first << "' failed." << endl;
	    ok = false;
	    continue;
	}

	if (do_number)
	{
	    cout << "Running number cleanup for '" << value.first << "'." << endl;

	    if (!call_with_error_check([snapper](){ do_cleanup_number(snapper, verbose, report); }))
	    {
		cerr << "number cleanup for '" << value.first << "' failed." << endl;
		ok = false;
	    }
	}

	if (do_timeline)
	{
	    cout << "Running timeline cleanup for '" << value.first << "'." << endl;

	    if (!call_with_error_check([snapper](){ do_cleanup_timeline(snapper, verbose, report); }))
	    {
		cerr << "timeline cleanup for '" << value.first << "' failed." << endl;
		ok = false;
	    }
	}

	if (do_empty_pre_post)
	{
	    cout << "Running empty-pre-post cleanup for '" << value.first << "'." << endl;

	    if (!call_with_error_check([snapper](){ do_cleanup_empty_pre_post(snapper, verbose, report); }))
	    {
		cerr << "empty-pre-post cleanup for " << value.first << " failed." << endl;
		ok = false;
	    }
	}

	string fstype;
	if (proxy_config.getValue(KEY_FSTYPE, fstype) && fstype == "btrfs")
	{
	    string subvolume;
	    if (proxy_config.getValue(KEY_SUBVOLUME, subvolume))
	    {
		string general_dir = (subvolume == "/" ? "" : subvolume) + "/" SNAPSHOTS_NAME;

		cout << "Running 'btrfs qgroup clear-stale " << general_dir << "'." << endl;

		SystemCmd cmd({ BTRFS_BIN, "qgroup", "clear-stale", general_dir });
		if (cmd.retcode() != 0)
		{
		    // This fails more often than not since qgroups of just deleted
		    // subvolume are busy. So do not set an error code here. Still log the
		    // failure to help people understand this stuff.

		    cerr << "'btrfs qgroup clear-stale " << general_dir << "' failed." << endl;
		}
	    }
	}
    }

    return ok;
}


int
main(int argc, char** argv)
{
    setlocale(LC_ALL, "");

    bool do_timeline = false;
    bool do_cleanup = false;
    map<string, string> userdata;

    try
    {
	const vector<Option> options = {
	    Option("timeline",		no_argument),
	    Option("cleanup",		no_argument),
	    Option("userdata",		required_argument,	'u')
	};

	GetOpts get_opts(argc, argv);

	ParsedOpts opts = get_opts.parse(options);

	ParsedOpts::const_iterator opt;

	if (opts.has_option("timeline"))
	    do_timeline = true;

	if (opts.has_option("cleanup"))
	    do_cleanup = true;

	if ((opt = opts.find("userdata")) != opts.end())
	    userdata = read_userdata(opt->second);
    }
    catch (const OptionsException& e)
    {
	SN_CAUGHT(e);
	cerr << e.what() << endl;
	exit(EXIT_FAILURE);
    }

    bool ok = true;

    if (!call_with_error_check([do_timeline, do_cleanup, userdata, &ok]() {

	ProxySnappers snappers(ProxySnappers::createDbus());

	if (do_timeline)
	    if (!timeline(&snappers, userdata))
		ok = false;

	if (do_cleanup)
	    if (!cleanup(&snappers))
		ok = false;

    }))
    {
	ok = false;
    }

    for (const Plugins::Report::Entry& entry : report.entries)
    {
	if (entry.exit_status != 0)
	{
	    cerr << "server-side plugin '" << entry.name << "' failed\n";
	    ok = false;
	}
    }

    exit(ok ? EXIT_SUCCESS : EXIT_FAILURE);
}
