/*
 * Copyright (c) [2011-2015] Novell, Inc.
 * Copyright (c) [2016-2023] SUSE LLC
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


#ifndef SNAPPER_SNAPSHOT_H
#define SNAPPER_SNAPSHOT_H


#include <ctime>
#include <sys/types.h>
#include <cstdint>
#include <string>
#include <list>
#include <map>

#include "snapper/Exception.h"
#include "snapper/Plugins.h"


namespace snapper
{
    using std::string;
    using std::list;
    using std::map;


    class Snapper;
    class SDir;


    enum SnapshotType { SINGLE, PRE, POST };


    struct CreateSnapshotFailedException : public Exception
    {
	explicit CreateSnapshotFailedException() : Exception("create snapshot failed") {}
    };

    struct DeleteSnapshotFailedException : public Exception
    {
	explicit DeleteSnapshotFailedException() : Exception("delete snapshot failed") {}
    };


    struct IsSnapshotMountedFailedException : public Exception
    {
	explicit IsSnapshotMountedFailedException() : Exception("is snapshot mounted failed") {}
    };

    struct MountSnapshotFailedException : public Exception
    {
	explicit MountSnapshotFailedException() : Exception("mount snapshot failed") {}
    };

    struct UmountSnapshotFailedException : public Exception
    {
	explicit UmountSnapshotFailedException() : Exception("umount snapshot failed") {}
    };


    class Snapshot
    {
    public:

	friend class Snapshots;

	Snapshot(const Snapper* snapper, SnapshotType type, unsigned int num, time_t date);
	~Snapshot();

	SnapshotType getType() const { return type; }

	unsigned int getNum() const { return num; }
	bool isCurrent() const { return num == 0; }

	time_t getDate() const { return date; }

	uid_t getUid() const { return uid; }

	unsigned int getPreNum() const { return pre_num; }

	const string& getDescription() const { return description; }
	const string& getCleanup() const { return cleanup; }
	const map<string, string>& getUserdata() const { return userdata; }

	string snapshotDir() const;

	SDir openInfoDir() const;
	SDir openSnapshotDir() const;

	/**
	 * Determine iff snapshot is read-only (only for btrfs).
	 */
	bool isReadOnly() const;

	/**
	 * Set snapshot read-only or read-write (only for btrfs).
	 */
	void setReadOnly(bool read_only);

	/**
	 * Determine iff snapshot is default (will be activated on next boot time).
	 */
	bool isDefault() const;

	/**
	 * Change default snapshot (will be activated on next boot time).
	 */
	void setDefault(Plugins::Report& report) const;

	/**
	 * Determine iff snapshot is active (activated on last boot time).
	 */
	bool isActive() const;

	/**
	 * Query the disk space used by the snapshot. So far only
	 * available for btrfs and only if quota is enabled. In that
	 * case the used space is the exclusive space of the btrfs
	 * qgroup of the snapshot.
	 */
	uint64_t getUsedSpace() const;

	void mountFilesystemSnapshot(bool user_request) const;
	void umountFilesystemSnapshot(bool user_request) const;
	void handleUmountFilesystemSnapshot() const;

	friend std::ostream& operator<<(std::ostream& s, const Snapshot& snapshot);

    private:

	const Snapper* snapper;

	SnapshotType type;

	unsigned int num;

	time_t date;

	uid_t uid = 0;

	bool read_only = true;

	unsigned int pre_num = 0;	// valid only for type=POST

	string description;	// likely empty for type=POST
	string cleanup;
	map<string, string> userdata;

	mutable bool mount_checked = false;
	mutable bool mount_user_request = false;
	mutable unsigned int mount_use_count = 0;

	void writeInfo() const;

	void createFilesystemSnapshot(unsigned int num_parent, bool read_only, bool empty) const;
	void createFilesystemSnapshotOfDefault(bool read_only) const;
	void deleteFilesystemSnapshot() const;

	void deleteFilelists() const;

    };


    inline bool operator<(const Snapshot& a, const Snapshot& b)
    {
	return a.getNum() < b.getNum();
    }


    // Snapshot Modify Data
    class SMD
    {
    public:

	string description;
	string cleanup;
	map<string, string> userdata;

    };

    // Snapshot Create Data
    class SCD : public SMD
    {
    public:

	bool read_only = true;

	/**
	 * Create an empty snapshot. For btrfs this creates a subvolume
	 * instead of a snapshot, for other filesystem types ignored.
	 */
	bool empty = false;

	uid_t uid = 0;

    };


    class Snapshots
    {
    public:

	friend class Snapper;

	Snapshots(const Snapper* snapper);
	~Snapshots();

	typedef list<Snapshot>::iterator iterator;
	typedef list<Snapshot>::const_iterator const_iterator;
	typedef list<Snapshot>::size_type size_type;

	iterator begin() { return entries.begin(); }
	const_iterator begin() const { return entries.begin(); }

	iterator end() { return entries.end(); }
	const_iterator end() const { return entries.end(); }

	size_type size() const { return entries.size(); }

	iterator find(unsigned int num);
	const_iterator find(unsigned int num) const;

	iterator findPre(const_iterator post);
	const_iterator findPre(const_iterator post) const;

	iterator findPost(const_iterator pre);
	const_iterator findPost(const_iterator pre) const;

	const_iterator getSnapshotCurrent() const { return entries.begin(); }

	/**
	 * Query the default snapshot. Only supported for btrfs.
	 *
	 * For btrfs the default btrfs snapshot will be mounted next
	 * time the filesystem is mounted (unless overridden).
	 *
	 * The default btrfs snapshot may not be controlled by snapper
	 * in which this function returns end().
	 */
	iterator getDefault();

	const_iterator getDefault() const;

	/**
	 * Query the active snapshot. Only supported for btrfs.
	 *
	 * For btrfs the active btrfs snapshot is the one currently mounted.
	 *
	 * The active btrfs snapshot may not be controlled by snapper
	 * in which this function returns end().
	 */
	const_iterator getActive() const;

    private:

	void initialize();

	void read();

	void check() const;

	void checkUserdata(const map<string, string>& userdata) const;

	iterator createSingleSnapshot(const SCD& scd, Plugins::Report& report);
	iterator createSingleSnapshot(const_iterator parent, const SCD& scd, Plugins::Report& report);
	iterator createSingleSnapshotOfDefault(const SCD& scd, Plugins::Report& report);
	iterator createPreSnapshot(const SCD& scd, Plugins::Report& report);
	iterator createPostSnapshot(const_iterator pre, const SCD& scd, Plugins::Report& report);

	iterator createHelper(Snapshot& snapshot, const_iterator parent, bool empty, Plugins::Report& report);

	void modifySnapshot(iterator snapshot, const SMD& smd, Plugins::Report& report);

	void deleteSnapshot(iterator snapshot, Plugins::Report& report);

	unsigned int nextNumber() const;

	const Snapper* snapper;

	list<Snapshot> entries;

    };

}


#endif
