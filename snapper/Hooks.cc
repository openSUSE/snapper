/*
 * Copyright (c) [2011-2015] Novell, Inc.
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

#include <string.h>

#include <boost/algorithm/string/join.hpp>

#include "snapper/FileUtils.h"
#include "snapper/Hooks.h"
#include "snapper/SystemCmd.h"
#include "snapper/Log.h"


namespace snapper
{
    using namespace std;

    static bool
    _plugins_filter_entries(unsigned char type, const char* name)
    {
	// must start with digit
	if (*name >= '0' && *name <= '9')
	    return true;
	return false;
    }

    void
    Hooks::run_scripts(const list<string>& args)
    {
	    SDir dir("/usr/lib/snapper/plugins");

	    vector<string> scripts = dir.entries(_plugins_filter_entries);
	    std::sort(scripts.begin(), scripts.end());
	    for (const string& script : scripts)
	    {
		string cmdln = dir.fullname(script);
		for (const string& arg : args) {
		    cmdln += " " + quote(arg);
		}
		SystemCmd cmd(cmdln);
	    }
    }

    void
    Hooks::create_config(const string& subvolume, const Filesystem* filesystem)
    {
	grub(subvolume, filesystem, "--enable");
    }


    void
    Hooks::delete_config(const string& subvolume, const Filesystem* filesystem)
    {
	grub(subvolume, filesystem, "--disable");
    }


    void
    Hooks::create_snapshot(const string& subvolume, const Filesystem* filesystem, const Snapshot& snapshot)
    {
	grub(subvolume, filesystem, "--refresh");
	run_scripts(std::list<string>({"create-snapshot", subvolume, std::to_string(snapshot.getNum())}));
    }


    void
    Hooks::modify_snapshot(const string& subvolume, const Filesystem* filesystem, const Snapshot& snapshot)
    {
	grub(subvolume, filesystem, "--refresh");
	run_scripts(std::list<string>({"modify-snapshot", subvolume, std::to_string(snapshot.getNum())}));
    }


    void
    Hooks::delete_snapshot(const string& subvolume, const Filesystem* filesystem, const Snapshot& snapshot)
    {
	grub(subvolume, filesystem, "--refresh");
	run_scripts(std::list<string>({"delete-snapshot", subvolume, std::to_string(snapshot.getNum())}));
    }

    void
    Hooks::set_default_snapshot(const string& subvolume, const Filesystem* filesystem, unsigned int num)
    {
	run_scripts(std::list<string>({"set-default-snapshot", subvolume, std::to_string(num)}));
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

}
