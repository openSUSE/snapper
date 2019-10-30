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

#ifndef SNAPPER_CLI_COMMAND_LIST_CONFIGS_SNAPPERS_DATA_H
#define SNAPPER_CLI_COMMAND_LIST_CONFIGS_SNAPPERS_DATA_H

#include <string>
#include <vector>

#include "client/Command/ListConfigs.h"
#include "client/proxy.h"

namespace snapper
{
    namespace cli
    {

	class Command::ListConfigs::SnappersData
	{

	public:

	    class Table;

	    class Csv;

	    class Json;

	    SnappersData(const Command::ListConfigs& command);

	    virtual ~SnappersData();

	    virtual std::string output() const = 0;

	protected:

	    std::string value_for(const std::string& column, ProxySnapper* snapper) const;

	    std::vector<ProxySnapper*> snappers() const;

	private:

	    std::vector<std::string> configs() const;

	    const ListConfigs& _command;
	};

    }
}

#endif
