/*
 * Copyright (c) [2011-2012] Novell, Inc.
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


#ifndef SNAPPER_COMPARISON_H
#define SNAPPER_COMPARISON_H


#include "snapper/Comparison.h"
#include "snapper/Snapshot.h"
#include "snapper/File.h"


namespace snapper
{

    class Comparison
    {
    public:

	Comparison(const Snapper* snapper, Snapshots::const_iterator snapshot1,
		   Snapshots::const_iterator snapshot2, bool delay = false);

	const Snapper* getSnapper() const { return snapper; }

	Snapshots::const_iterator getSnapshot1() const { return snapshot1; }
	Snapshots::const_iterator getSnapshot2() const { return snapshot2; }

	void initialize();
	bool isInitialized() const { return initialized; }

	Files& getFiles();
	const Files& getFiles() const;

	UndoStatistic getUndoStatistic() const;

	bool doUndo();

    private:

	const Snapper* snapper;

	Snapshots::const_iterator snapshot1;
	Snapshots::const_iterator snapshot2;

	bool initialized;

	Files files;

    };

}


#endif
