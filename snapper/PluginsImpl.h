/*
 * Copyright (c) [2011-2015] Novell, Inc.
 * Copyright (c) [2022-2023] SUSE LLC
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


#ifndef SNAPPER_PLUGINS_IMPL_H
#define SNAPPER_PLUGINS_IMPL_H


#include "snapper/Plugins.h"


namespace snapper
{
    using std::string;


    class Filesystem;
    class Snapshot;


    namespace Plugins
    {

	enum class Stage { PRE_ACTION, POST_ACTION };

	void create_config(Stage stage, const string& subvolume, const Filesystem* filesystem, Report& report);
	void delete_config(Stage stage, const string& subvolume, const Filesystem* filesystem, Report& report);

	void create_snapshot(Stage stage, const string& subvolume, const Filesystem* filesystem,
			     const Snapshot& snapshot, Report& report);
	void modify_snapshot(Stage stage, const string& subvolume, const Filesystem* filesystem,
			     const Snapshot& snapshot, Report& report);
	void delete_snapshot(Stage stage, const string& subvolume, const Filesystem* filesystem,
			     const Snapshot& snapshot, Report& report);

	void set_default_snapshot(Stage stage, const string& subvolume, const Filesystem* filesystem,
				  unsigned int num, Report& report);

	void rollback(const string& old_root, const string& new_root, Report& report);

	void rollback(Stage stage, const string& subvolume, const Filesystem* filesystem, unsigned int old_num,
		      unsigned int new_num, Report& report);

    };

}


#endif
