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

#include "rollback-method.h"

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
	bool
	is_renameable_subvol(const string& subvol_name)
	{
	    return !subvol_name.empty() && subvol_name.find('/') == string::npos;
	}

    }


    bool
    use_subvol_rename(const string& rollback_method, const string& subvol_name)
    {
	const bool renameable = is_renameable_subvol(subvol_name);

	if (rollback_method == "set-default")
	    return false;

	if (rollback_method == "subvol-rename")
	{
	    if (!renameable)
		SN_THROW(Exception(_("ROLLBACK_METHOD is 'subvol-rename' but root is not "
				     "mounted with a top-level named subvolume.")));
	    return true;
	}

	if (rollback_method.empty() || rollback_method == "auto")
	    return renameable;

	SN_THROW(Exception(sformat(_("Unknown ROLLBACK_METHOD '%s'."), rollback_method.c_str())));
    }


    Ambit
    classic_or_transactional(Ambit cli_ambit, SubvolumeMode mode)
    {
	if (cli_ambit == Ambit::CLASSIC || cli_ambit == Ambit::TRANSACTIONAL)
	    return cli_ambit;

	switch (mode)
	{
	    case SubvolumeMode::READ_ONLY:  return Ambit::TRANSACTIONAL;
	    case SubvolumeMode::READ_WRITE: return Ambit::CLASSIC;
	    case SubvolumeMode::UNKNOWN:    return Ambit::AUTO;
	}

	return Ambit::AUTO;
    }

#endif

}
