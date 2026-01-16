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

#ifndef SNAPPER_THE_BIG_THING_H
#define SNAPPER_THE_BIG_THING_H


#include <string>
#include <vector>

#include "../proxy/proxy.h"
#include "../proxy/locker.h"

#include "CmdBtrfs.h"
#include "TreeView.h"


namespace snapper
{

    using std::string;
    using std::vector;


    class BackupConfig;
    class TheBigThings;


    class TheBigThing
    {
    public:

	// snapshots on target are always read-only if valid

	enum class SourceState { MISSING, READ_ONLY, READ_WRITE };
	enum class TargetState { MISSING, VALID, INVALID };

	/** Snapshot copy mode. */
	enum class CopyMode
	{
	    /** Copy the snapshot from the source to the target (i.e., transfer). */
	    SOURCE_TO_TARGET,

	    /** Copy the snapshots from the target to the source (i.e., restore). */
	    TARGET_TO_SOURCE
	};

	/**
	 * Specification for the copy source or the copy destination.
	 * The `source` here is different from the backup `source`. A copy source can be
	 * either a snapshot on the backup source or the backup target.
	 */
	struct CopySpec
	{
	    Shell shell;
	    string mkdir_bin;
	    string btrfs_bin;
	    string remote_host;
	    string snapshot_dir;
	    string parent_subvol_path;
	};

	TheBigThing(unsigned int num) : num(num) {}

	void copy(const BackupConfig& backup_config, TheBigThings& the_big_things,
	          const CopySpec& src_spec, const CopySpec& dst_spec);
	void transfer(const BackupConfig& backup_config, TheBigThings& the_big_things, bool quiet);
	void restore(const BackupConfig& backup_config, TheBigThings& the_big_things, bool quiet);

	void remove(const BackupConfig& backup_config, bool quiet);

	/**
	 * Create specifications (assigned by reference) for the copy source and the copy
	 * destination according to the specified `copy_mode`.
	 */
	void make_copy_spec(const BackupConfig& backup_config,
	                    const TheBigThings& the_big_things, CopySpec& src_spec,
	                    CopySpec& dst_spec, CopyMode copy_mode);

	string source_snapshot_dir(const BackupConfig& backup_config) const;
	string target_snapshot_dir(const BackupConfig& backup_config) const;

	unsigned int num;
	time_t date = 0;	// as reported by snapper

	SourceState source_state = SourceState::MISSING;
	TargetState target_state = TargetState::MISSING;

	string source_uuid;
	string source_parent_uuid;
	string source_received_uuid;
	string source_creation_time;

	string target_uuid;
	string target_parent_uuid;
	string target_received_uuid;
	string target_creation_time;

    };


    template <> struct EnumInfo<TheBigThing::SourceState> { static const vector<string> names; };

    template <> struct EnumInfo<TheBigThing::TargetState> { static const vector<string> names; };


    class TheBigThings
    {
    public:

	/**
	 * Queries the snapshots on the source and target. Also gets a ProxySnapper and
	 * locks it.
	 */
	TheBigThings(const BackupConfig& backup_config, ProxySnappers* snappers, bool verbose);

	void transfer(const BackupConfig& backup_config, bool quiet, bool verbose);
	void restore(const BackupConfig& backup_config, bool quiet, bool verbose);

	void remove(const BackupConfig& backup_config, bool quiet, bool verbose);

	typedef vector<TheBigThing>::iterator iterator;
	typedef vector<TheBigThing>::const_iterator const_iterator;

	iterator begin() { return the_big_things.begin(); }
	const_iterator begin() const { return the_big_things.begin(); }

	iterator end() { return the_big_things.end(); }
	const_iterator end() const { return the_big_things.end(); }

	iterator find(unsigned int num);

	CmdBtrfsVersion source_btrfs_version;
	CmdBtrfsVersion target_btrfs_version;

	int proto();

	/**
	 * Helper objects for finding a suitable Btrfs send parent when transferring and
	 * restoring snapshots.
	 */
	TreeView source_tree;
	TreeView target_tree;

    private:

	const ProxySnapper* snapper;
	const Locker locker;

	vector<TheBigThing> the_big_things;

	void probe_source(const BackupConfig& backup_config, bool verbose);
	void probe_target(const BackupConfig& backup_config, bool verbose);

    };

}

#endif
