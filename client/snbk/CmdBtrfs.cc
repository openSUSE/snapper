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


#include <cstring>
#include <regex>
#include <boost/algorithm/string.hpp>

#include "snapper/SnapperTmpl.h"
#include "snapper/SystemCmd.h"
#include "snapper/SnapperDefines.h"
#include "snapper/Exception.h"
#include "snapper/Log.h"
#include "CmdBtrfs.h"


namespace snapper
{

    using namespace std;


    CmdBtrfsSubvolumeList::CmdBtrfsSubvolumeList(const string& btrfs_bin, const Shell& shell, const string& mount_point)
    {
	SystemCmd::Args cmd_args = { btrfs_bin, "subvolume", "list", "-a", "-puqR", mount_point };
	SystemCmd cmd(shellify(shell, cmd_args));

	if (cmd.retcode() != 0)
	{
	    y2err("command '" << cmd.cmd() << "' failed: " << cmd.retcode());
	    for (const string& tmp : cmd.get_stdout())
		y2err(tmp);
	    for (const string& tmp : cmd.get_stderr())
		y2err(tmp);

	    SN_THROW(Exception("'btrfs subvolume list' failed"));
	}

	parse(cmd.get_stdout());
    }


    void
    CmdBtrfsSubvolumeList::parse(const vector<string>& lines)
    {
	for (const string& line : lines)
	{
	    Entry entry;

	    string::size_type pos1 = line.find("ID ");
	    if (pos1 == string::npos)
		SN_THROW(Exception("could not find 'id' in 'btrfs subvolume list' output"));
	    line.substr(pos1 + strlen("ID ")) >> entry.id;

	    string::size_type pos2 = line.find(" parent ");
	    if (pos2 == string::npos)
		SN_THROW(Exception("could not find 'parent' in 'btrfs subvolume list' output"));
	    line.substr(pos2 + strlen(" parent ")) >> entry.parent_id;

	    // Subvolume can already be deleted, in which case parent is "0"
	    // (and path "DELETED"). That is a temporary state.
	    if (entry.parent_id == 0)
		continue;

	    string::size_type pos3 = line.find(" path ");
	    if (pos3 == string::npos)
		SN_THROW(Exception("could not find 'path' in 'btrfs subvolume list' output"));
	    entry.path = line.substr(pos3 + strlen(" path "));
	    if (boost::starts_with(entry.path, "<FS_TREE>/"))
		entry.path.erase(0, strlen("<FS_TREE>/"));

	    string::size_type pos4 = line.find(" uuid ");
	    if (pos4 == string::npos)
		SN_THROW(Exception("could not find 'uuid' in 'btrfs subvolume list' output"));
	    line.substr(pos4 + strlen(" uuid ")) >> entry.uuid;

	    string::size_type pos5 = line.find(" parent_uuid ");
	    if (pos5 == string::npos)
		SN_THROW(Exception("could not find 'parent_uuid' in 'btrfs subvolume list' output"));
	    line.substr(pos5 + strlen(" parent_uuid ")) >> entry.parent_uuid;
	    if (entry.parent_uuid == "-")
		entry.parent_uuid = "";

	    string::size_type pos6 = line.find(" received_uuid ");
	    if (pos6 == string::npos)
		SN_THROW(Exception("could not find 'received_uuid' in 'btrfs subvolume list' output"));
	    line.substr(pos6 + strlen(" received_uuid ")) >> entry.received_uuid;
	    if (entry.received_uuid == "-")
		entry.received_uuid = "";

	    data.push_back(entry);
	}

	// No way to get read-only flag. Only showing read-only snapshots does not help.

	y2mil(*this);
    }


    CmdBtrfsSubvolumeList::const_iterator
    CmdBtrfsSubvolumeList::find_entry_by_path(const string& path) const
    {
	for (const_iterator it = data.begin(); it != data.end(); ++it)
	{
	    if (it->path == path)
		return it;
	}

	return data.end();
    }


    std::ostream&
    operator<<(std::ostream& s, const CmdBtrfsSubvolumeList& cmd_btrfs_subvolume_list)
    {
	for (const CmdBtrfsSubvolumeList::Entry& entry : cmd_btrfs_subvolume_list)
	    s << entry;

	return s;
    }


    std::ostream&
    operator<<(std::ostream& s, const CmdBtrfsSubvolumeList::Entry& entry)
    {
	s << "id:" << entry.id << " parent-id:" << entry.parent_id
	  << " path:" << entry.path << " uuid:" << entry.uuid;

	if (!entry.parent_uuid.empty())
	    s << " parent-uuid:" << entry.parent_uuid;

	if (!entry.received_uuid.empty())
	    s << " received-uuid:" << entry.received_uuid;

	s  << '\n';

	return s;
    }


    CmdBtrfsSubvolumeShow::CmdBtrfsSubvolumeShow(const string& btrfs_bin, const Shell& shell, const string& mount_point)
    {
	SystemCmd::Args cmd_args = { btrfs_bin, "subvolume", "show", mount_point };
	SystemCmd cmd(shellify(shell, cmd_args));

	if (cmd.retcode() != 0)
	{
	    y2err("command '" << cmd.cmd() << "' failed: " << cmd.retcode());
	    for (const string& tmp : cmd.get_stdout())
		y2err(tmp);
	    for (const string& tmp : cmd.get_stderr())
		y2err(tmp);

	    SN_THROW(Exception("'btrfs subvolume show' failed"));
	}

	parse(cmd.get_stdout());
    }


    void
    CmdBtrfsSubvolumeShow::parse(const vector<string>& lines)
    {
	static const regex uuid_regex("[ \t]*UUID:[ \t]*(" UUID_REGEX "|-)[ \t]*", regex::extended);
	static const regex parent_uuid_regex("[ \t]*Parent UUID:[ \t]*(" UUID_REGEX "|-)[ \t]*", regex::extended);
	static const regex received_uuid_regex("[ \t]*Received UUID:[ \t]*(" UUID_REGEX "|-)[ \t]*", regex::extended);
	static const regex creation_time_regex("[ \t]*Creation time:[ \t]*([-+0-9: ]+)[ \t]*", regex::extended);
	static const regex flags_regex("[ \t]*Flags:[ \t]*(" "readonly" "|-)[ \t]*", regex::extended);

	smatch match;

	for (const string& line : lines)
	{
	    if (regex_match(line, match, uuid_regex))
		uuid = match[1];

	    if (regex_match(line, match, parent_uuid_regex))
	    {
		if (match[1] != "-")
		    parent_uuid = match[1];
	    }

	    if (regex_match(line, match, received_uuid_regex))
	    {
		if (match[1] != "-")
		    received_uuid = match[1];
	    }

	    if (regex_match(line, match, creation_time_regex))
		creation_time = match[1];

	    // so far readonly is the only flag so unclear whether several flags will be
	    // separated by space or comma

	    if (regex_match(line, match, flags_regex))
		read_only = match[1] == "readonly";
	}

	if (uuid.empty())
	    SN_THROW(Exception("could not find 'uuid' in 'btrfs subvolume show' output"));

	if (uuid == "-")
	{
	    // If the btrfs was created with older kernels (whatever that means) (tested
	    // with SLES 11 SP3), the top-level subvolume does not have a UUID. Other
	    // subvolumes do have a UUID. In that case also all subvolumes are listed
	    // wrongly as snapshots of the top-level subvolume (by 'btrfs subvolume
	    // show'). The relationship between other subvolumes/snapshots seems to be
	    // fine.

	    y2mil("could not find 'uuid' in 'btrfs subvolume show' output - happens if "
		  "btrfs was created with an old kernel");

	    uuid = "";
	}

	y2mil(*this);
    }


    std::ostream&
    operator<<(std::ostream& s, const CmdBtrfsSubvolumeShow& cmd_btrfs_subvolume_show)
    {
	s << "uuid:" << cmd_btrfs_subvolume_show.uuid;

	if (!cmd_btrfs_subvolume_show.parent_uuid.empty())
	    s << " parent-uuid:" << cmd_btrfs_subvolume_show.parent_uuid;

	if (!cmd_btrfs_subvolume_show.received_uuid.empty())
	    s << " received-uuid:" << cmd_btrfs_subvolume_show.received_uuid;

	if (!cmd_btrfs_subvolume_show.creation_time.empty())
	    s << " creation-time:" << cmd_btrfs_subvolume_show.creation_time;

	if (cmd_btrfs_subvolume_show.read_only)
	    s << " read-only";

	s  << '\n';

	return s;
    }

}
