/*
 * Copyright (c) [2004-2015] Novell, Inc.
 * Copyright (c) [2017-2024] SUSE LLC
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


#ifndef SNAPPER_CMD_BTRFS_H
#define SNAPPER_CMD_BTRFS_H


#include "Shell.h"


namespace snapper
{

    using std::string;
    using std::vector;


    /**
     * Class to probe for btrfs subvolumes: Call "btrfs subvolume list
     * <mount-point>".
     */
    class CmdBtrfsSubvolumeList
    {
    public:

	static const long top_level_id = 5;
	static const long unknown_id = -1;

	CmdBtrfsSubvolumeList(const Shell& shell, const string& mount_point);

	/**
	 * Entry for every subvolume (unfortunately except the top-level).
	 *
	 * Caution: parent_id and parent_uuid are something completely
	 * different - not just different ways to specify the
	 * "parent".
	 */
	struct Entry
	{
	    long id = unknown_id;
	    long parent_id = unknown_id;
	    string path;
	    string uuid;
	    string parent_uuid;
	    string received_uuid;
	};

	typedef vector<Entry>::value_type value_type;
	typedef vector<Entry>::const_iterator const_iterator;

	const_iterator begin() const { return data.begin(); }
	const_iterator end() const { return data.end(); }

	const_iterator find_entry_by_path(const string& path) const;

	friend std::ostream& operator<<(std::ostream& s, const CmdBtrfsSubvolumeList& cmd_btrfs_subvolume_list);
	friend std::ostream& operator<<(std::ostream& s, const Entry& entry);

    private:

	void parse(const vector<string>& lines);

	vector<Entry> data;

    };


    /**
     * Class to probe for btrfs subvolume information: Call "btrfs subvolume
     * show <mount-point>".
     */
    class CmdBtrfsSubvolumeShow
    {
    public:

	CmdBtrfsSubvolumeShow(const Shell& shell, const string& mount_point);

	const string& get_uuid() const { return uuid; }
	const string& get_parent_uuid() const { return parent_uuid; }
	const string& get_received_uuid() const { return received_uuid; }
	const string& get_creation_time() const { return creation_time; }
	bool is_read_only() const { return read_only; }

	friend std::ostream& operator<<(std::ostream& s, const CmdBtrfsSubvolumeShow&
					cmd_btrfs_subvolume_show);

    private:

	void parse(const vector<string>& lines);

	string uuid;
	string parent_uuid;
	string received_uuid;
	string creation_time;	// TODO should be time_t
	bool read_only = false;

    };

}

#endif
