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


#include "config.h"

#include "ambit.h"

#include <regex>

#include <snapper/AppUtil.h>
#include <snapper/Exception.h>

#include "../utils/text.h"

using std::regex;
using std::smatch;
using std::regex_match;


namespace snapper
{

#ifdef ENABLE_ROLLBACK

    string
    subvol_name_from_options(const vector<string>& options)
    {
	static const regex re("^subvol=/?(\\S+)$");

	for (const string& opt : options)
	{
	    smatch m;
	    if (regex_match(opt, m, re) && m[1].str() != "/")
		return m[1].str();
	}

	return "";
    }


    string
    get_subvol_name(const string& mount_point)
    {
	bool found = false;
	MtabData mtab_data;

	if (!getMtabData(mount_point, found, mtab_data) || !found)
	    return "";

	return subvol_name_from_options(mtab_data.options);
    }


    namespace
    {

	// A subvolume can only be swapped by name when it is a single top-level
	// component; nested names (containing a slash) fall back to set-default.
	// Btrfs::rollbackSubvolRename enforces the same rule for its callers.
	bool
	is_renameable_subvol(const string& subvol_name)
	{
	    return !subvol_name.empty() && subvol_name.find('/') == string::npos;
	}

    }


    Ambit
    determine_ambit(Ambit cli_ambit, const string& subvol_name, SubvolumeMode mode)
    {
	// an explicit --ambit wins
	if (cli_ambit == Ambit::SUBVOL_RENAME)
	{
	    if (!is_renameable_subvol(subvol_name))
		SN_THROW(Exception(_("Ambit is 'subvol-rename' but root is not "
				     "mounted with a top-level named subvolume.")));
	    return cli_ambit;
	}

	if (cli_ambit != Ambit::AUTO)
	    return cli_ambit;

	// a system mounting root by a top-level named subvolume needs the rename
	// mechanism since the btrfs default subvolume id is ignored at boot
	if (is_renameable_subvol(subvol_name))
	    return Ambit::SUBVOL_RENAME;

	// otherwise derive from the read-only/-write state of the default snapshot
	switch (mode)
	{
	    case SubvolumeMode::READ_ONLY:  return Ambit::TRANSACTIONAL;
	    case SubvolumeMode::READ_WRITE: return Ambit::CLASSIC;
	    case SubvolumeMode::UNKNOWN:    return Ambit::AUTO;
	}

	return Ambit::AUTO;
    }


    bool
    is_set_default_ineffective(Ambit ambit, const string& subvol_name)
    {
	// Only a top-level name indicates a by-name mount: for a mount by the
	// default subvolume id the kernel shows the resolved (nested) path in
	// /proc/mounts, where set-default works as intended.
	return (ambit == Ambit::CLASSIC || ambit == Ambit::TRANSACTIONAL) &&
	    is_renameable_subvol(subvol_name);
    }

#endif

}
