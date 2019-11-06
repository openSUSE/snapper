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

#ifndef SNAPPER_CLI_COMMAND_LIST_SNAPSHOTS_SNAPPERS_DATA_TABLE_H
#define SNAPPER_CLI_COMMAND_LIST_SNAPSHOTS_SNAPPERS_DATA_TABLE_H

#include <string>

#include "client/Command/ListSnapshots/SnappersData.h"
#include "client/utils/Table.h"

namespace snapper
{
    namespace cli
    {

	class Command::ListSnapshots::SnappersData::Table : public Command::ListSnapshots::SnappersData
	{

	public:

	    Table(const ListSnapshots& command, TableLineStyle style);

	    virtual std::string output() const override;

	private:

	    std::string label_for(const string& column) const;

	    virtual std::string
	    number_value(const ProxySnapshot& snapshot, const ProxySnapshots& snapshots) const override;

	    virtual string boolean_text(bool value) const override;

	    std::string snapper_description(const ProxySnapper* snapper) const;

	    std::string snapper_table(const ProxySnapper* snapper) const;

	    bool skip_column(const string& column, const ProxySnapper* snapper) const;

	    TableAlign column_alignment(const string& column) const;

	    TableLineStyle _style;

	};

    }
}

#endif
