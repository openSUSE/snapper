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


// TODO obsolete, remove with next ABI break


#ifndef SNAPPER_HOOKS_H
#define SNAPPER_HOOKS_H


#include "snapper/Snapper.h"
#include "snapper/Filesystem.h"


namespace snapper
{
    using namespace std;


    class Hooks
    {
    public:

	enum class Stage { PRE_ACTION, POST_ACTION };

	static void create_config(Stage stage, const string& subvolume, const Filesystem* filesystem) SN_DEPRECATED;
	static void delete_config(Stage stage, const string& subvolume, const Filesystem* filesystem) SN_DEPRECATED;

	static void create_snapshot(Stage stage, const string& subvolume, const Filesystem* filesystem,
				    const Snapshot& snapshot) SN_DEPRECATED;
	static void modify_snapshot(Stage stage, const string& subvolume, const Filesystem* filesystem,
				    const Snapshot& snapshot) SN_DEPRECATED;
	static void delete_snapshot(Stage stage, const string& subvolume, const Filesystem* filesystem,
				    const Snapshot& snapshot) SN_DEPRECATED;

	static void set_default_snapshot(Stage stage, const string& subvolume, const Filesystem* filesystem,
					 unsigned int num) SN_DEPRECATED;

	static void rollback(const string& old_root, const string& new_root) SN_DEPRECATED;

	static void rollback(Stage stage, const string& subvolume, const Filesystem* filesystem, unsigned int old_num,
			     unsigned int new_num) SN_DEPRECATED;

    private:

	static void grub(const string& subvolume, const Filesystem* filesystem,
			 const char* option);

	static void run_scripts(const vector<string>& args);

    };

}


#endif
