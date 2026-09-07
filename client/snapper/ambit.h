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


#ifndef SNAPPER_AMBIT_H
#define SNAPPER_AMBIT_H

#include <string>
#include <vector>

#include "client/snapper/GlobalOptions.h"


namespace snapper
{
    using std::string;
    using std::vector;

#ifdef ENABLE_ROLLBACK

    using Ambit = GlobalOptions::Ambit;

    enum class SubvolumeMode { UNKNOWN, READ_WRITE, READ_ONLY };

    /**
     * Return the named subvolume from already-parsed mount options (e.g. "@root"),
     * or an empty string if mounted by the btrfs default subvolume id. The name
     * may contain slashes for a nested subvolume. Separated from I/O so it can be
     * unit-tested without /proc/mounts.
     */
    string subvol_name_from_options(const vector<string>& options);

    /**
     * Return the named subvolume that mount_point is mounted with (e.g. "@root"),
     * or an empty string if it uses the btrfs default subvolume id instead.
     */
    string get_subvol_name(const string& mount_point);

    /**
     * Determine the effective ambit for the rollback. An explicit --ambit
     * (cli_ambit) wins; SUBVOL_RENAME is validated against the root mount and
     * throws when root is not mounted with a top-level named subvolume. With
     * AUTO, a top-level named root subvolume (subvol_name, "" when mounted by
     * default subvolume id) selects SUBVOL_RENAME, otherwise the ambit is
     * derived from the read-only/-write state of the current default snapshot
     * (mode). Returns AUTO when it cannot be determined.
     */
    Ambit determine_ambit(Ambit cli_ambit, const string& subvol_name, SubvolumeMode mode);

    /**
     * Whether a rollback with the given ambit sets the default subvolume id
     * while root is mounted with a top-level subvol= name (subvol_name) - the
     * kernel then ignores the default subvolume id and the rollback has no
     * effect on the next boot. Used to warn when an explicit --ambit conflicts
     * with the root mount. False for a nested name, which the kernel also
     * shows for a mount by the default subvolume id.
     */
    bool is_set_default_ineffective(Ambit ambit, const string& subvol_name);

#endif

}

#endif
