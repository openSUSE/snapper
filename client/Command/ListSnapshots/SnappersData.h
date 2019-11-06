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

#ifndef SNAPPER_CLI_COMMAND_LIST_SNAPSHOTS_SNAPPERS_DATA_H
#define SNAPPER_CLI_COMMAND_LIST_SNAPSHOTS_SNAPPERS_DATA_H

#include <string>
#include <vector>
#include <map>

#include "client/Command/ListSnapshots.h"
#include "client/proxy.h"

namespace snapper
{
    namespace cli
    {

	class Command::ListSnapshots::SnappersData
	{

	public:

	    class Table;

	    class Csv;

	    class Json;

	    SnappersData(const Command::ListSnapshots& command);

	    virtual ~SnappersData();

	    virtual std::string output() const = 0;

	protected:

	    const ListSnapshots& command() const
	    {
		return _command;
	    }

	    std::vector<ProxySnapper*> snappers() const;

	    vector<const ProxySnapshot*> selected_snapshots(const ProxySnapper* snapper) const;

	    std::vector<std::string> selected_columns() const;

	    std::string value_for(const string& column,
		const ProxySnapshot& snapshot, const ProxySnapshots& snapshots,
		const ProxySnapper* snapper) const;

	private:

	    std::vector<std::string> configs() const;

	    const ProxySnapshots& snapshots(const ProxySnapper* snapper) const;

	    bool calculated_used_space(const ProxySnapper* snapper) const;

	    void calculate_used_space();

	    void calculate_used_space(ProxySnapper* snapper);

	    bool need_used_space() const;

	    std::string config_value(const ProxySnapper* snapper) const;

	    std::string subvolume_value(const ProxySnapper* snapper) const;

	    virtual std::string
	    number_value(const ProxySnapshot& snapshot, const ProxySnapshots& snapshots) const;

	    std::string
	    default_value(const ProxySnapshot& snapshot, const ProxySnapshots& snapshots) const;

	    std::string
	    active_value(const ProxySnapshot& snapshot, const ProxySnapshots& snapshots) const;

	    std::string type_value(const ProxySnapshot& snapshot) const;

	    std::string date_value(const ProxySnapshot& snapshot) const;

	    std::string user_value(const ProxySnapshot& snapshot) const;

	    std::string
	    used_space_value(const ProxySnapshot& snapshot, const ProxySnapper* snapper) const;

	    std::string cleanup_value(const ProxySnapshot& snapshot) const;

	    std::string description_value(const ProxySnapshot& snapshot) const;

	    std::string userdata_value(const ProxySnapshot& snapshot) const;

	    std::string pre_number_value(const ProxySnapshot& snapshot) const;

	    std::string post_number_value(const ProxySnapshot& snapshot,
		const ProxySnapshots& snapshots) const;

	    std::string
	    post_date_value(const ProxySnapshot& snapshot, const ProxySnapshots& snapshots) const;

	    std::string snapshot_date(const ProxySnapshot& snapshot) const;

	    std::string snapshot_used_value(const ProxySnapshot& snapshot) const;

	    bool
	    is_default_snapshot(const ProxySnapshot& snapshot, const ProxySnapshots& snapshots) const;

	    bool
	    is_active_snapshot(const ProxySnapshot& snapshot, const ProxySnapshots& snapshots) const;

	    ProxySnapshots::const_iterator default_snapshot(const ProxySnapshots& snapshots) const;

	    ProxySnapshots::const_iterator active_snapshot(const ProxySnapshots& snapshots) const;

	    ProxySnapshots::const_iterator
	    find_post(const ProxySnapshot& snapshot, const ProxySnapshots& snapshots) const;

	    virtual string boolean_text(bool value) const = 0;

	    const ListSnapshots& _command;

	    std::map<const ProxySnapper*, bool> _calculated_used_space;

	};

    }
}

#endif
