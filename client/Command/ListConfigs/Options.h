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

#ifndef SNAPPER_CLI_COMMAND_LIST_CONFIGS_OPTIONS_H
#define SNAPPER_CLI_COMMAND_LIST_CONFIGS_OPTIONS_H

#include "client/Options.h"
#include "client/Command/ListConfigs.h"
#include "client/Command/ColumnsOption.h"

namespace snapper
{
    namespace cli
    {

	class Command::ListConfigs::Options : public cli::Options
	{

	public:

	    struct Columns
	    {
		static const std::string CONFIG;
		static const std::string SUBVOLUME;
	    };

	    static const std::vector<std::string> ALL_COLUMNS;

	    static std::string help_text();

	    Options(GetOpts& parser);

	    std::vector<std::string> columns() const;

	private:

	    void parse_options();

	    string columns_raw() const;

	    Command::ColumnsOption _columns_option;

	};

    }
}

#endif
