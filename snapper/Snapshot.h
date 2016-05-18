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


#ifndef SNAPPER_SNAPSHOT_H
#define SNAPPER_SNAPSHOT_H


#include <time.h>
#include <string>
#include <list>
#include <map>

#include "snapper/Exception.h"


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

	bool isReadOnly() const;

	void setDefault() const;

	void mountFilesystemSnapshot(bool user_request) const;
	void umountFilesystemSnapshot(bool user_request) const;
	void handleUmountFilesystemSnapshot() const;

	friend std::ostream& operator<<(std::ostream& s, const Snapshot& snapshot);

    private:

	const Snapper* snapper;

	SnapshotType type;

	unsigned int num;

	time_t date;

	uid_t uid;

	unsigned int pre_num;	// valid only for type=POST

	string description;	// likely empty for type=POST
	string cleanup;
	map<string, string> userdata;

	mutable bool mount_checked;
	mutable bool mount_user_request;
	mutable unsigned int mount_use_count;

	void writeInfo() const;

	void createFilesystemSnapshot(unsigned int num_parent, bool read_only) const;
	void createFilesystemSnapshotOfDefault(bool read_only) const;
	void deleteFilesystemSnapshot() const;

    };


    inline bool operator<(const Snapshot& a, const Snapshot& b)
    {
	return a.getNum() < b.getNum();
    }


    // Snapshot Modify Data
    class SMD
    {
    public:

	SMD() : description(), cleanup(), userdata({}) {}

	string description;
	string cleanup;
	map<string, string> userdata;

    };

    // Snapshot Create Data
    class SCD : public SMD
    {
    public:

	SCD() : SMD(), read_only(true), uid(0) {}

	bool read_only;
	uid_t uid;

    };


    class Snapshots
    {
    public:

	friend class Snapper;

	Snapshots(const Snapper* snapper) : snapper(snapper) {}

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

    private:

	void initialize();

	void read();

	void check() const;

	void checkUserdata(const map<string, string>& userdata) const;

	iterator createSingleSnapshot(const SCD& scd);
	iterator createSingleSnapshot(const_iterator parent, const SCD& scd);
	iterator createSingleSnapshotOfDefault(const SCD& scd);
	iterator createPreSnapshot(const SCD& scd);
	iterator createPostSnapshot(const_iterator pre, const SCD& scd);

	iterator createHelper(Snapshot& snapshot, const_iterator parent, bool read_only);

	void modifySnapshot(iterator snapshot, const SMD& smd);

	void deleteSnapshot(iterator snapshot);

	unsigned int nextNumber();

	const Snapper* snapper;

	list<Snapshot> entries;

    };

}


#endif
