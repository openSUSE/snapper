/*
 * Copyright (c) [2019-2020] SUSE LLC
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

#ifndef SNAPPER_CLI_COMMAND_LIST_SNAPSHOTS_OPTIONS_H
#define SNAPPER_CLI_COMMAND_LIST_SNAPSHOTS_OPTIONS_H

#include "client/Options.h"
#include "client/GlobalOptions.h"
#include "client/Command/ColumnsOption.h"
#include "client/Command/ListSnapshots.h"
#include "client/Command/ListSnapshots/SnappersData.h"

namespace snapper
{
    namespace cli
    {

	class Command::ListSnapshots::Options : public cli::Options
	{

	public:

	    struct Columns
	    {
		static const std::string CONFIG;
		static const std::string SUBVOLUME;
		static const std::string NUMBER;
		static const std::string DEFAULT;
		static const std::string ACTIVE;
		static const std::string TYPE;
		static const std::string DATE;
		static const std::string USER;
		static const std::string USED_SPACE;
		static const std::string CLEANUP;
		static const std::string DESCRIPTION;
		static const std::string USERDATA;
		static const std::string PRE_NUMBER;
		static const std::string POST_NUMBER;
		static const std::string POST_DATE;
	    };

	    static const std::vector<std::string> ALL_COLUMNS;

	    enum class ListMode { ALL, SINGLE, PRE_POST };

	    static std::string help_text();

	    Options(GetOpts& parser);

	    ListMode list_mode() const { return _list_mode; }
	    bool disable_used_space() const { return _disable_used_space; }
	    bool all_configs() const { return _all_configs; }

	    std::vector<std::string> columns(GlobalOptions::OutputFormat format) const;

	private:

	    void parse_options();

	    ListMode list_mode_value() const;

	    string columns_raw() const;

	    std::vector<std::string> list_mode_columns(GlobalOptions::OutputFormat format) const;

	    std::vector<std::string> all_mode_columns(GlobalOptions::OutputFormat format) const;

	    std::vector<std::string> single_mode_columns(GlobalOptions::OutputFormat format) const;

	    std::vector<std::string> pre_post_mode_columns(GlobalOptions::OutputFormat format) const;

	    ListMode _list_mode;

	    bool _disable_used_space;

	    bool _all_configs;

	    Command::ColumnsOption _columns_option;

	};

    }

    template <> struct EnumInfo<cli::Command::ListSnapshots::Options::ListMode> { static const vector<string> names; };

}

#endif
