/*
 * Copyright (c) [2004-2015] Novell, Inc.
 * Copyright (c) [2017-2025] SUSE LLC
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


#include <sys/utsname.h>
#include <regex>

#include "snapper/SnapperTmpl.h"
#include "snapper/SystemCmd.h"
#include "snapper/SnapperDefines.h"
#include "snapper/Exception.h"
#include "snapper/LoggerImpl.h"
#include "CmdBtrfs.h"


namespace snapper
{

    using namespace std;


    CmdBtrfsSubvolumeShow::CmdBtrfsSubvolumeShow(const string& btrfs_bin, const Shell& shell, const string& mount_point)
    {
	SystemCmd::Args cmd_args = { btrfs_bin, "subvolume", "show", "--", mount_point };
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


    void
    CmdBtrfsVersion::query_version()
    {
	if (did_set_version)
	    return;

	SystemCmd::Args cmd_args = { btrfs_bin, "--version" };
	SystemCmd cmd(shellify(shell, cmd_args));

	if (cmd.retcode() != 0)
	{
	    y2err("command '" << cmd.cmd() << "' failed: " << cmd.retcode());
	    for (const string& tmp : cmd.get_stdout())
		y2err(tmp);
	    for (const string& tmp : cmd.get_stderr())
		y2err(tmp);

	    SN_THROW(Exception("'btrfs --version' failed"));
	}

	parse_version(cmd.get_stdout()[0]);
    }


    void
    CmdBtrfsVersion::parse_version(const string& version)
    {
	// example versions: "5.14 " (yes, with a trailing space), "6.0", "6.0.2"
	const regex version_rx("btrfs-progs v([0-9]+)\\.([0-9]+)(\\.([0-9]+))?( )*", regex::extended);

	smatch match;

	if (!regex_match(version, match, version_rx))
	    SN_THROW(Exception("failed to parse btrfs version '" + version + "'"));

	major = stoi(match[1]);
	minor = stoi(match[2]);
	patchlevel = match[4].length() == 0 ? 0 : stoi(match[4]);

	y2mil("major:" << major << " minor:" << minor << " patchlevel:" << patchlevel);

	did_set_version = true;
    }


    int
    CmdBtrfsVersion::supported_proto()
    {
	query_version();

	if (major >= 6)
	    return 2;

	return 1;
    }


    int
    Uname::supported_proto()
    {
	const regex release_rx("^([0-9]+)\\.([0-9]+).*", regex::extended);

	struct utsname buffer;

	if (uname(&buffer) < 0)
	    SN_THROW(Exception("syscall uname failed"));

	cmatch match;

	if (!regex_match(buffer.release, match, release_rx))
	    SN_THROW(Exception("failed to parse uname release '" + string(buffer.release) + "'"));

	int major = stoi(match[1]);
	int minor = stoi(match[2]);

	y2mil("major:" << major << " minor:" << minor);

	if (major >= 6)
	    return 2;

	return 1;
    }

}
