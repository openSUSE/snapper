/*
 * Copyright (c) [2024-2026] SUSE LLC
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
#include "CmdLs.h"
#include "BackupConfig.h"
#include "TheBigThing.h"


namespace snapper
{

    namespace
    {
	/** A base class for constructing a node from an iterator. */
	class BaseNode : public TreeView::ProxyNode
	{
	public:

	    BaseNode(const TheBigThings::const_iterator& it) : it(it) {}
	    unsigned int get_number() const override { return it->num; }
	    bool is_virtual() const override { return false; }

	    bool is_valid() const override
	    {
		return (it->source_state == TheBigThing::SourceState::READ_ONLY &&
		        it->target_state == TheBigThing::TargetState::VALID);
	    }

	    const TheBigThings::const_iterator it;
	};

	/**
	 * Specialized class for source nodes.
	 * (Used when sending snapshots from source to target.)
	 */
	class SourceNode : public BaseNode
	{
	public:

	    SourceNode(const TheBigThings::const_iterator& it) : BaseNode(it) {}
	    const string& get_uuid() const override { return it->source_uuid; }
	    const string& get_parent_uuid() const override
	    {
		return it->source_parent_uuid;
	    }
	};

	/**
	 * Specialized class for target nodes.
	 * (Used when sending snapshots from target to source.)
	 */
	class TargetNode : public BaseNode
	{
	public:

	    TargetNode(const TheBigThings::const_iterator& it) : BaseNode(it) {}
	    const string& get_uuid() const override { return it->target_uuid; }
	    const string& get_parent_uuid() const override
	    {
		return it->target_parent_uuid;
	    }
	};


	template <typename NodeType>
	vector<shared_ptr<TreeView::ProxyNode>>
	make_nodes(const TheBigThings& the_big_things)
	{
	    vector<shared_ptr<TreeView::ProxyNode>> nodes;
	    for (TheBigThings::const_iterator it = the_big_things.begin();
		 it != the_big_things.end(); ++it)
	    {
		nodes.push_back(std::make_shared<NodeType>(it));
	    }

	    return nodes;
	}

    }


    using namespace std;


    const vector<string> EnumInfo<TheBigThing::SourceState>::names({
	"missing", "read-only", "read-write"
    });


    const vector<string> EnumInfo<TheBigThing::TargetState>::names({
	"missing", "valid", "invalid"
    });


    void TheBigThing::copy(const BackupConfig& backup_config,
                           TheBigThings& the_big_things,
                           const pair<CopySpec, CopySpec>& copy_specs)
    {
	// Unpack copy specification
	const CopySpec& src_spec = copy_specs.first;
	const CopySpec& dst_spec = copy_specs.second;

	// Create the snapshot directory on the destination.
	SystemCmd::Args cmd1_args = { dst_spec.mkdir_bin, "--parents", "--",
	                              dst_spec.snapshot_dir };
	SystemCmd cmd1(shellify(dst_spec.shell, cmd1_args));
	if (cmd1.retcode() != 0)
	{
	    y2err("command '" << cmd1.cmd() << "' failed: " << cmd1.retcode());
	    for (const string& tmp : cmd1.get_stdout())
		y2err(tmp);
	    for (const string& tmp : cmd1.get_stderr())
		y2err(tmp);

	    SN_THROW(Exception(_("'mkdir' failed.")));
	}

	// Copy info.xml to the destination.
	switch (backup_config.target_mode)
	{
	    case BackupConfig::TargetMode::LOCAL:
	    {
		SystemCmd::Args cmd2_args = { CP_BIN, "--",
		                              src_spec.snapshot_dir + "/info.xml",
		                              dst_spec.snapshot_dir + "/" };
		SystemCmd cmd2(shellify(src_spec.shell, cmd2_args));
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
		cmd2_args << "--"
		          << src_spec.remote_host + src_spec.snapshot_dir + "/info.xml"
		          << dst_spec.remote_host + dst_spec.snapshot_dir + "/";

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
	};

	// Copy snapshot to the destination.
	const int proto = the_big_things.proto();

	SystemCmd::Args cmd3a_args = { src_spec.btrfs_bin, "send" };
	if (proto >= 2)
	    cmd3a_args << "--proto" << to_string(proto);
	if (backup_config.send_compressed_data && proto >= 2)
	    cmd3a_args << "--compressed-data";
	cmd3a_args << backup_config.send_options;

	if (!src_spec.parent_subvol_path.empty())
	{
	    cmd3a_args << "-p" << src_spec.parent_subvol_path;
	}
	cmd3a_args << "--" << src_spec.snapshot_dir + "/" SNAPSHOT_NAME;

	SystemCmd::Args cmd3b_args = { dst_spec.btrfs_bin, "receive" };
	cmd3b_args << backup_config.receive_options;
	cmd3b_args << "--" << dst_spec.snapshot_dir;

	y2deb("source: " << cmd3a_args.get_values());
	y2deb("destination: " << cmd3b_args.get_values());

	SystemCmd cmd3(
	    shellify_pipe(src_spec.shell, cmd3a_args, dst_spec.shell, cmd3b_args));
	if (cmd3.retcode() != 0)
	{
	    y2err("command '" << cmd3.cmd() << "' failed: " << cmd3.retcode());
	    for (const string& tmp : cmd3.get_stdout())
		y2err(tmp);
	    for (const string& tmp : cmd3.get_stderr())
		y2err(tmp);

	    SN_THROW(Exception(_("'btrfs send | btrfs receive' failed.")));
	}
    }

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

	// Copy the snapshot from the source to the target
	copy(backup_config, the_big_things,
	     make_copy_specs(backup_config, the_big_things, CopyMode::SOURCE_TO_TARGET));

	target_state = TargetState::VALID;
    }


    void
    TheBigThing::restore(const BackupConfig& backup_config, TheBigThings& the_big_things,
			 bool quiet)
    {
	if (!quiet)
	    cout << sformat(_("Restoring snapshot %d."), num) << '\n';

	if (target_state != TargetState::VALID)
	    SN_THROW(Exception(_("Snapshot not on target.")));

	if (source_state != SourceState::MISSING)
	    SN_THROW(Exception(_("Snapshot already on source.")));

	// Copy the snapshot from the target to the source
	copy(backup_config, the_big_things,
	     make_copy_specs(backup_config, the_big_things, CopyMode::TARGET_TO_SOURCE));

	source_state = SourceState::READ_ONLY;
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

    const pair<TheBigThing::CopySpec, TheBigThing::CopySpec>
    TheBigThing::make_copy_specs(const BackupConfig& backup_config,
                                 const TheBigThings& the_big_things,
                                 CopyMode copy_mode) const
    {
	CopySpec spec_source; // Copy specification for the snapshot on the source.
	spec_source.shell = backup_config.get_source_shell();
	spec_source.mkdir_bin = MKDIR_BIN;
	spec_source.btrfs_bin = BTRFS_BIN;
	spec_source.snapshot_dir = source_snapshot_dir(backup_config);

	CopySpec spec_target; // Copy specification for the snapshot on the target.
	spec_target.shell = backup_config.get_target_shell();
	spec_target.mkdir_bin = backup_config.target_mkdir_bin;
	spec_target.btrfs_bin = backup_config.target_btrfs_bin;
	spec_target.snapshot_dir = target_snapshot_dir(backup_config);

	// Resolve the remote host when using SSH push.
	if (backup_config.target_mode == BackupConfig::TargetMode::SSH_PUSH)
	{
	    spec_target.remote_host =
	        (backup_config.ssh_user.empty() ? "" : backup_config.ssh_user + "@") +
	        backup_config.ssh_host + ":";
	}

	switch (copy_mode)
	{
	    case CopyMode::SOURCE_TO_TARGET:

		// Resolve Btrfs send parent.
		if (auto parent =
		        the_big_things.source_tree.find_nearest_valid_node(source_uuid))
		{
		    const BaseNode* parent_node =
		        static_cast<const BaseNode*>(parent->node);
		    spec_source.parent_subvol_path =
		        parent_node->it->source_snapshot_dir(backup_config) +
		        "/" SNAPSHOT_NAME;
		}

		// Return copy specification.
		return {
		    spec_source, // Snapshot on the source as the copy source.,
		    spec_target  // Backup target as the copy destination.
		};

	    case CopyMode::TARGET_TO_SOURCE:

		// Resolve Btrfs send parent
		if (auto parent =
		        the_big_things.target_tree.find_nearest_valid_node(target_uuid))
		{
		    const BaseNode* parent_node =
		        static_cast<const BaseNode*>(parent->node);
		    spec_target.parent_subvol_path =
		        parent_node->it->target_snapshot_dir(backup_config) +
		        "/" SNAPSHOT_NAME;
		}

		// Return copy specification
		return {
		    spec_target, // Snapshot on the target as the copy source.
		    spec_source  // Backup source as the copy destination.
		};
	};

	SN_THROW(Exception("invalid copy mode"));
	__builtin_unreachable();
    }

    string TheBigThing::source_snapshot_dir(const BackupConfig& backup_config) const
    {
	return backup_config.source_path + "/" SNAPSHOTS_NAME "/" + to_string(num);
    }

    string TheBigThing::target_snapshot_dir(const BackupConfig& backup_config) const
    {
	return backup_config.target_path + "/" + to_string(num);
    }


    int
    TheBigThings::proto()
    {
	return std::min({
	    Uname::supported_proto(),
	    source_btrfs_version.supported_proto(),
	    target_btrfs_version.supported_proto()
	});
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

	// Construct tree for finding Btrfs send parent
	source_tree = TreeView(make_nodes<SourceNode>(*this));
	target_tree = TreeView(make_nodes<TargetNode>(*this));
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

	/* Using 'btrfs subvolume list' instead of 'ls' is much more complicated. */

	CmdLs cmd_ls(backup_config.target_ls_bin, shell_target, backup_config.target_path);

	if (verbose)
	    cout << _("Probing extra information for target snapshots.") << endl;

	static const regex num_regex("[0-9]+", regex::extended);

	for (const string& num_string : cmd_ls)
	{
	    if (!regex_match(num_string, num_regex))
	    {
		string error = sformat(_("Invalid subvolume path '%s' on target."), num_string.c_str());
		SN_THROW(Exception(error));
	    }

	    unsigned int num = stoi(num_string);
	    vector<TheBigThing>::iterator it = find(num);

	    // Query additional information (receive-uuid, read-only) from btrfs.

	    try
	    {
		CmdBtrfsSubvolumeShow extra(backup_config.target_btrfs_bin, shell_target, backup_config.target_path +
					    "/" + num_string + "/" SNAPSHOT_NAME);

		bool is_read_only = extra.is_read_only();
		if (!is_read_only)
		{
		    y2deb(num << " not read-only, maybe interrupted transfer");
		}

		if (it != end())
		{
		    // Wrong receive-uuid can happen when a snapshots is transferred, then removed
		    // and a new one with the same number is generated.

		    // When a snapshot is restored using btrfs send and receive the received
		    // uuid of the source is identical to the received uuid of the target -
		    // not the uuid of the target. In that case the target is also valid.

		    bool correct_uuid = false;

		    if (!extra.get_received_uuid().empty())
		    {
			if (it->source_uuid == extra.get_received_uuid())
			    correct_uuid = true;
			else if (it->source_received_uuid == extra.get_received_uuid())
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

		it->target_uuid = extra.get_uuid();
		it->target_parent_uuid = extra.get_parent_uuid();
		it->target_received_uuid = extra.get_received_uuid();
		it->target_creation_time = extra.get_creation_time();
	    }
	    catch (const Exception& e)
	    {
		SN_CAUGHT(e);

		// target_state is plain and simply missing or the snapshot is not even in
		// the list.
	    }
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
    TheBigThings::restore(const BackupConfig& backup_config, bool quiet, bool verbose)
    {
	for (TheBigThing& the_big_thing : the_big_things)
	{
	    if (the_big_thing.target_state == TheBigThing::TargetState::VALID &&
		the_big_thing.source_state == TheBigThing::SourceState::MISSING)
	    {
		the_big_thing.restore(backup_config, *this, quiet);
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

}
