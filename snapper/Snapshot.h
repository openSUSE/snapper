/*
 * Copyright (c) 2011 Novell, Inc.
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


#ifndef SNAPSHOT_H
#define SNAPSHOT_H


#include <time.h>

#include <string>
#include <list>


namespace snapper
{
    using std::string;
    using std::list;


    class Snapper;


    enum SnapshotType { SINGLE, PRE, POST };


    class Snapshot
    {
    public:

	friend class Snapshots;

	Snapshot(const Snapper* snapper)
	    : snapper(snapper), type(SINGLE), num(0), date((time_t)(-1)), pre_num(0) {}

	SnapshotType getType() const { return type; }

	unsigned int getNum() const { return num; }
	bool isCurrent() const { return num == 0; }

	time_t getDate() const { return date; }

	void setDescription(const string& description);
	string getDescription() const { return description; }

	unsigned int getPreNum() const { return pre_num; }

	void setCleanup(const string& cleanup);
	string getCleanup() const { return cleanup; }

	string baseDir() const;
	string snapshotDir() const;

	friend std::ostream& operator<<(std::ostream& s, const Snapshot& snapshot);

    private:

	const Snapper* snapper;

	SnapshotType type;

	unsigned int num;

	time_t date;

	string description;	// likely empty for type=POST

	unsigned int pre_num;	// valid only for type=POST

	string cleanup;

	bool writeInfo() const;
	bool createFilesystemSnapshot() const;
	bool deleteFilesystemSnapshot() const;

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

	iterator begin() { return entries.begin(); }
	const_iterator begin() const { return entries.begin(); }

	iterator end() { return entries.end(); }
	const_iterator end() const { return entries.end(); }

	iterator find(unsigned int num);
	const_iterator find(unsigned int num) const;

	const_iterator findPost(const_iterator pre) const;

	const_iterator getSnapshotCurrent() const { return entries.begin(); }

    private:

	void initialize();

	void read();

	void check() const;

	iterator createSingleSnapshot(string description);
	iterator createPreSnapshot(string description);
	iterator createPostSnapshot(const_iterator pre);

	void deleteSnapshot(iterator snapshot);

	unsigned int nextNumber();

	const Snapper* snapper;

	list<Snapshot> entries;

    };

}


#endif
