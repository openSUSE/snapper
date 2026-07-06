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
     * Whether the configured ROLLBACK_METHOD and the root mount select the
     * subvolume-rename mechanism. rollback_method is the ROLLBACK_METHOD value
     * ("auto"/""/"set-default"/"subvol-rename"), subvol_name the named root
     * subvolume ("" when mounted by default subvolume id). Throws for an unknown
     * ROLLBACK_METHOD or when subvol-rename is requested without a top-level
     * named subvolume.
     */
    bool use_subvol_rename(const string& rollback_method, const string& subvol_name);

    /**
     * Ambit for the set-default mechanism: an explicit --ambit (cli_ambit) wins,
     * otherwise it is derived from the read-only/-write state of the current
     * default snapshot (mode). Returns AUTO when it cannot be determined.
     */
    Ambit classic_or_transactional(Ambit cli_ambit, SubvolumeMode mode);

#endif

}

#endif
