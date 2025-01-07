/*
 * Copyright (c) [2019-2024] SUSE LLC
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

#ifndef SNAPPER_GLOBAL_OPTIONS_H
#define SNAPPER_GLOBAL_OPTIONS_H


#include <string>
#include <boost/optional.hpp>

#include <snapper/Enum.h>

#include "client/utils/GetOpts.h"
#include "client/utils/Table.h"
#include "BackupConfig.h"


namespace snapper
{

    class GlobalOptions
    {

    public:

	enum class OutputFormat { TABLE, CSV, JSON };

	static void help_global_options();

	GlobalOptions(GetOpts& get_opts);

	bool quiet() const { return _quiet; }
	bool verbose() const { return _verbose; }
	bool debug() const { return _debug; }
	const boost::optional<BackupConfig::TargetMode>& target_mode() { return _target_mode; }
	bool automatic() const { return _automatic; }
	bool utc() const { return _utc; }
	bool iso() const { return _iso; }
	bool no_dbus() const { return _no_dbus; }
	bool version() const { return _version; }
	bool help() const { return _help; }
	Style table_style() const { return _table_style; }
	OutputFormat output_format() const { return _output_format; }
	const string& separator() const { return _separator; }
	bool headers() const { return _headers; }
	const boost::optional<string>& backup_config() const { return _backup_config; }

    private:

	void check_options(const ParsedOpts& parsed_opts) const;

	boost::optional<BackupConfig::TargetMode> only_target_mode_value(const ParsedOpts& parsed_opts) const;
	Style table_style_value(const ParsedOpts& parsed_opts) const;
	OutputFormat output_format_value(const ParsedOpts& parsed_opts) const;
	string separator_value(const ParsedOpts& parsed_opts) const;
	boost::optional<string> backup_config_value(const ParsedOpts& parsed_opts) const;

	bool _quiet;
	bool _verbose;
	bool _debug;
	boost::optional<BackupConfig::TargetMode> _target_mode;
	bool _automatic;
	bool _utc;
	bool _iso;
	bool _no_dbus;
	bool _version;
	bool _help;
	Style _table_style;
	OutputFormat _output_format;
	string _separator;
	bool _headers;
	boost::optional<string> _backup_config;

    };


    template <> struct EnumInfo<GlobalOptions::OutputFormat> { static const vector<string> names; };

}

#endif
