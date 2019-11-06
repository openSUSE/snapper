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

#ifndef SNAPPER_CLI_COMMAND_LIST_CONFIGS_SNAPPERS_DATA_CSV_H
#define SNAPPER_CLI_COMMAND_LIST_CONFIGS_SNAPPERS_DATA_CSV_H

#include <string>

#include "client/Command/ListConfigs/SnappersData.h"

namespace snapper
{
    namespace cli
    {

	class Command::ListConfigs::SnappersData::Csv : public Command::ListConfigs::SnappersData
	{

	public:

	    Csv(const ListConfigs& command, const std::string separator);

	    virtual std::string output() const override;

	private:

	    const std::string _separator;
	};

    }
}

#endif
