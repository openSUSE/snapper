/*
 * Copyright (c) [2019] SUSE LLC
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

#include <string>

#include "client/Options.h"
#include "client/utils/text.h"

using namespace std;

namespace snapper
{
    namespace cli
    {

	Options::Options(GetOpts& parser)
	    : _parser(parser), _options()
	{}


	bool
	Options::has_option(const string& name) const
	{
	    return _options.find(name) != _options.end();
	}


	const string&
	Options::get_argument(const string& name) const
	{
	    return _options.find(name)->second;
	}

    }
}
