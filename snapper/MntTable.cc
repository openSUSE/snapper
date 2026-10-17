/*
 * Copyright (c) [2015-2020] SUSE LLC
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


#include "snapper/MntTable.h"
#include "snapper/LoggerImpl.h"

#include <sstream>


namespace snapper
{

    struct libmnt_fs*
    MntTable::find_target_up(const string& path, int direction)
    {
	string tmp = path;

	for (;;)
	{
	    if (tmp.empty())
		return nullptr;

	    libmnt_fs* fs = mnt_table_find_target(table, tmp.c_str(), direction);
	    if (fs)
		return fs;

	    if (tmp == "/" || tmp == ".")
		return nullptr;

	    tmp = dirname(tmp);
	}
    }


    vector<string>
    fstab_options(const string& root_prefix, const string& mount_point)
    {
	vector<string> ret;

	try
	{
	    MntTable mnt_table(root_prefix);
	    mnt_table.parse_fstab();

	    libmnt_fs* fs = mnt_table.find_target(mount_point, MNT_ITER_BACKWARD);
	    if (!fs || !mnt_fs_get_options(fs))
		return ret;

	    std::istringstream stream(mnt_fs_get_options(fs));
	    for (string option; std::getline(stream, option, ',');)
		if (!option.empty())
		    ret.push_back(option);
	}
	catch (const runtime_error& e)
	{
	    y2war("cannot read fstab below '" << root_prefix << "': " << e.what());
	}

	return ret;
    }

}
