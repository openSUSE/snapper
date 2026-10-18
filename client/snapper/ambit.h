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

    /**
     * Options of all rootflags= of a kernel command line (e.g. the content of
     * /proc/cmdline) joined, with surrounding quotes removed.
     */
    vector<string> rootflags_from_cmdline(const string& cmdline);

    /**
     * The root subvolume the system boots, from the boot configuration (not
     * from /proc/mounts, which shows the running root under its current name,
     * e.g. <subvol>.rollback.N after a rollback without a reboot, or a booted
     * snapshot).
     */
    struct BootRoot
    {
	// top-level name the root is mounted by, "" when booted by the btrfs
	// default subvolume id
	string subvol;

	// why no rollback method changes what boots, "" if one can
	string unsupported;

	// fstab also has subvolid=, which no longer matches after a rename
	bool stale_subvolid = false;
    };

    /**
     * Determine the BootRoot from the rootflags of the kernel command line
     * (nullopt when unknown, for a system at another root prefix), the options
     * of the fstab entry for / and the current name of the running root (from
     * /proc/mounts). The command line decides at boot: without subvol= or
     * subvolid= the btrfs default subvolume boots. A one-off boot of a
     * snapshot (by subvolid= or a nested name) and an unknown command line
     * fall back to fstab, which describes the normal boot. Unsupported when
     * command line and fstab name different subvolumes, when the command line
     * pins the root by subvolid= (also next to subvol=, which no longer
     * matches after a rename), or when the normal root is mounted by a nested
     * name.
     */
    BootRoot boot_root_from(const std::optional<vector<string>>& rootflags,
			    const vector<string>& fstab_options, const string& running_subvol);

    /**
     * Determine the BootRoot of the system at root_prefix from /proc/cmdline and
     * <root_prefix>/etc/fstab.
     */
    BootRoot get_boot_root(const string& root_prefix);

    /**
     * Determine the effective ambit for the rollback. An explicit --ambit
     * (cli_ambit) wins; SUBVOL_RENAME is validated against the boot root and
     * throws when the system does not boot a top-level named subvolume. With
     * AUTO, a top-level named boot root (subvol_name, "" when booted by the
     * default subvolume id, see BootRoot) selects SUBVOL_RENAME, otherwise the ambit is
     * derived from the read-only/-write state of the current default snapshot
     * (mode). Returns AUTO when it cannot be determined.
     */
    Ambit determine_ambit(Ambit cli_ambit, const string& subvol_name, SubvolumeMode mode);

    /**
     * Whether a rollback with the given ambit sets the default subvolume id
     * while the system boots a top-level subvol= name (subvol_name, see
     * BootRoot) - the kernel then ignores the default subvolume id and the
     * rollback has no effect on the next boot. Used to warn when an explicit
     * --ambit conflicts with the boot configuration.
     */
    bool is_set_default_ineffective(Ambit ambit, const string& subvol_name);

#endif

}

#endif
