/*
 * Copyright (c) [2024-2025] SUSE LLC
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


#include <regex>

#include "../utils/GetOpts.h"
#include "../utils/text.h"


namespace snapper
{

    vector<unsigned int>
    parse_nums(GetOpts& get_opts)
    {
	static const regex num_regex("[0-9]+", regex::extended);

	vector<unsigned int> nums;

	while (get_opts.has_args())
	{
	    string arg = get_opts.pop_arg();

	    if (!regex_match(arg, num_regex))
		SN_THROW(Exception(_("Failed to parse number.")));

	    nums.push_back(stoi(arg));
	}

	return nums;
    }

}
