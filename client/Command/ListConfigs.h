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

#ifndef SNAPPER_CLI_COMMAND_LIST_CONFIGS_H
#define SNAPPER_CLI_COMMAND_LIST_CONFIGS_H

#include <memory>

#include "client/Command.h"
#include "client/proxy.h"

namespace snapper
{
    namespace cli
    {

	class Command::ListConfigs : public Command
	{

	public:

	    class Options;

	    class ColumnsOption;

	    class SnappersData;

	    static std::string help();

	    ListConfigs(const GlobalOptions& global_options, GetOpts& options_parser,
		ProxySnappers& snappers);

	    const Options& options() const;

	    virtual std::vector<std::string> errors() const override;

	    virtual void run() override;

	    virtual ~ListConfigs();

	private:

	    std::unique_ptr<Options> _options;
	};

    }
}

#endif
