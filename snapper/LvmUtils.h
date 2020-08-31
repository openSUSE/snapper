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


#ifndef SNAPPER_LVM_UTILS_H
#define SNAPPER_LVM_UTILS_H


#include <utility>
#include <string>


namespace snapper
{
    using std::string;
    using std::pair;


    namespace LvmUtils
    {

	std::pair<string, string> split_device_name(const string& name);

    }

}


#endif
