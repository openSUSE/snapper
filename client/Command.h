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

#ifndef SNAPPER_CLI_COMMAND_H
#define SNAPPER_CLI_COMMAND_H

#include <string>
#include <vector>

#include "client/GlobalOptions.h"
#include "client/proxy.h"
#include "client/utils/GetOpts.h"
#include "client/utils/Table.h"

namespace snapper
{
    namespace cli
    {

	class Command
	{

	public:

	    class ColumnsOption;

	    class GetConfig;

	    class ListConfigs;

	    class ListSnapshots;

	    Command(const GlobalOptions& global_options, GetOpts& options_parser,
		ProxySnappers& snappers);

	    const GlobalOptions& global_options() const
	    {
		return _global_options;
	    }

	    ProxySnappers& snappers() const
	    {
		return _snappers;
	    }

	    virtual void run() = 0;

	    virtual ~Command() {}

	protected:

	    GetOpts& _options_parser;

	private:

	    const GlobalOptions& _global_options;

	    ProxySnappers& _snappers;

	};

    }
}

#endif
