/*
 * Copyright (c) [2011-2015] Novell, Inc.
 * Copyright (c) [2022-2025] SUSE LLC
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

#include <boost/algorithm/string.hpp>

#include "snapper/Filesystem.h"
#include "snapper/PluginsImpl.h"
#include "snapper/SystemCmd.h"
#include "snapper/SnapperDefines.h"
#include "snapper/Exception.h"
#include "snapper/Snapshot.h"
#include "snapper/LoggerImpl.h"


namespace snapper
{
    using namespace std;


    static bool
    plugins_filter_entries(unsigned char type, const char* name)
    {
	// must start with digit
	return *name >= '0' && *name <= '9';
    }


    void
    run_scripts(const vector<string>& args, Plugins::Report& report)
    {
	try
	{
	    SDir dir(PLUGINS_DIR);

	    vector<string> scripts = dir.entries(plugins_filter_entries);
	    std::sort(scripts.begin(), scripts.end());
	    for (const string& script : scripts)
	    {
		string fullname = dir.fullname(script);

		SystemCmd::Args cmd_args = { fullname };
		cmd_args << args;
		SystemCmd cmd(cmd_args);

		report.entries.emplace_back(fullname, args, cmd.retcode());

		y2mil("result of " << fullname << " " << boost::join(args, " "));

		for (const string& line : cmd.get_stdout())
		    y2mil("stdout: " << line);

		for (const string& line : cmd.get_stderr())
		    y2mil("stderr: " << line);

		if (cmd.retcode() != 0)
		    y2err("return code: " << cmd.retcode());
		else
		    y2mil("return code: " << cmd.retcode());
	    }
	}
	catch (const Exception& e)
	{
	    SN_CAUGHT(e);
	}
    }


    void
    grub(const string& subvolume, const Filesystem* filesystem, const char* option, Plugins::Report& report)
    {
#ifdef ENABLE_ROLLBACK

#define GRUB_SCRIPT "/usr/lib/snapper/plugins/grub"

	if (subvolume == "/" && filesystem->fstype() == "btrfs" && access(GRUB_SCRIPT, X_OK) == 0)
	{
	    SystemCmd cmd({ GRUB_SCRIPT, option });
	    report.entries.emplace_back(GRUB_SCRIPT, vector<string>{ option }, cmd.retcode());
	}
#endif
    }


    // Actions without -pre/-post are legacy and deprecated (2022-12-22).


    void
    Plugins::create_config(Stage stage, const string& subvolume, const Filesystem* filesystem, Report& report)
    {
	switch (stage)
	{
	    case Stage::PRE_ACTION:
		run_scripts({ "create-config-pre", subvolume, filesystem->fstype() }, report);
		break;

	    case Stage::POST_ACTION:
		grub(subvolume, filesystem, "--enable", report);
		run_scripts({ "create-config", subvolume, filesystem->fstype() }, report);
		run_scripts({ "create-config-post", subvolume, filesystem->fstype() }, report);
		break;
	}
    }


    void
    Plugins::delete_config(Stage stage, const string& subvolume, const Filesystem* filesystem, Report& report)
    {
	switch (stage)
	{
	    case Stage::PRE_ACTION:
		grub(subvolume, filesystem, "--disable", report);
		run_scripts({ "delete-config-pre", subvolume, filesystem->fstype() }, report);
		run_scripts({ "delete-config", subvolume, filesystem->fstype() }, report);
		break;

	    case Stage::POST_ACTION:
		run_scripts({ "delete-config-post", subvolume, filesystem->fstype() }, report);
		break;
	}
    }


    void
    Plugins::create_snapshot(Stage stage, const string& subvolume, const Filesystem* filesystem,
			     const Snapshot& snapshot, Report& report)
    {
	switch (stage)
	{
	    case Stage::PRE_ACTION:
		run_scripts({ "create-snapshot-pre", subvolume, filesystem->fstype(),
			std::to_string(snapshot.getNum()) }, report);
		break;

	    case Stage::POST_ACTION:
		grub(subvolume, filesystem, "--refresh", report);
		run_scripts({ "create-snapshot", subvolume, filesystem->fstype(),
			std::to_string(snapshot.getNum()) }, report);
		run_scripts({ "create-snapshot-post", subvolume, filesystem->fstype(),
			std::to_string(snapshot.getNum()) }, report);
		break;
	}
    }


    void
    Plugins::modify_snapshot(Stage stage, const string& subvolume, const Filesystem* filesystem,
			     const Snapshot& snapshot, Report& report)
    {
	switch (stage)
	{
	    case Stage::PRE_ACTION:
		run_scripts({ "modify-snapshot-pre", subvolume, filesystem->fstype(),
			std::to_string(snapshot.getNum()) }, report);
		break;

	    case Stage::POST_ACTION:
		grub(subvolume, filesystem, "--refresh", report);
		run_scripts({ "modify-snapshot", subvolume, filesystem->fstype(),
			std::to_string(snapshot.getNum()) }, report);
		run_scripts({ "modify-snapshot-post", subvolume, filesystem->fstype(),
			std::to_string(snapshot.getNum()) }, report);
		break;
	}
    }


    void
    Plugins::delete_snapshot(Stage stage, const string& subvolume, const Filesystem* filesystem,
			     const Snapshot& snapshot, Report& report)
    {
	switch (stage)
	{
	    case Stage::PRE_ACTION:
		run_scripts({ "delete-snapshot-pre", subvolume, filesystem->fstype(),
			std::to_string(snapshot.getNum()) }, report);
		break;

	    case Stage::POST_ACTION:
		grub(subvolume, filesystem, "--refresh", report);
		run_scripts({ "delete-snapshot", subvolume, filesystem->fstype(),
			std::to_string(snapshot.getNum()) }, report);
		run_scripts({ "delete-snapshot-post", subvolume, filesystem->fstype(),
			std::to_string(snapshot.getNum()) }, report);
		break;
	}
    }


    void
    Plugins::set_default_snapshot(Stage stage, const string& subvolume, const Filesystem* filesystem,
				  unsigned int num, Report& report)
    {
	switch (stage)
	{
	    case Stage::PRE_ACTION:
		run_scripts({ "set-default-snapshot-pre", subvolume, filesystem->fstype(),
			std::to_string(num) }, report);
		break;

	    case Stage::POST_ACTION:
		run_scripts({ "set-default-snapshot", subvolume, filesystem->fstype(),
			std::to_string(num) }, report);
		run_scripts({ "set-default-snapshot-post", subvolume, filesystem->fstype(),
			std::to_string(num) }, report);
		break;
	}
    }


    void
    Plugins::rollback(const string& old_root, const string& new_root, Report& report)
    {
#ifdef ENABLE_ROLLBACK

#define ROLLBACK_SCRIPT "/usr/lib/snapper/plugins/rollback"

	// Fate#319108
	if (access(ROLLBACK_SCRIPT, X_OK) == 0)
	{
	    SystemCmd cmd({ ROLLBACK_SCRIPT, old_root, new_root });
	    report.entries.emplace_back(ROLLBACK_SCRIPT, vector<string>{ old_root, new_root }, cmd.retcode());
	}
#endif
    }


    void
    Plugins::rollback(Stage stage, const string& subvolume, const Filesystem* filesystem, unsigned int old_num,
		      unsigned int new_num, Report& report)
    {
	switch (stage)
	{
	    case Stage::PRE_ACTION:
		run_scripts({ "rollback-pre", subvolume, filesystem->fstype(), std::to_string(old_num),
			std::to_string(new_num) }, report);
		break;

	    case Stage::POST_ACTION:
		run_scripts({ "rollback", subvolume, filesystem->fstype(), std::to_string(old_num),
			std::to_string(new_num) }, report);
		run_scripts({ "rollback-post", subvolume, filesystem->fstype(), std::to_string(old_num),
			std::to_string(new_num) }, report);
		break;
	}
    }

}
