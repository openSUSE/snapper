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

#include <algorithm>
#include <string.h>
#include <time.h>
#include <dbus/DBusMessage.h>

#include <snapper/Snapper.h>
#include <snapper/Exception.h>
#include <snapper/SnapperTmpl.h>
#include <snapper/Snapshot.h>
#include <snapper/Enum.h>

#include "client/Command/ListSnapshots/SnappersData.h"
#include "client/Command/ListSnapshots/Options.h"
#include "client/utils/HumanString.h"
#include "client/misc.h"

using namespace std;

namespace snapper
{
    namespace cli
    {

	Command::ListSnapshots::SnappersData::SnappersData(const Command::ListSnapshots& command) :
	    _command(command)
	{
	    calculate_used_space();
	}


	Command::ListSnapshots::SnappersData::~SnappersData()
	{}


	vector<ProxySnapper*> Command::ListSnapshots::SnappersData::snappers() const
	{
	    vector<ProxySnapper*> snappers;

	    for (auto config_name : configs())
	    {
		ProxySnapper* snapper = command().snappers().getSnapper(config_name);

		snappers.push_back(snapper);
	    }

	    return snappers;
	}


	vector<const ProxySnapshot*>
	Command::ListSnapshots::SnappersData::selected_snapshots(const ProxySnapper* snapper) const
	{
	    const ProxySnapshots& snapshots = this->snapshots(snapper);

	    vector<const ProxySnapshot*> selected;

	    for (const ProxySnapshot& snapshot : snapshots)
	    {
		switch (command().options().list_mode())
		{
		    case Options::ListMode::ALL:
		    {
			selected.push_back(&snapshot);
		    }
		    break;

		    case Options::ListMode::SINGLE:
		    {
			if (snapshot.getType() == SINGLE)
			    selected.push_back(&snapshot);
		    }
		    break;

		    case Options::ListMode::PRE_POST:
		    {
			ProxySnapshots::const_iterator post_it = find_post(snapshot, snapshots);

			if (post_it != snapshots.end())
			    selected.push_back(&snapshot);
		    }
		    break;
		}
	    }

	    return selected;
	}


	vector<string>
	Command::ListSnapshots::SnappersData::selected_columns() const
	{
	    return command().options().columns(command().global_options().output_format());
	}


	string Command::ListSnapshots::SnappersData::value_for(const string& column,
	    const ProxySnapshot& snapshot,
	    const ProxySnapshots& snapshots,
	    const ProxySnapper* snapper) const
	{
	    if (column == Options::Columns::CONFIG)
		return config_value(snapper);

	    if (column == Options::Columns::SUBVOLUME)
		return subvolume_value(snapper);

	    if (column == Options::Columns::NUMBER)
		return number_value(snapshot, snapshots);

	    if (column == Options::Columns::DEFAULT)
		return default_value(snapshot, snapshots);

	    if (column == Options::Columns::ACTIVE)
		return active_value(snapshot, snapshots);

	    if (column == Options::Columns::TYPE)
		return type_value(snapshot);

	    if (column == Options::Columns::DATE)
		return date_value(snapshot);

	    if (column == Options::Columns::USER)
		return user_value(snapshot);

	    if (column == Options::Columns::USED_SPACE)
		return used_space_value(snapshot, snapper);

	    if (column == Options::Columns::CLEANUP)
		return cleanup_value(snapshot);

	    if (column == Options::Columns::DESCRIPTION)
		return description_value(snapshot);

	    if (column == Options::Columns::USERDATA)
		return userdata_value(snapshot);

	    if (column == Options::Columns::PRE_NUMBER)
		return pre_number_value(snapshot);

	    if (column == Options::Columns::POST_NUMBER)
		return post_number_value(snapshot, snapshots);

	    if (column == Options::Columns::POST_DATE)
		return post_date_value(snapshot, snapshots);

	    return "";
	}


	vector<string> Command::ListSnapshots::SnappersData::configs() const
	{
	    vector<string> names;

	    if (!command().options().all_configs())
	    {
		names.push_back(command().global_options().config());
	    }
	    else
	    {
		map<string, ProxyConfig> configs = command().snappers().getConfigs();

		for (map<string, ProxyConfig>::value_type it : configs)
		    names.push_back(it.first);
	    }

	    return names;
	}


	const ProxySnapshots&
	Command::ListSnapshots::SnappersData::snapshots(const ProxySnapper* snapper) const
	{
	    return snapper->getSnapshots();
	}


	bool
	Command::ListSnapshots::SnappersData::calculated_used_space(const ProxySnapper* snapper) const
	{
	    map<const ProxySnapper*, bool>::const_iterator it = _calculated_used_space.find(snapper);

	    if (it == _calculated_used_space.end())
		return false;

	    return it->second;
	}


	void Command::ListSnapshots::SnappersData::calculate_used_space()
	{
	    for (ProxySnapper* snapper : snappers())
		calculate_used_space(snapper);
	}


	void
	Command::ListSnapshots::SnappersData::calculate_used_space(ProxySnapper* snapper)
	{
	    bool calculated;

	    if (!need_used_space())
	    {
		calculated = false;
	    }
	    else
	    {
		try
		{
		    snapper->calculateUsedSpace();

		    calculated = true;
		}
		catch (const QuotaException& e)
		{
		    SN_CAUGHT(e);

		    calculated = false;
		}
	    }

	    _calculated_used_space[snapper] = calculated;
	}


	bool Command::ListSnapshots::SnappersData::need_used_space() const
	{
	    vector<string> columns = selected_columns();

	    vector<string>::iterator it =
	    	find(columns.begin(), columns.end(), Options::Columns::USED_SPACE);

	    return it != columns.end();
	}


	string
	Command::ListSnapshots::SnappersData::config_value(const ProxySnapper* snapper) const
	{
	    return snapper->configName();
	}


	string
	Command::ListSnapshots::SnappersData::subvolume_value(const ProxySnapper* snapper) const
	{
	    return snapper->getConfig().getSubvolume();
	}


	string
	Command::ListSnapshots::SnappersData::number_value(const ProxySnapshot& snapshot,
	    const ProxySnapshots& snapshots) const
	{
	    return decString(snapshot.getNum());
	}


	string Command::ListSnapshots::SnappersData::default_value(const ProxySnapshot& snapshot,
	    const ProxySnapshots& snapshots) const
	{
	    return boolean_text(is_default_snapshot(snapshot, snapshots));
	}


	string Command::ListSnapshots::SnappersData::active_value(const ProxySnapshot& snapshot,
	    const ProxySnapshots& snapshots) const
	{
	    return boolean_text(is_active_snapshot(snapshot, snapshots));
	}


	string
	Command::ListSnapshots::SnappersData::date_value(const ProxySnapshot& snapshot) const
	{
	    if (command().options().list_mode() == Options::ListMode::PRE_POST)
		return snapshot_date(snapshot);

	    return snapshot.isCurrent() ? "" : snapshot_date(snapshot);
	}

	string
	Command::ListSnapshots::SnappersData::type_value(const ProxySnapshot& snapshot) const
	{
	    return toString(snapshot.getType());
	}


	string
	Command::ListSnapshots::SnappersData::user_value(const ProxySnapshot& snapshot) const
	{
	    return username(snapshot.getUid());
	}


	string Command::ListSnapshots::SnappersData::used_space_value(
	    const ProxySnapshot& snapshot, const ProxySnapper* snapper) const
	{
	    if (!calculated_used_space(snapper))
		return "";

	    return snapshot.isCurrent() ? "" : snapshot_used_value(snapshot);
	}


	string
	Command::ListSnapshots::SnappersData::cleanup_value(const ProxySnapshot& snapshot) const
	{
	    return snapshot.getCleanup();
	}


	string
	Command::ListSnapshots::SnappersData::description_value(const ProxySnapshot& snapshot) const
	{
	    return snapshot.getDescription();
	}


	string
	Command::ListSnapshots::SnappersData::userdata_value(const ProxySnapshot& snapshot) const
	{
	    return show_userdata(snapshot.getUserdata());
	}


	string
	Command::ListSnapshots::SnappersData::pre_number_value(const ProxySnapshot& snapshot) const
	{
	    return snapshot.getType() == POST ? decString(snapshot.getPreNum()) : "";
	}


	string
	Command::ListSnapshots::SnappersData::post_number_value(const ProxySnapshot& snapshot,
	    const ProxySnapshots& snapshots) const
	{
	    ProxySnapshots::const_iterator post_it = find_post(snapshot, snapshots);

	    if (post_it == snapshots.end())
		return "";

	    return number_value(*post_it, snapshots);
	}


	string
	Command::ListSnapshots::SnappersData::post_date_value(const ProxySnapshot& snapshot,
	    const ProxySnapshots& snapshots) const
	{
	    ProxySnapshots::const_iterator post_it = find_post(snapshot, snapshots);

	    if (post_it == snapshots.end())
		return "";

	    return snapshot_date(*post_it);
	}


	string
	Command::ListSnapshots::SnappersData::snapshot_date(const ProxySnapshot& snapshot) const
	{
	    bool iso = command().global_options().iso();

	    if (command().global_options().output_format() != GlobalOptions::OutputFormat::TABLE)
		iso = true;

	    return datetime(snapshot.getDate(), command().global_options().utc(), iso);
	}


	string
	Command::ListSnapshots::SnappersData::snapshot_used_value(const ProxySnapshot& snapshot) const
	{
	    uint64_t used_space = snapshot.getUsedSpace();

	    if (command().global_options().output_format() == GlobalOptions::OutputFormat::TABLE)
		return byte_to_humanstring(used_space, 2);
	    else
		return to_string(used_space);
	}


	bool
	Command::ListSnapshots::SnappersData::is_default_snapshot(const ProxySnapshot& snapshot,
	    const ProxySnapshots& snapshots) const
	{
	    auto default_snapshot = this->default_snapshot(snapshots);

	    return default_snapshot != snapshots.end() &&
	    	default_snapshot->getNum() == snapshot.getNum();
	}


	bool
	Command::ListSnapshots::SnappersData::is_active_snapshot(const ProxySnapshot& snapshot,
	    const ProxySnapshots& snapshots) const
	{
	    auto active_snapshot = this->active_snapshot(snapshots);

	    return active_snapshot != snapshots.end() &&
	    	active_snapshot->getNum() == snapshot.getNum();
	}


	ProxySnapshots::const_iterator
	Command::ListSnapshots::SnappersData::default_snapshot(const ProxySnapshots& snapshots) const
	{
	    try
	    {
		return snapshots.getDefault();
	    }
	    catch (const DBus::ErrorException& e)
	    {
		SN_CAUGHT(e);

		// If snapper was just updated and the old snapperd is still
		// running it might not know the GetDefaultSnapshot method.

		if (strcmp(e.name(), "error.unknown_method") != 0)
		    SN_RETHROW(e);

		return snapshots.end();
	    }
	}


	ProxySnapshots::const_iterator
	Command::ListSnapshots::SnappersData::active_snapshot(const ProxySnapshots& snapshots) const
	{
	    try
	    {
		return snapshots.getActive();
	    }
	    catch (const DBus::ErrorException& e)
	    {
		SN_CAUGHT(e);

		// If snapper was just updated and the old snapperd is still
		// running it might not know the GetActiveSnapshot method.

		if (strcmp(e.name(), "error.unknown_method") != 0)
		    SN_RETHROW(e);

		return snapshots.end();
	    }
	}


	ProxySnapshots::const_iterator Command::ListSnapshots::SnappersData::find_post(
	    const ProxySnapshot& snapshot, const ProxySnapshots& snapshots) const
	{
	    if (snapshot.getType() != SnapshotType::PRE)
		return snapshots.end();

	    ProxySnapshots::const_iterator it = snapshots.find(snapshot.getNum());
	    ProxySnapshots::const_iterator post_it = snapshots.findPost(it);

	    if (post_it == snapshots.end())
		return snapshots.end();

	    return post_it;
	}

    }
}
