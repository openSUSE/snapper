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


#ifndef SNAPPER_H
#define SNAPPER_H


#include "snapper/Snapshot.h"
#include "snapper/File.h"


namespace snapper
{

    struct CompareCallback
    {
	CompareCallback() {}
	virtual ~CompareCallback() {}

	virtual void start() {}
	virtual void stop() {}
    };


    class Snapper
    {
    public:

	Snapper();
	~Snapper();

	const Snapshots& getSnapshots() const { return snapshots; }

	Snapshots::iterator createSingleSnapshot(string description);
	Snapshots::iterator createPreSnapshot(string description);
	Snapshots::iterator createPostSnapshot(Snapshots::const_iterator pre);

	void startBackgroundComparsion(Snapshots::const_iterator snapshot1,
				       Snapshots::const_iterator snapshot2);

	bool setComparisonNums(Snapshots::const_iterator snapshot1,
			       Snapshots::const_iterator snapshot2);

	Snapshots::const_iterator getSnapshot1() const { return snapshot1; }
	Snapshots::const_iterator getSnapshot2() const { return snapshot2; }

	Files& getFiles() { return files; }
	const Files& getFiles() const { return files; }

	bool doRollback();

	void setCompareCallback(CompareCallback* p) { compare_callback = p; }
	CompareCallback* getCompareCallback() const { return compare_callback; }

    private:

	Snapshots snapshots;

	Snapshots::const_iterator snapshot1;
	Snapshots::const_iterator snapshot2;

	Files files;

	CompareCallback* compare_callback;

    };

};


#endif
