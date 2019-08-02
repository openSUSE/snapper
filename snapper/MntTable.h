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

#include <libmount/libmount.h>

#include <string>
#include <stdexcept>

#include "snapper/AppUtil.h"


namespace snapper
{

    using namespace std;


    class MntTable
    {

    public:

	MntTable(const string& root_prefix)
	    : root_prefix(root_prefix), table(mnt_new_table())
	{
	    if (!table)
		throw runtime_error("mnt_new_table failed");

	    mnt_table_enable_comments(table, 1);
	}

	~MntTable()
	{
	    mnt_reset_table(table);
	}

	void parse_fstab()
	{
	    if (mnt_table_parse_fstab(table, target_fstab().c_str()) != 0)
		throw runtime_error("mnt_table_parse_fstab failed");
	}

	void parse_mtab()
	{
	    if (mnt_table_parse_mtab(table, NULL))
		throw runtime_error("mnt_table_parse_mtab failed");
	}

	void set_cache(libmnt_cache* cache)
	{
	    if (cache == NULL || mnt_table_set_cache(table, cache) != 0)
		throw runtime_error("Setting the file system cache failed");
	}

	void replace_file()
	{
	    if (mnt_table_replace_file(table, target_fstab().c_str()) != 0)
		throw runtime_error("mnt_table_replace_file failed");
	}

	struct libmnt_fs* find_target(const string& path, int directon)
	{
	    return mnt_table_find_target(table, path.c_str(), directon);
	}

	struct libmnt_fs* find_target_up(const string& path, int direction);

	void add_fs(struct libmnt_fs* fs)
	{
	    if (mnt_table_add_fs(table, fs) != 0)
		throw runtime_error("mnt_table_add_fs failed");
	}

	void remove_fs(struct libmnt_fs* fs)
	{
	    if (mnt_table_remove_fs(table, fs) != 0)
		throw runtime_error("mnt_table_remove_fs failed");
	}

    private:

	string target_fstab() const
	{
	    return prepend_root_prefix(root_prefix, "/etc/fstab");
	}

	const string root_prefix;

	struct libmnt_table* table;

    };

}
