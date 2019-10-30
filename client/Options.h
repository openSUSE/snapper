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

#ifndef SNAPPER_CLI_OPTIONS_H
#define SNAPPER_CLI_OPTIONS_H

#include <string>
#include <vector>

#include "client/utils/GetOpts.h"

namespace snapper
{
    namespace cli
    {

	class Options
	{

	public:

	    Options(GetOpts& parser);

	    virtual bool has_errors() const;

	    virtual std::vector<std::string> errors() const = 0;

	    virtual ~Options();

	protected:

	    bool has_option(const std::string option_name) const;

	    GetOpts::parsed_opts::const_iterator
	    get_option(const std::string option_name) const;

	    GetOpts& _parser;

	    GetOpts::parsed_opts _options;

	};

    }
}

#endif