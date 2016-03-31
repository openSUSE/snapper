/*
 * Copyright (c) [2011-2015] Novell, Inc.
 * Copyright (c) 2016 SUSE LLC
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


#include <stdint.h>
#include <string>
#include <vector>


namespace snapper
{
    using std::string;
    using std::vector;


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

	void quota_enable(int fd);
	void quota_disable(int fd);

	void quota_rescan(int fd);

	qgroup_t calc_qgroup(uint64_t level, subvolid_t id);
	uint64_t get_level(qgroup_t qgroup);
	uint64_t get_subvolid(qgroup_t qgroup);
	qgroup_t parse_qgroup(const string& str);
	string format_qgroup(qgroup_t qgroup);

	void qgroup_create(int fd, qgroup_t qgroup);
	void qgroup_destroy(int fd, qgroup_t qgroup);

	void qgroup_assign(int fd, qgroup_t src, qgroup_t dst);
	void qgroup_remove(int fd, qgroup_t src, qgroup_t dst);

	qgroup_t qgroup_find_free(int fd, uint64_t level);

	vector<qgroup_t> qgroup_query_children(int fd, qgroup_t parent);

	struct QGroupUsage
	{
	    QGroupUsage() : referenced(0), referenced_compressed(0), exclusive(0),
			    exclusive_compressed(0) {}

	    uint64_t referenced;
	    uint64_t referenced_compressed;
	    uint64_t exclusive;
	    uint64_t exclusive_compressed;
	};

	QGroupUsage qgroup_query_usage(int fd, qgroup_t qgroup);

	void sync(int fd);

    }

}


#endif
