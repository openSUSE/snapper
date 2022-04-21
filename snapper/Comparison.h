/*
 * Copyright (c) [2011-2012] Novell, Inc.
 * Copyright (c) 2022 SUSE LLC
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


#include "snapper/Snapshot.h"
#include "snapper/Snapper.h"
#include "snapper/File.h"


namespace snapper
{

    class Comparison
    {
    public:

	/**
	 * Create a comparison.
	 *
	 * The mount parameter allows to ensure that the two snapshots are
	 * mounted for the lifetime of the Comparison object.
	 */
	Comparison(const Snapper* snapper, Snapshots::const_iterator snapshot1,
		   Snapshots::const_iterator snapshot2, bool mount);

	~Comparison();

	const Snapper* getSnapper() const { return snapper; }

	Snapshots::const_iterator getSnapshot1() const { return snapshot1; }
	Snapshots::const_iterator getSnapshot2() const { return snapshot2; }

	Files& getFiles() { return files; }
	const Files& getFiles() const { return files; }

	UndoStatistic getUndoStatistic() const;
	XAUndoStatistic getXAUndoStatistic() const;

	vector<UndoStep> getUndoSteps() const;

	bool doUndoStep(const UndoStep& undo_step);

    private:

	void initialize();
	void create();

	/**
	 * Check the header. Throws if the header is unsupported. Return true iff a header
	 * was found.
	 */
	bool check_header(const string& line) const;

	/**
	 * Check the footer. Throws if the footer is unsupported. Return true iff a footer
	 * was found.
	 */
	bool check_footer(const string& line) const;

	bool load();

	bool load(int fd, Compression compression, bool invert);

	bool save() const;

	void filter();

	void do_mount() const;
	void do_umount() const;

	const Snapper* snapper;

	const Snapshots::const_iterator snapshot1;
	const Snapshots::const_iterator snapshot2;

	const bool mount;

	FilePaths file_paths;

	Files files;

    };

}


#endif
