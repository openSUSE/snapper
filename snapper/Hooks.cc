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

#include "snapper/Hooks.h"
#include "snapper/SystemCmd.h"


namespace snapper
{
    using namespace std;


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
    Hooks::create_snapshot(const string& subvolume, const Filesystem* filesystem)
    {
	grub(subvolume, filesystem, "--refresh");
    }


    void
    Hooks::modify_snapshot(const string& subvolume, const Filesystem* filesystem)
    {
	grub(subvolume, filesystem, "--refresh");
    }


    void
    Hooks::delete_snapshot(const string& subvolume, const Filesystem* filesystem)
    {
	grub(subvolume, filesystem, "--refresh");
    }


    void
    Hooks::grub(const string& subvolume, const Filesystem* filesystem, const char* option)
    {
#ifdef ENABLE_ROLLBACK
	if (subvolume == "/" && filesystem->fstype() == "btrfs" &&
	    access("/usr/lib/snapper/plugins/grub", X_OK) == 0)
	{
	    SystemCmd cmd(string("/usr/lib/snapper/plugins/grub ") + option);
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
