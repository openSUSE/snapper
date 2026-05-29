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


#ifndef SNAPPER_ROLLBACK_METHOD_H
#define SNAPPER_ROLLBACK_METHOD_H

#include <string>
#include <vector>


namespace snapper
{
    using std::string;
    using std::vector;


    enum class Ambit { AUTO, CLASSIC, TRANSACTIONAL };

#ifdef ENABLE_ROLLBACK

    enum class RollbackMethod { SET_DEFAULT, SUBVOL_RENAME };

    /**
     * Determine rollback method from already-parsed mount options.
     * Returns SUBVOL_RENAME only for a top-level named subvolume (no slashes).
     * Separated from I/O so it can be unit-tested without /proc/mounts.
     */
    RollbackMethod detect_rollback_method_from_options(const vector<string>& options);

    /**
     * Determine rollback method by reading mount options for mount_point
     * from /proc/mounts. Delegates to detect_rollback_method_from_options.
     */
    RollbackMethod detect_rollback_method(const string& mount_point);

    /**
     * Return the named subvolume that mount_point is mounted with (e.g. "@root"),
     * or an empty string if it uses the btrfs default subvolume id instead.
     */
    string get_subvol_name(const string& mount_point);

    enum class SubvolumeMode { UNKNOWN, READ_WRITE, READ_ONLY };

    Ambit detect_ambit(RollbackMethod method, SubvolumeMode mode);

#endif

}

#endif
