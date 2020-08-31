/*
 * Copyright (c) [2011-2014] Novell, Inc.
 * Copyright (c) 2020 SUSE LLC
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


#include <boost/algorithm/string.hpp>

#include "snapper/LvmUtils.h"
#include "snapper/Regex.h"
#include "snapper/AppUtil.h"


namespace snapper
{

    namespace LvmUtils
    {

	pair<string, string>
	split_device_name(const string& name)
	{
	    Regex rx("^/dev/mapper/(.*[^-])-([^-].*)$");
	    if (!rx.match(name))
		throw std::runtime_error("faild to split device name into volume group and "
					 "logical volume name");

	    string vg_name = boost::replace_all_copy(rx.cap(1), "--", "-");
	    string lv_name = boost::replace_all_copy(rx.cap(2), "--", "-");

	    return make_pair(vg_name, lv_name);
	}

    }
}
