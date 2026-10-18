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

#include <optional>
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

    // options of all rootflags= of a kernel command line, joined
    vector<string> rootflags_from_cmdline(const string& cmdline);

    /**
     * The root subvolume the next boot mounts, from the boot configuration:
     * /proc/mounts shows the running root under its current name, e.g.
     * <subvol>.rollback.N after a rollback without a reboot.
     */
    struct BootRoot
    {
	// top-level name the root is mounted by, "" when booted by the btrfs
	// default subvolume id
	string subvol;

	// reason to refuse the rollback, "" if none
	string unsupported;

	// fstab also has subvolid=, which no longer matches after a rename
	bool stale_subvolid = false;
    };

    /**
     * rootflags is nullopt when the kernel command line is unknown (another
     * root prefix, a chroot). The rules are described in doc/rollback.txt.
     */
    BootRoot boot_root_from(const std::optional<vector<string>>& rootflags,
			    const vector<string>& fstab_options, const string& running_subvol);

    // from /proc/cmdline (running system only) and <root_prefix>/etc/fstab
    BootRoot get_boot_root(const string& root_prefix);

    /**
     * An explicit cli_ambit wins (SUBVOL_RENAME throws if subvol_name is not
     * a top-level name). Returns AUTO when it cannot be determined.
     */
    Ambit determine_ambit(Ambit cli_ambit, const string& subvol_name, SubvolumeMode mode);

    // set-default has no effect when the root is mounted by a top-level name
    bool is_set_default_ineffective(Ambit ambit, const string& subvol_name);

#endif

}

#endif
