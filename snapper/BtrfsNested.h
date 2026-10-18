/*
 * Copyright (c) [2026] SUSE LLC
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


#ifndef SNAPPER_BTRFS_NESTED_H
#define SNAPPER_BTRFS_NESTED_H


#include <string>
#include <vector>

#include "snapper/FileUtils.h"


namespace snapper
{

    using std::string;
    using std::vector;


    /*
     * Subvolumes nested in the root subvolume. A btrfs snapshot only holds a
     * placeholder (inode BTRFS_EMPTY_SUBVOL_DIR_OBJECTID) for them, so they
     * stay in the old root: .snapshots is moved, empty ones are freed, ones
     * with data refuse the rollback (see doc/rollback.txt). Requires
     * libbtrfsutil, the constructor throws without it.
     */
    class NestedSubvolumes
    {
    public:

	// paths relative to root_dir, e.g. "var/lib/machines"
	explicit NestedSubvolumes(const SDir& root_dir);

	const vector<string>& paths() const { return nested; }

	// throws naming the ones with data (besides .snapshots), returns the empty ones
	vector<string> check_empty(const SDir& root_dir) const;

	bool has_snapshots() const;

	// throws if it cannot be moved (e.g. EBUSY when mounted), leaving it in from_root
	void move_snapshots(const SDir& from_root, const SDir& to_root) const;

	// returns a message for each placeholder that could not be removed
	vector<string> remove_placeholders(const SDir& new_root) const;

    private:

	vector<string> nested;

    };

}


#endif
