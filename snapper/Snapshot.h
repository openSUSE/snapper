/*
 * Copyright (c) [2011-2013] Novell, Inc.
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


    struct CreateSnapshotFailedException : public SnapperException
    {
	explicit CreateSnapshotFailedException() throw() {}
	virtual const char* what() const throw() { return "create snapshot failed"; }
    };

    struct DeleteSnapshotFailedException : public SnapperException
    {
	explicit DeleteSnapshotFailedException() throw() {}
	virtual const char* what() const throw() { return "delete snapshot failed"; }
    };


    struct IsSnapshotMountedFailedException : public SnapperException
    {
	explicit IsSnapshotMountedFailedException() throw() {}
	virtual const char* what() const throw() { return "is snapshot mounted failed"; }
    };

    struct MountSnapshotFailedException : public SnapperException
    {
	explicit MountSnapshotFailedException() throw() {}
	virtual const char* what() const throw() { return "mount snapshot failed"; }
    };

    struct UmountSnapshotFailedException : public SnapperException
    {
	explicit UmountSnapshotFailedException() throw() {}
	virtual const char* what() const throw() { return "umount snapshot failed"; }
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

	void setUid(uid_t uid) __attribute__ ((deprecated));
	uid_t getUid() const { return uid; }

	unsigned int getPreNum() const { return pre_num; }

	void setDescription(const string& description) __attribute__ ((deprecated));
	string getDescription() const { return description; }

	void setCleanup(const string& cleanup) __attribute__ ((deprecated));
	string getCleanup() const { return cleanup; }

	void setUserdata(const map<string, string>& userdata) __attribute__ ((deprecated));
	map<string, string> getUserdata() const { return userdata; }

	void flushInfo() __attribute__ ((deprecated));

	string snapshotDir() const;

	SDir openInfoDir() const;
	SDir openSnapshotDir() const;

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

	bool info_modified;

	mutable bool mount_checked;
	mutable bool mount_user_request;
	mutable unsigned int mount_use_count;

	void writeInfo() const;

	void createFilesystemSnapshot() const;
	void deleteFilesystemSnapshot() const;

    };


    inline bool operator<(const Snapshot& a, const Snapshot& b)
    {
	return a.getNum() < b.getNum();
    }


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

	iterator createSingleSnapshot(string description) __attribute__ ((deprecated));
	iterator createPreSnapshot(string description) __attribute__ ((deprecated));
	iterator createPostSnapshot(string description, const_iterator pre) __attribute__ ((deprecated));

	iterator createSingleSnapshot(uid_t uid, const string& description, const string& cleanup,
				      const map<string, string>& userdata);
	iterator createPreSnapshot(uid_t uid, const string& description, const string& cleanup,
				   const map<string, string>& userdata);
	iterator createPostSnapshot(const_iterator pre, uid_t uid, const string& description,
				    const string& cleanup, const map<string, string>& userdata);

	iterator createHelper(Snapshot& snapshot);

	void modifySnapshot(iterator snapshot, const string& description, const string& cleanup,
			    const map<string, string>& userdata);

	void deleteSnapshot(iterator snapshot);

	unsigned int nextNumber();

	const Snapper* snapper;

	list<Snapshot> entries;

    };

}


#endif
