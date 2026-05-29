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

using std::regex;
using std::smatch;
using std::regex_match;


namespace snapper
{

#ifdef ENABLE_ROLLBACK

    static string
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


    RollbackMethod
    detect_rollback_method_from_options(const vector<string>& options)
    {
	string name = subvol_name_from_options(options);
	return (!name.empty() && name.find('/') == string::npos)
	    ? RollbackMethod::SUBVOL_RENAME : RollbackMethod::SET_DEFAULT;
    }


    RollbackMethod
    detect_rollback_method(const string& mount_point)
    {
	bool found = false;
	MtabData mtab_data;

	if (!getMtabData(mount_point, found, mtab_data) || !found)
	    SN_THROW(IOErrorException("failed to find mount point " + mount_point + " in /proc/mounts"));

	return detect_rollback_method_from_options(mtab_data.options);
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


    Ambit
    detect_ambit(RollbackMethod method, SubvolumeMode mode)
    {
	if (method == RollbackMethod::SUBVOL_RENAME)
	    return Ambit::CLASSIC;

	switch (mode)
	{
	    case SubvolumeMode::UNKNOWN:    return Ambit::AUTO;
	    case SubvolumeMode::READ_ONLY:  return Ambit::TRANSACTIONAL;
	    case SubvolumeMode::READ_WRITE: return Ambit::CLASSIC;
	}

	return Ambit::AUTO;
    }

#endif

}
