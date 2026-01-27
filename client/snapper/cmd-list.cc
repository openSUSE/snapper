/*
 * Copyright (c) [2011-2015] Novell, Inc.
 * Copyright (c) [2016-2024] SUSE LLC
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


#include "config.h"

#include <iostream>
#include <any>

#include <snapper/SnapperTmpl.h>
#include <snapper/BtrfsUtils.h>

#include "../utils/text.h"
#include "../utils/help.h"
#include "../proxy/proxy.h"
#include "../proxy/locker.h"
#include "GlobalOptions.h"
#include "../misc.h"
#include "../utils/TableFormatter.h"
#include "../utils/CsvFormatter.h"
#include "../utils/JsonFormatter.h"
#include "../utils/OutputOptions.h"
#include "dbus/DBusMessage.h"
#include "../utils/HumanString.h"


namespace snapper
{

    using namespace std;


    void
    help_list()
    {
	cout << _("  List snapshots:") << '\n'
	     << _("\tsnapper list") << '\n'
	     << '\n'
	     << _("    Options for 'list' command:") << '\n';

	print_options({
	    { _("--type, -t <type>"), _("Type of snapshots to list.") },
	    { _("--disable-used-space"), _("Disable showing used space.") },
	    { _("--all-configs, -a"), _("List snapshots from all accessible configs.") },
	    { _("--columns <columns>"), _("Columns to show separated by comma." ) }
	});
    }


    namespace
    {

	enum class Column
	{
	    CONFIG, SUBVOLUME, NUMBER, DEFAULT, ACTIVE, TYPE, DATE, USER, USED_SPACE,
	    CLEANUP, DESCRIPTION, USERDATA, PRE_NUMBER, POST_NUMBER, POST_DATE, READ_ONLY
	};

	enum class ListMode { ALL, SINGLE, PRE_POST };


	/**
	 * Some help for the output, e.g. check if a snapshot is the default or active
	 * snapshot, check if a snapshot should be skipped in the output or check if the
	 * used-space works.
	 */
	class OutputHelper
	{
	public:

	    OutputHelper(const ProxySnapper* snapper, const vector<Column>& columns);

	    bool is_default(const ProxySnapshot& snapshot) const
	    {
		return default_snapshot != snapshots.end() && default_snapshot->getNum() == snapshot.getNum();
	    }

	    bool is_active(const ProxySnapshot& snapshot) const
	    {
		return active_snapshot != snapshots.end() && active_snapshot->getNum() == snapshot.getNum();
	    }

	    char extra_sign(const ProxySnapshot& snapshot) const
	    {
		static const char sign[2][2] = { { ' ', '-' }, { '+', '*' } };
		return sign[is_default(snapshot)][is_active(snapshot)];
	    }

	    ProxySnapshots::const_iterator find_post(const ProxySnapshot& snapshot) const
	    {
		if (snapshot.getType() != SnapshotType::PRE)
		    return snapshots.end();

		ProxySnapshots::const_iterator it = snapshots.find(snapshot.getNum());

		return snapshots.findPost(it);
	    }

	    bool is_used_space_broken() const { return used_space_broken; }

	    bool skip_column(Column column) const { return column == Column::USED_SPACE && used_space_broken; }

	    bool skip_snapshot(const ProxySnapshot& snapshot, ListMode list_mode) const;

	    const ProxySnapper* snapper;

	private:

	    Locker locker;

	public:

	    const ProxySnapshots& snapshots;

	private:

	    ProxySnapshots::const_iterator default_snapshot;
	    ProxySnapshots::const_iterator active_snapshot;

	    bool used_space_broken = true;

#ifdef ENABLE_BTRFS

	    /**
	     * For all btrfses (by btrfs filesystem uuid) keep track of whether used space
	     * is broken.
	     */
	    static map<Uuid, bool> used_space_broken_by_uuid;

#endif

	};


#ifdef ENABLE_BTRFS

	map<Uuid, bool> OutputHelper::used_space_broken_by_uuid;

#endif


	OutputHelper::OutputHelper(const ProxySnapper* snapper, const vector<Column>& columns)
	    : snapper(snapper), locker(snapper), snapshots(snapper->getSnapshots()),
	      default_snapshot(snapshots.end()), active_snapshot(snapshots.end())
	{
	    try
	    {
		default_snapshot = snapshots.getDefault();
		active_snapshot = snapshots.getActive();
	    }
	    catch (const DBus::ErrorException& e)
	    {
		SN_CAUGHT(e);

		// If snapper was just updated and the old snapperd is still
		// running it might not know the GetDefaultSnapshot and
		// GetActiveSnapshot methods.

		if (strcmp(e.name(), "error.unknown_method") != 0)
		    SN_RETHROW(e);
	    }

	    // Calculate the used space iff columns include USED_SPACE. Also sets
	    // used_space_broken (even cached) for use in skip_column.

	    if (find(columns.begin(), columns.end(), Column::USED_SPACE) != columns.end())
	    {
		string subvolume = snapper->getConfig().getSubvolume();

#ifdef ENABLE_BTRFS

		try
		{
		    Uuid uuid = BtrfsUtils::get_uuid(subvolume);

		    map<Uuid, bool>::const_iterator pos = used_space_broken_by_uuid.find(uuid);
		    if (pos != used_space_broken_by_uuid.end())
		    {
			used_space_broken = pos->second;
		    }
		    else
		    {
			try
			{
			    snapper->calculateUsedSpace();
			    used_space_broken = used_space_broken_by_uuid[uuid] = false;
			}
			catch (const QuotaException& e)
			{
			    SN_CAUGHT(e);

			    used_space_broken_by_uuid[uuid] = true;
			}
		    }
		}
		catch (const runtime_error& e)
		{
		    // getting uuid failed, maybe it is a LVM config
		}

#endif
	    }
	}


	bool
	OutputHelper::skip_snapshot(const ProxySnapshot& snapshot, ListMode list_mode) const
	{
	    switch (list_mode)
	    {
		case ListMode::ALL:
		    return false;

		case ListMode::SINGLE:
		    return snapshot.getType() != SINGLE;

		case ListMode::PRE_POST:
		    return find_post(snapshot) == snapshots.end();
	    }

	    SN_THROW(Exception("invalid column value"));
	    __builtin_unreachable();
	}


	vector<Column>
	default_columns(ListMode list_mode, GlobalOptions::OutputFormat format)
	{
	    switch (list_mode)
	    {
		case ListMode::ALL:
		{
		    switch (format)
		    {
			case GlobalOptions::OutputFormat::TABLE:
			    return { Column::NUMBER, Column::TYPE, Column::PRE_NUMBER, Column::DATE, Column::USER,
				     Column::USED_SPACE, Column::CLEANUP, Column::DESCRIPTION, Column::USERDATA };

			case GlobalOptions::OutputFormat::CSV:
			    return { Column::CONFIG, Column::SUBVOLUME, Column::NUMBER, Column::DEFAULT,
				     Column::ACTIVE, Column::TYPE, Column::PRE_NUMBER, Column::DATE, Column::USER,
				     Column::USED_SPACE, Column::CLEANUP, Column::DESCRIPTION, Column::USERDATA };

			case GlobalOptions::OutputFormat::JSON:
			    return { Column::SUBVOLUME, Column::NUMBER, Column::DEFAULT, Column::ACTIVE,
				     Column::TYPE, Column::PRE_NUMBER, Column::DATE, Column::USER,
				     Column::USED_SPACE, Column::CLEANUP, Column::DESCRIPTION, Column::USERDATA };
		    }
		}
		break;

		case ListMode::SINGLE:
		{
		    switch (format)
		    {
			case GlobalOptions::OutputFormat::TABLE:
			    return { Column::NUMBER, Column::DATE, Column::USER, Column::USED_SPACE,
				     Column::DESCRIPTION, Column::USERDATA };

			case GlobalOptions::OutputFormat::CSV:
			    return { Column::CONFIG, Column::SUBVOLUME, Column::NUMBER, Column::DEFAULT,
				     Column::ACTIVE, Column::DATE, Column::USER, Column::USED_SPACE,
				     Column::DESCRIPTION, Column::USERDATA };

			case GlobalOptions::OutputFormat::JSON:
			    return { Column::SUBVOLUME, Column::NUMBER, Column::DEFAULT, Column::ACTIVE,
				     Column::DATE, Column::USER, Column::USED_SPACE, Column::DESCRIPTION,
				     Column::USERDATA };
		    }
		}
		break;

		case ListMode::PRE_POST:
		{
		    switch (format)
		    {
			case GlobalOptions::OutputFormat::TABLE:
			    return { Column::NUMBER, Column::POST_NUMBER, Column::DATE, Column::POST_DATE,
				     Column::DESCRIPTION, Column::USERDATA };

			case GlobalOptions::OutputFormat::CSV:
			    return { Column::CONFIG, Column::SUBVOLUME, Column::NUMBER, Column::DEFAULT,
				     Column::ACTIVE, Column::POST_NUMBER, Column::DATE, Column::POST_DATE,
				     Column::DESCRIPTION, Column::USERDATA };

			case GlobalOptions::OutputFormat::JSON:
			    return { Column::SUBVOLUME, Column::NUMBER, Column::DEFAULT, Column::ACTIVE,
				     Column::POST_NUMBER, Column::DATE, Column::POST_DATE, Column::DESCRIPTION,
				     Column::USERDATA };
		    }
		}
		break;
	    }

	    SN_THROW(Exception("invalid list mode or output format"));
	    __builtin_unreachable();
	}


	Cell
	header_for(ListMode list_mode, Column column)
	{
	    switch (column)
	    {
		case Column::CONFIG:
		    return Cell(_("Config"));

		case Column::SUBVOLUME:
		    return Cell(_("Subvolume"));

		case Column::NUMBER:
		    if (list_mode != ListMode::PRE_POST)
			return Cell(_("#"), Id::NUMBER, Align::RIGHT);
		    else
			return Cell(_("Pre #"), Id::NUMBER, Align::RIGHT);

		case Column::DEFAULT:
		    return Cell(_("Default"));

		case Column::ACTIVE:
		    return Cell(_("Active"));

		case Column::TYPE:
		    return Cell(_("Type"), Id::TYPE);

		case Column::DATE:
		    if (list_mode != ListMode::PRE_POST)
			return Cell(_("Date"));
		    else
			return Cell(_("Pre Date"));

		case Column::USER:
		    return Cell(_("User"));

		case Column::USED_SPACE:
		    return Cell(_("Used Space"), Align::RIGHT);;

		case Column::CLEANUP:
		    return Cell(_("Cleanup"));

		case Column::DESCRIPTION:
		    return Cell(_("Description"), Id::DESCRIPTION);

		case Column::USERDATA:
		    return Cell(_("Userdata"), Id::USERDATA);

		case Column::PRE_NUMBER:
		    return Cell(_("Pre #"), Id::PRE_NUMBER, Align::RIGHT);

		case Column::POST_NUMBER:
		    return Cell(_("Post #"), Id::POST_NUMBER, Align::RIGHT);

		case Column::POST_DATE:
		    return Cell(_("Post Date"));

		case Column::READ_ONLY:
		    return Cell(_("Read-Only"));
	    }

	    SN_THROW(Exception("invalid column value"));
	    __builtin_unreachable();
	}


	std::any
	value_for_as_any(const OutputOptions& output_options, const OutputHelper& output_helper, Column column,
			 const ProxySnapshot& snapshot)
	{
	    switch (column)
	    {
		case Column::CONFIG:
		    return output_helper.snapper->configName();

		case Column::SUBVOLUME:
		    return output_helper.snapper->getConfig().getSubvolume();

		case Column::NUMBER:
		{
		    if (output_options.human)
			return decString(snapshot.getNum()) + output_helper.extra_sign(snapshot);
		    else
			return snapshot.getNum();
		}

		case Column::DEFAULT:
		    return output_helper.is_default(snapshot);

		case Column::ACTIVE:
		    return output_helper.is_active(snapshot);

		case Column::TYPE:
		    return toString(snapshot.getType());

		case Column::DATE:
		    return snapshot.isCurrent() ? "" : datetime(snapshot.getDate(), output_options.utc,
								output_options.iso);

		case Column::USER:
		    return username(snapshot.getUid());

		case Column::USED_SPACE:
		{
		    if (snapshot.isCurrent() || output_helper.is_used_space_broken())
			return nullptr;

		    uint64_t used_space = snapshot.getUsedSpace();
		    if (output_options.human)
			return byte_to_humanstring(used_space, false, 2);
		    else
			return used_space;
		}

		case Column::CLEANUP:
		    return snapshot.getCleanup();

		case Column::DESCRIPTION:
		    return snapshot.getDescription();

		case Column::USERDATA:
		    return snapshot.getUserdata();

		case Column::PRE_NUMBER:
		{
		    if (snapshot.getType() != POST)
			return nullptr;

		    return snapshot.getPreNum();
		}

		case Column::POST_NUMBER:
		{
		    ProxySnapshots::const_iterator it = output_helper.find_post(snapshot);
		    if (it == output_helper.snapshots.end())
			return nullptr;

		    if (output_options.human)
			return decString(it->getNum()) + output_helper.extra_sign(*it);
		    else
			return it->getNum();
		}

		case Column::POST_DATE:
		{
		    ProxySnapshots::const_iterator it = output_helper.find_post(snapshot);
		    if (it == output_helper.snapshots.end())
			return nullptr;

		    return datetime(it->getDate(), output_options.utc, output_options.iso);
		}

		case Column::READ_ONLY:
		    return snapshot.isReadOnly();
	    }

	    SN_THROW(Exception("invalid column value in value_for_as_any"));
	    __builtin_unreachable();
	}


	string
	value_for_as_string(const OutputOptions& output_options, const OutputHelper& output_helper,
			    Column column, const ProxySnapshot& snapshot)
	{
	    std::any value = value_for_as_any(output_options, output_helper, column, snapshot);

	    if (value.type() == typeid(nullptr_t))
	    {
		return "";
	    }
	    else if (value.type() == typeid(unsigned int))
	    {
		return decString(std::any_cast<unsigned int>(value));
	    }
	    else if (value.type() == typeid(bool))
	    {
		if (output_options.human)
		    return std::any_cast<bool>(value) ? _("yes") : _("no");
		else
		    return std::any_cast<bool>(value) ? "yes" : "no";
	    }
	    else if (value.type() == typeid(string))
	    {
		return std::any_cast<string>(value).c_str();
	    }
	    else if (value.type() == typeid(uint64_t))
	    {
		return decString(std::any_cast<uint64_t>(value));
	    }
	    else if (value.type() == typeid(map<string, string>))
	    {
		return show_userdata(std::any_cast<map<string, string>>(value));
	    }

	    SN_THROW(Exception("invalid column type in value_for_as_string"));
	    __builtin_unreachable();
	}


	json_object*
	value_for_as_json(const OutputOptions& output_options, const OutputHelper& output_helper,
			  Column column, const ProxySnapshot& snapshot)
	{
	    std::any value = value_for_as_any(output_options, output_helper, column, snapshot);

	    if (value.type() == typeid(nullptr_t))
	    {
		return nullptr;
	    }
	    else if (value.type() == typeid(unsigned int))
	    {
		return json_object_new_int(std::any_cast<unsigned int>(value));
	    }
	    else if (value.type() == typeid(bool))
	    {
		return json_object_new_boolean(std::any_cast<bool>(value));
	    }
	    else if (value.type() == typeid(string))
	    {
		return json_object_new_string(std::any_cast<string>(value).c_str());
	    }
	    else if (value.type() == typeid(uint64_t))
	    {
#if JSON_C_VERSION_NUM >= ((0 << 16) | (14 << 8) | 0)
		return json_object_new_uint64(std::any_cast<uint64_t>(value));
#else
		return json_object_new_int64(std::any_cast<uint64_t>(value));
#endif
	    }
	    else if (value.type() == typeid(map<string, string>))
	    {
		map<string, string> tmp = std::any_cast<map<string, string>>(value);
		if (tmp.empty())
		    return nullptr;

		json_object* json_userdata = json_object_new_object();
		for (const map<string, string>::value_type& sub_value : tmp)
		    json_object_object_add(json_userdata, sub_value.first.c_str(),
					   json_object_new_string(sub_value.second.c_str()));

		return json_userdata;
	    }

	    SN_THROW(Exception("invalid column type in value_for_as_json"));
	    __builtin_unreachable();
	}


	void
	output(GlobalOptions& global_options, const vector<Column>& columns,
	       const vector<const ProxySnapper*>& snappers, ListMode list_mode)
	{
	    switch (global_options.output_format())
	    {
		case GlobalOptions::OutputFormat::TABLE:
		{
		    OutputOptions output_options(global_options.utc(), global_options.iso(), true);

		    bool first_table = true;

		    for (const ProxySnapper* snapper : snappers)
		    {
			if (!first_table)
			    cout << endl;

			if (snappers.size() > 1)
			{
			    cout << "Config: " << snapper->configName() << ", subvolume: "
				 << snapper->getConfig().getSubvolume() << endl;
			}

			OutputHelper output_helper(snapper, columns);

			TableFormatter formatter(global_options.table_style());

			for (Column column : columns)
			{
			    if (output_helper.skip_column(column))
				continue;

			    formatter.header().push_back(header_for(list_mode, column));

			    formatter.trim().push_back(Id::NUMBER);
			    formatter.trim().push_back(Id::PRE_NUMBER);
			    formatter.trim().push_back(Id::POST_NUMBER);

			    if (global_options.abbreviate())
				formatter.abbreviate().push_back(Id::DESCRIPTION);
			}

			for (const ProxySnapshot& snapshot : output_helper.snapshots)
			{
			    if (output_helper.skip_snapshot(snapshot, list_mode))
				continue;

			    vector<string> row;

			    for (Column column : columns)
			    {
				if (output_helper.skip_column(column))
				    continue;

				row.push_back(value_for_as_string(output_options, output_helper, column,
								  snapshot));
			    }

			    formatter.rows().push_back(row);
			}

			cout << formatter;

			first_table = false;
		    }
		}
		break;

		case GlobalOptions::OutputFormat::CSV:
		{
		    OutputOptions output_options(global_options.utc(), true, false);

		    CsvFormatter formatter(global_options.separator(), global_options.headers());

		    for (Column column : columns)
			formatter.header().push_back(toString(column));

		    for (const ProxySnapper* snapper : snappers)
		    {
			OutputHelper output_helper(snapper, columns);

			for (const ProxySnapshot& snapshot : output_helper.snapshots)
			{
			    if (output_helper.skip_snapshot(snapshot, list_mode))
				continue;

			    vector<string> row;

			    for (Column column : columns)
				row.push_back(value_for_as_string(output_options, output_helper, column,
								  snapshot));

			    formatter.rows().push_back(row);
			}
		    }

		    cout << formatter;
		}
		break;

		case GlobalOptions::OutputFormat::JSON:
		{
		    OutputOptions output_options(global_options.utc(), true, false);

		    JsonFormatter formatter;

		    for (const ProxySnapper* snapper : snappers)
		    {
			json_object* json_config = json_object_new_array();
			json_object_object_add(formatter.root(), snapper->configName().c_str(), json_config);

			OutputHelper output_helper(snapper, columns);

			for (const ProxySnapshot& snapshot : output_helper.snapshots)
			{
			    if (output_helper.skip_snapshot(snapshot, list_mode))
				continue;

			    json_object* json_snapshot = json_object_new_object();
			    json_object_array_add(json_config, json_snapshot);

			    for (const Column& column : columns)
				json_object_object_add(json_snapshot, toString(column).c_str(),
						       value_for_as_json(output_options, output_helper,
									 column, snapshot));
			}
		    }

		    cout << formatter;
		}
		break;
	    }
	}
    }


    void
    command_list(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers* snappers,
		 ProxySnapper* snapper, Plugins::Report& report)
    {
	const vector<Option> options = {
	    Option("type",			required_argument,	't'),
	    Option("disable-used-space",	no_argument),
	    Option("all-configs",		no_argument,		'a'),
	    Option("columns",			required_argument)
	};

	ParsedOpts opts = get_opts.parse("list", options);

	ListMode list_mode = ListMode::ALL;
	bool show_used_space = true;
	vector<Column> columns;

	ParsedOpts::const_iterator opt;

	if ((opt = opts.find("type")) != opts.end())
	{
	    if (!toValue(opt->second, list_mode, false))
	    {
		string error = sformat(_("Unknown type '%s'."), opt->second.c_str()) + '\n' +
		    possible_enum_values<ListMode>();
		SN_THROW(OptionsException(error));
	    }
	}

	if ((opt = opts.find("disable-used-space")) != opts.end())
	{
	    show_used_space = false;
	}

	if ((opt = opts.find("columns")) != opts.end())
	{
	    columns = parse_columns<Column>(opt->second);
	}
	else
	{
	    columns = default_columns(list_mode, global_options.output_format());
	}

	if (!show_used_space)
	{
	    columns.erase(remove(columns.begin(), columns.end(), Column::USED_SPACE), columns.end());
	}

	if (get_opts.has_args())
	{
	    SN_THROW(OptionsException(_("Command 'list' does not take arguments.")));
	}

	if ((opt = opts.find("all-configs")) == opts.end())
	{
	    output(global_options, columns, { snappers->getSnapper(global_options.config()) }, list_mode);
	}
	else
	{
	    vector<const ProxySnapper*> tmp;

	    for (map<string, ProxyConfig>::value_type it : snappers->getConfigs())
		tmp.push_back(snappers->getSnapper(it.first));

	    output(global_options, columns, tmp, list_mode);
	}
    }


    template <> struct EnumInfo<Column> { static const vector<string> names; };

    const vector<string> EnumInfo<Column>::names({
	"config", "subvolume", "number", "default", "active", "type", "date", "user", "used-space", "cleanup",
	"description", "userdata", "pre-number", "post-number", "post-date", "read-only"
    });


    template <> struct EnumInfo<ListMode> { static const vector<string> names; };

    const vector<string> EnumInfo<ListMode>::names({ "all", "single", "pre-post" });

}
