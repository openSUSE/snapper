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

	Options::Options(GetOpts& parser) :
	    _parser(parser), _options()
	{}


	Options::~Options()
	{}


	bool Options::has_option(const string option_name) const
	{
	    GetOpts::parsed_opts::const_iterator option = get_option(option_name);

	    return option != _options.end();
	}


	GetOpts::parsed_opts::const_iterator
	Options::get_option(const string option_name) const
	{
	    return _options.find(option_name);
	}


	bool Options::has_errors() const
	{
	    return !errors().empty();
	}

    }
}
