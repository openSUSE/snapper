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
     * Subvolumes nested in the root subvolume, as handled by a subvol-rename
     * rollback (see doc/rollback.txt).
     *
     * A btrfs snapshot does not include nested subvolumes: where one was, the
     * snapshot only holds a placeholder (inode BTRFS_EMPTY_SUBVOL_DIR_OBJECTID)
     * that nothing can be created in. After the rollback the nested subvolumes
     * stay in the old root. The rules:
     *
     *   .snapshots  moved into the new root, so the snapshots stay usable
     *   empty       placeholder removed from the new root, so the path can be
     *               recreated (e.g. by systemd-tmpfiles at boot)
     *   not empty   the rollback is refused, since the data would only be
     *               reachable in the old root
     *
     * Distributions that keep such subvolumes outside the root subvolume (e.g.
     * a separate /var) have none but .snapshots, so the rest is never used.
     * Requires libbtrfsutil, the constructor throws without it.
     */
    class NestedSubvolumes
    {
    public:

	// Inventory of the subvolumes nested directly in the subvolume of
	// root_dir, as paths relative to it (e.g. "var/lib/machines").
	explicit NestedSubvolumes(const SDir& root_dir);

	const vector<string>& paths() const { return nested; }

	// Throws if a nested subvolume other than .snapshots is not empty,
	// naming each one. Returns the empty ones.
	vector<string> check_empty(const SDir& root_dir) const;

	// Whether .snapshots is one of them.
	bool has_snapshots() const;

	// Move .snapshots from from_root into to_root, replacing its
	// placeholder there: before the exchange into the copy of the target
	// (and back if the exchange fails). Throws if it cannot be moved,
	// e.g. when it is mounted (EBUSY), leaving it in from_root.
	void move_snapshots(const SDir& from_root, const SDir& to_root) const;

	// After the exchange: remove the placeholders of the other nested
	// subvolumes from new_root. Returns a message for each one that could
	// not be removed.
	vector<string> remove_placeholders(const SDir& new_root) const;

    private:

	vector<string> nested;

    };

}


#endif
