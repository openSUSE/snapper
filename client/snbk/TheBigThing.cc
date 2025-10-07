/*
 * Copyright (c) [2024-2025] SUSE LLC
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

#include <iostream>
#include <regex>
#include <boost/algorithm/string/predicate.hpp>

#include "snapper/SystemCmd.h"
#include "snapper/SnapperDefines.h"
#include "snapper/SnapperTmpl.h"

#include "../proxy/proxy.h"
#include "../utils/text.h"

#include "CmdBtrfs.h"
#include "CmdFindmnt.h"
#include "CmdRealpath.h"
#include "BackupConfig.h"
#include "TheBigThing.h"


namespace snapper
{

    using namespace std;


    const vector<string> EnumInfo<TheBigThing::SourceState>::names({
	"missing", "read-only", "read-write"
    });


    const vector<string> EnumInfo<TheBigThing::TargetState>::names({
	"missing", "valid", "invalid"
    });


    void
    TheBigThing::transfer(const BackupConfig& backup_config, TheBigThings& the_big_things,
			  bool quiet)
    {
	if (!quiet)
	    cout << sformat(_("Transferring snapshot %d."), num) << '\n';

	if (source_state == SourceState::MISSING)
	    SN_THROW(Exception(_("Snapshot not on source.")));

	if (target_state != TargetState::MISSING)
	    SN_THROW(Exception(_("Snapshot already on target.")));

	const string num_string = to_string(num);

	// Create directory on target.

	SystemCmd::Args cmd1_args = { backup_config.target_mkdir_bin, "--", backup_config.target_path + "/" +
	    num_string };
	SystemCmd cmd1(shellify(backup_config.get_target_shell(), cmd1_args));
	if (cmd1.retcode() != 0)
	{
	    y2err("command '" << cmd1.cmd() << "' failed: " << cmd1.retcode());
	    for (const string& tmp : cmd1.get_stdout())
		y2err(tmp);
	    for (const string& tmp : cmd1.get_stderr())
		y2err(tmp);

	    SN_THROW(Exception(_("'mkdir' failed.")));
	}

	// Copy info.xml to target.

	switch (backup_config.target_mode)
	{
	    case BackupConfig::TargetMode::LOCAL:
	    {
		SystemCmd::Args cmd2_args = { CP_BIN, "--", backup_config.source_path + "/" SNAPSHOTS_NAME "/" +
		    num_string + "/info.xml", backup_config.target_path + "/" + num_string + "/" };
		SystemCmd cmd2(shellify(backup_config.get_target_shell(), cmd2_args));
		if (cmd2.retcode() != 0)
		{
		    y2err("command '" << cmd2.cmd() << "' failed: " << cmd2.retcode());
		    for (const string& tmp : cmd2.get_stdout())
			y2err(tmp);
		    for (const string& tmp : cmd2.get_stderr())
			y2err(tmp);

		    SN_THROW(Exception(_("'cp info.xml' failed.")));
		}
	    }
	    break;

	    case BackupConfig::TargetMode::SSH_PUSH:
	    {
		SystemCmd::Args cmd2_args = { SCP_BIN };
		if (backup_config.ssh_port != 0)
		    cmd2_args << "-P" << to_string(backup_config.ssh_port);
		if (!backup_config.ssh_identity.empty())
		    cmd2_args << "-i" << backup_config.ssh_identity;
		cmd2_args << "--" << backup_config.source_path + "/" SNAPSHOTS_NAME "/" + num_string +
		    "/info.xml" << (backup_config.ssh_user.empty() ? "" : backup_config.ssh_user + "@") +
		    backup_config.ssh_host + ":" + backup_config.target_path + "/" + num_string + "/";

		SystemCmd cmd2(cmd2_args);
		if (cmd2.retcode() != 0)
		{
		    y2err("command '" << cmd2.cmd() << "' failed: " << cmd2.retcode());
		    for (const string& tmp : cmd2.get_stdout())
			y2err(tmp);
		    for (const string& tmp : cmd2.get_stderr())
			y2err(tmp);

		    SN_THROW(Exception(_("'scp info.xml' failed.")));
		}
	    }
	    break;
	}

	// Copy snapshot to target.

	const int proto = std::min({
	    Uname::supported_proto(),
	    the_big_things.source_btrfs_version.supported_proto(),
	    the_big_things.target_btrfs_version.supported_proto()
	});

	TheBigThings::const_iterator it1 = the_big_things.find_send_parent(*this);

	SystemCmd::Args cmd3a_args = { BTRFS_BIN, "send" };
	if (proto >= 2)
	    cmd3a_args << "--proto" << to_string(proto);
	if (backup_config.send_compressed_data && proto >= 2)
	    cmd3a_args << "--compressed-data";
	cmd3a_args << backup_config.send_options;

	if (it1 != the_big_things.end())
	    cmd3a_args << "-p" << backup_config.source_path + "/" SNAPSHOTS_NAME "/" +
		to_string(it1->num) + "/" SNAPSHOT_NAME;
	cmd3a_args << "--" << backup_config.source_path + "/" SNAPSHOTS_NAME "/" + num_string + "/" SNAPSHOT_NAME;

	SystemCmd::Args cmd3b_args = { backup_config.target_btrfs_bin, "receive" };
	cmd3b_args << backup_config.receive_options;
	cmd3b_args << "--" << backup_config.target_path + "/" + num_string;

	y2deb("source: " << cmd3a_args.get_values());
	y2deb("target: " << cmd3b_args.get_values());

	SystemCmd cmd3(shellify_pipe(cmd3a_args, backup_config.get_target_shell(), cmd3b_args));
	if (cmd3.retcode() != 0)
	{
	    y2err("command '" << cmd3.cmd() << "' failed: " << cmd3.retcode());
	    for (const string& tmp : cmd3.get_stdout())
		y2err(tmp);
	    for (const string& tmp : cmd3.get_stderr())
		y2err(tmp);

	    SN_THROW(Exception(_("'btrfs send | btrfs receive' failed.")));
	}

	target_state = TargetState::VALID;
    }


    void
    TheBigThing::remove(const BackupConfig& backup_config, bool quiet)
    {
	if (!quiet)
	    cout << sformat(_("Deleting snapshot %d."), num) << '\n';

	if (target_state == TargetState::MISSING)
	    SN_THROW(Exception(_("Snapshot not on target.")));

	const string num_string = to_string(num);

	// Delete snapshot on target.

	SystemCmd::Args cmd1_args = { backup_config.target_btrfs_bin, "subvolume", "delete", "--",
	    backup_config.target_path + "/" + num_string + "/" SNAPSHOT_NAME };
	SystemCmd cmd1(shellify(backup_config.get_target_shell(), cmd1_args));
	if (cmd1.retcode() != 0)
	{
	    y2err("command '" << cmd1.cmd() << "' failed: " << cmd1.retcode());
	    for (const string& tmp : cmd1.get_stdout())
		y2err(tmp);
	    for (const string& tmp : cmd1.get_stderr())
		y2err(tmp);

	    SN_THROW(Exception(_("'btrfs subvolume delete' failed.")));
	}

	// Remove info.xml on target.

	SystemCmd::Args cmd2_args = { backup_config.target_rm_bin, "--", backup_config.target_path + "/" + num_string +
	    "/info.xml" };
	SystemCmd cmd2(shellify(backup_config.get_target_shell(), cmd2_args));
	if (cmd2.retcode() != 0)
	{
	    y2err("command '" << cmd2.cmd() << "' failed: " << cmd2.retcode());
	    for (const string& tmp : cmd2.get_stdout())
		y2err(tmp);
	    for (const string& tmp : cmd2.get_stderr())
		y2err(tmp);

	    SN_THROW(Exception(_("'rm info.xml' failed.")));
	}

	// Remove directory on target.

	SystemCmd::Args cmd3_args = { backup_config.target_rmdir_bin, "--", backup_config.target_path + "/" +
	    num_string };
	SystemCmd cmd3(shellify(backup_config.get_target_shell(), cmd3_args));
	if (cmd3.retcode() != 0)
	{
	    y2err("command '" << cmd3.cmd() << "' failed: " << cmd3.retcode());
	    for (const string& tmp : cmd3.get_stdout())
		y2err(tmp);
	    for (const string& tmp : cmd3.get_stderr())
		y2err(tmp);

	    SN_THROW(Exception(_("'rmdir' failed.")));
	}

	target_state = TargetState::MISSING;
    }


    bool
    operator<(const TheBigThing& lhs, const TheBigThing& rhs)
    {
	return lhs.num < rhs.num;
    }


    TheBigThings::TheBigThings(const BackupConfig& backup_config, ProxySnappers* snappers, bool verbose)
	: source_btrfs_version(BTRFS_BIN, backup_config.get_source_shell()),
	  target_btrfs_version(backup_config.target_btrfs_bin, backup_config.get_target_shell()),
	  snapper(snappers->getSnapper(backup_config.config)), locker(snapper)
    {
	if (backup_config.source_path != snapper->getConfig().getSubvolume())
	{
	    string error = sformat(_("Path mismatch between source-path of backup-config and subvolume of "
				     "snapper config ('%s' vs. '%s')."), backup_config.source_path.c_str(),
				   snapper->getConfig().getSubvolume().c_str());
	    SN_THROW(Exception(error));
	}

	probe_source(backup_config, verbose);
	probe_target(backup_config, verbose);

	sort(the_big_things.begin(), the_big_things.end());
    }


    void
    TheBigThings::probe_source(const BackupConfig& backup_config, bool verbose)
    {
	const Shell shell_source = backup_config.get_source_shell();

	// Query snapshots on source from snapperd.

	if (verbose)
	    cout << _("Probing source snapshots.") << endl;

	const ProxySnapshots& source_snapshots = snapper->getSnapshots();

	if (verbose)
	    cout << _("Probing extra information for source snapshots.") << endl;

	for (const ProxySnapshot& source_snapshot : source_snapshots)
	{
	    unsigned int num = source_snapshot.getNum();
	    if (num == 0)
		continue;

	    // Query additional information (uuids, read-only) from btrfs.

	    CmdBtrfsSubvolumeShow extra(BTRFS_BIN, shell_source, backup_config.source_path + "/" SNAPSHOTS_NAME "/" +
					to_string(num) + "/" SNAPSHOT_NAME);

	    TheBigThing the_big_thing(num);
	    the_big_thing.date = source_snapshot.getDate();
	    the_big_thing.source_state = extra.is_read_only() ? TheBigThing::SourceState::READ_ONLY :
		TheBigThing::SourceState::READ_WRITE;
	    the_big_thing.source_uuid = extra.get_uuid();
	    the_big_thing.source_parent_uuid = extra.get_parent_uuid();
	    the_big_thing.source_received_uuid = extra.get_received_uuid();
	    the_big_thing.source_creation_time = extra.get_creation_time();

	    the_big_things.push_back(the_big_thing);
	}
    }


    void
    TheBigThings::probe_target(const BackupConfig& backup_config, bool verbose)
    {
	const Shell shell_target = backup_config.get_target_shell();

	// Query snapshots on target from btrfs.

	if (verbose)
	    cout << _("Probing target snapshots.") << endl;

	// In case the target-path is a symbolic link (or includes things like "/../") we
	// need a lookup for the realpath first.

	CmdRealpath cmd_realpath(backup_config.target_realpath_bin, shell_target, backup_config.target_path);
	const string target_path = cmd_realpath.get_realpath();

	CmdFindmnt cmd_findmnt(backup_config.target_findmnt_bin, shell_target, target_path);
	const string mount_point = cmd_findmnt.get_target();

	if (target_path.size() < mount_point.size())
	    SN_THROW(Exception("unsupported target-path setup"));

	if (!boost::starts_with(target_path, mount_point))
	    SN_THROW(Exception("unsupported target-path setup"));

	CmdBtrfsSubvolumeList target_snapshots(backup_config.target_btrfs_bin, shell_target, mount_point);

	string start;
	if (target_path != mount_point)
	    start = target_path.substr(mount_point.size() + 1) + "/";

	static const regex subvol_regex("\\[(.*?)\\]");
	smatch subvol;
	if (regex_search(cmd_findmnt.get_source(), subvol, subvol_regex))
		start = subvol[1].str().substr(1) + "/" + start;

	if (verbose)
	    cout << _("Probing extra information for target snapshots.") << endl;

	static const regex num_regex("([0-9]+)/snapshot", regex::extended);

	for (const CmdBtrfsSubvolumeList::Entry& target_snapshot : target_snapshots)
	{
	    if (!boost::starts_with(target_snapshot.path, start))
		continue;

	    string path = target_snapshot.path.substr(start.size());

	    smatch match;

	    if (!regex_match(path, match, num_regex))
	    {
		string error = sformat(_("Invalid subvolume path '%s' on target."), path.c_str());
		SN_THROW(Exception(error));
	    }

	    unsigned int num = stoi(match[1]);

	    // Query additional information (receive-uuid, read-only) from btrfs.

	    CmdBtrfsSubvolumeShow y(backup_config.target_btrfs_bin, shell_target, target_path + "/" + path);

	    bool is_read_only = y.is_read_only();
	    if (!is_read_only)
	    {
		y2deb(num << " not read-only, maybe interrupted transfer");
	    }

	    vector<TheBigThing>::iterator it = find(num);
	    if (it != end())
	    {
		// Wrong receive-uuid can happen when a snapshots is transferred, then removed
		// and a new one with the same number is generated.

		// When a snapshot is restored using btrfs send and receive the received
		// uuid of the source is identical to the received uuid of the target -
		// not the uuid of the target. In that case the target is also valid.

		bool correct_uuid = false;

		if (!y.get_received_uuid().empty())
		{
		    if (it->source_uuid == y.get_received_uuid())
			correct_uuid = true;
		    else if (it->source_received_uuid == y.get_received_uuid())
			correct_uuid = true;

		    if (!correct_uuid)
		    {
			y2deb(num << " wrong uuid, maybe snapshot number reuse");
		    }
		}

		if (correct_uuid && is_read_only)
		    it->target_state = TheBigThing::TargetState::VALID;
		else
		    it->target_state = TheBigThing::TargetState::INVALID;
	    }
	    else
	    {
		TheBigThing the_big_thing(num);

		// Cannot check received-uuid so assume valid.

		if (is_read_only)
		    the_big_thing.target_state = TheBigThing::TargetState::VALID;
		else
		    the_big_thing.target_state = TheBigThing::TargetState::INVALID;

		it = the_big_things.insert(the_big_things.end(), the_big_thing);
	    }

	    it->target_uuid = y.get_uuid();
	    it->target_parent_uuid = y.get_parent_uuid();
	    it->target_received_uuid = y.get_received_uuid();
	    it->target_creation_time = y.get_creation_time();
	}
    }


    void
    TheBigThings::transfer(const BackupConfig& backup_config, bool quiet, bool verbose)
    {
	for (TheBigThing& the_big_thing : the_big_things)
	{
	    if (the_big_thing.source_state == TheBigThing::SourceState::READ_ONLY)
	    {
		if (the_big_thing.target_state == TheBigThing::TargetState::INVALID)
		{
		    the_big_thing.remove(backup_config, quiet);
		}

		if (the_big_thing.target_state == TheBigThing::TargetState::MISSING)
		{
		    the_big_thing.transfer(backup_config, *this, quiet);
		}
	    }
	}
    }


    void
    TheBigThings::remove(const BackupConfig& backup_config, bool quiet, bool verbose)
    {
	for (TheBigThing& the_big_thing : the_big_things)
	{
	    if (the_big_thing.target_state == TheBigThing::TargetState::INVALID)
	    {
		the_big_thing.remove(backup_config, quiet);
	    }

	    if (the_big_thing.source_state == TheBigThing::SourceState::MISSING &&
		the_big_thing.target_state != TheBigThing::TargetState::MISSING)
	    {
		the_big_thing.remove(backup_config, quiet);
	    }
	}
    }


    vector<TheBigThing>::iterator
    TheBigThings::find(unsigned int num)
    {
	return find_if(begin(), end(), [num](const TheBigThing& the_big_thing) {
	    return the_big_thing.num == num;
	});
    }


    TheBigThings::const_iterator
    TheBigThings::find_send_parent(const TheBigThing& the_big_thing) const
    {
	typedef vector<TheBigThing>::const_reverse_iterator const_reverse_iterator;

	// Find the direct parent or a previous snapshots with the same parent UUID (more
	// a sibling).

	for (const_reverse_iterator it1 = the_big_things.rbegin(); it1 != the_big_things.rend(); ++it1)
	{
	    if (it1->num >= the_big_thing.num)
		continue;

	    if (it1->source_state != TheBigThing::SourceState::READ_ONLY ||
		it1->target_state != TheBigThing::TargetState::VALID)
		continue;

	    if (it1->source_uuid == the_big_thing.source_parent_uuid)
		// base() is a bit surprising, compensate that
		return (it1 + 1).base();

	    if (it1->source_parent_uuid == the_big_thing.source_parent_uuid)
		return (it1 + 1).base();
	}

	// Find the direct parent of the direct parent. This case can happen after a
	// rollback.

	for (const_reverse_iterator it1 = the_big_things.rbegin(); it1 != the_big_things.rend(); ++it1)
	{
	    if (it1->num >= the_big_thing.num)
		continue;

	    // Here the direct parent itself might be read-write and thus not available on
	    // the target at all.

	    if (it1->source_uuid == the_big_thing.source_parent_uuid)
	    {
		for (const_reverse_iterator it2 = the_big_things.rbegin(); it2 != the_big_things.rend(); ++it2)
		{
		    if (it2->num >= it1->num)
			continue;

		    if (it2->source_state != TheBigThing::SourceState::READ_ONLY ||
			it2->target_state != TheBigThing::TargetState::VALID)
			continue;

		    if (it2->source_uuid == it1->source_parent_uuid)
			return (it2 + 1).base();
		}
	    }
	}

	return end();
    }

}
