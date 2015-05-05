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


#ifndef SNAPPER_BTRFS_UTILS_H
#define SNAPPER_BTRFS_UTILS_H


#include <string>


namespace snapper
{
    using std::string;


    namespace BtrfsUtils
    {

	typedef uint64_t subvolid_t;

	typedef uint64_t qgroup_t;
	const qgroup_t no_qgroup = 0;

	bool is_subvolume(const struct stat& stat);

	bool is_subvolume_read_only(int fd);

	bool does_subvolume_exist(int fd, subvolid_t id);

	void create_subvolume(int fddst, const string& name);
	void create_snapshot(int fd, int fddst, const string& name, bool read_only,
			     qgroup_t qgroup);
	void delete_subvolume(int fd, const string& name);

	void set_default_id(int fd, subvolid_t id);
	subvolid_t get_default_id(int fd);

	string get_subvolume(int fd, subvolid_t id);
	subvolid_t get_id(int fd);

	qgroup_t make_qgroup(uint64_t level, subvolid_t id);
	qgroup_t make_qgroup(const string& str);

	void sync(int fd);

    }

}


#endif
