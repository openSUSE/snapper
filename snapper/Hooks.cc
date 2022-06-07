/*
 * Copyright (c) [2011-2015] Novell, Inc.
 * Copyright (c) 2022 SUSE LLC
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

#include "snapper/FileUtils.h"
#include "snapper/Hooks.h"
#include "snapper/SystemCmd.h"
#include "snapper/SnapperDefines.h"


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
    Hooks::run_scripts(const vector<string>& args)
    {
	try
	{
	    SDir dir(PLUGINS_DIR);

	    vector<string> scripts = dir.entries(plugins_filter_entries);
	    std::sort(scripts.begin(), scripts.end());
	    for (const string& script : scripts)
	    {
		string cmd_line = dir.fullname(script);
		for (const string& arg : args)
		    cmd_line += " " + quote(arg);
		SystemCmd cmd(cmd_line);
	    }
	}
	catch (const Exception& e)
	{
	    SN_CAUGHT(e);
	}
    }


    void
    Hooks::create_config(const string& subvolume, const Filesystem* filesystem)
    {
	grub(subvolume, filesystem, "--enable");
	run_scripts({ "create-config", subvolume, filesystem->fstype() });
    }


    void
    Hooks::delete_config(const string& subvolume, const Filesystem* filesystem)
    {
	grub(subvolume, filesystem, "--disable");
	run_scripts({ "delete-config", subvolume, filesystem->fstype() });
    }


    void
    Hooks::create_snapshot(const string& subvolume, const Filesystem* filesystem, const Snapshot& snapshot)
    {
	grub(subvolume, filesystem, "--refresh");
	run_scripts({ "create-snapshot", subvolume, filesystem->fstype(), std::to_string(snapshot.getNum()) });
    }


    void
    Hooks::modify_snapshot(const string& subvolume, const Filesystem* filesystem, const Snapshot& snapshot)
    {
	grub(subvolume, filesystem, "--refresh");
	run_scripts({ "modify-snapshot", subvolume, filesystem->fstype(), std::to_string(snapshot.getNum()) });
    }


    void
    Hooks::delete_snapshot(const string& subvolume, const Filesystem* filesystem, const Snapshot& snapshot)
    {
	grub(subvolume, filesystem, "--refresh");
	run_scripts({ "delete-snapshot", subvolume, filesystem->fstype(), std::to_string(snapshot.getNum()) });
    }


    void
    Hooks::set_default_snapshot(const string& subvolume, const Filesystem* filesystem, unsigned int num)
    {
	run_scripts({ "set-default-snapshot", subvolume, filesystem->fstype(), std::to_string(num) });
    }


    void
    Hooks::grub(const string& subvolume, const Filesystem* filesystem, const char* option)
    {
#ifdef ENABLE_ROLLBACK

#define GRUB_SCRIPT "/usr/lib/snapper/plugins/grub"

	if (subvolume == "/" && filesystem->fstype() == "btrfs" && access(GRUB_SCRIPT, X_OK) == 0)
	{
	    SystemCmd cmd(string(GRUB_SCRIPT) + " " + option);
	}
#endif
    }


    void
    Hooks::rollback(const string& old_root, const string& new_root)
    {
#ifdef ENABLE_ROLLBACK

#define ROLLBACK_SCRIPT "/usr/lib/snapper/plugins/rollback"

	// Fate#319108
	if (access(ROLLBACK_SCRIPT, X_OK) == 0)
	{
	    SystemCmd cmd(string(ROLLBACK_SCRIPT) + " " + old_root + " " + new_root);
	}
#endif
    }


    void
    Hooks::rollback(const string& subvolume, const Filesystem* filesystem, unsigned int old_num, unsigned int new_num)
    {
	run_scripts({ "rollback", subvolume, filesystem->fstype(), std::to_string(old_num), std::to_string(new_num) });
    }

}
