/*
 * Copyright (c) [2019-2023] SUSE LLC
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

#include <snapper/Enum.h>

#include "client/utils/GetOpts.h"
#include "client/utils/Table.h"


namespace snapper
{

    class GlobalOptions
    {

    public:

	enum class OutputFormat { TABLE, CSV, JSON };
	enum class Ambit { AUTO, CLASSIC, TRANSACTIONAL };

	static void help_global_options();

	GlobalOptions(GetOpts& get_opts);

	bool quiet() const { return _quiet; }
	bool verbose() const { return _verbose; }
	bool debug() const { return _debug; }
	bool utc() const { return _utc; }
	bool iso() const { return _iso; }
	bool no_dbus() const { return _no_dbus; }
	bool version() const { return _version; }
	bool help() const { return _help; }
	Style table_style() const { return _table_style; }
	bool abbreviate() const { return _abbreviate; }
	OutputFormat output_format() const { return _output_format; }
	const string& separator() const { return _separator; }
	bool headers() const { return _headers; }
	const string& config() const { return _config; }
	const string& root() const { return _root; }
	Ambit ambit() const { return _ambit; }

	void set_ambit(Ambit ambit) { _ambit = ambit; }

    private:

	void check_options(const ParsedOpts& parsed_opts) const;

	Style table_style_value(const ParsedOpts& parsed_opts) const;
	OutputFormat output_format_value(const ParsedOpts& parsed_opts) const;
	string separator_value(const ParsedOpts& parsed_opts) const;
	string config_value(const ParsedOpts& parsed_opts) const;
	string root_value(const ParsedOpts& parsed_opts) const;
	Ambit ambit_value(const ParsedOpts& parsed_opts) const;

	bool _quiet;
	bool _verbose;
	bool _debug;
	bool _utc;
	bool _iso;
	bool _no_dbus;
	bool _version;
	bool _help;
	Style _table_style;
	bool _abbreviate;
	OutputFormat _output_format;
	string _separator;
	bool _headers;
	string _config;
	string _root;
	Ambit _ambit;

    };


    template <> struct EnumInfo<GlobalOptions::OutputFormat> { static const vector<string> names; };

    template <> struct EnumInfo<GlobalOptions::Ambit> { static const vector<string> names; };

}

#endif
