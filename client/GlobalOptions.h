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

#ifndef SNAPPER_CLI_GLOBAL_OPTIONS_H
#define SNAPPER_CLI_GLOBAL_OPTIONS_H

#include <string>

#include "client/Options.h"
#include "client/utils/GetOpts.h"
#include "client/utils/Table.h"

namespace snapper
{
    namespace cli
    {

	class GlobalOptions : public Options
	{

	public:

	    enum OutputFormat { TABLE, CSV, JSON };

	    static std::string help_text();

	    GlobalOptions(GetOpts& parser);

	    virtual std::vector<std::string> errors() const override;

	    bool quiet() const
	    {
		return _quiet;
	    }

	    bool verbose() const
	    {
		return _verbose;
	    }

	    bool utc() const
	    {
		return _utc;
	    }

	    bool iso() const
	    {
		return _iso;
	    }

	    bool no_dbus() const
	    {
		return _no_dbus;
	    }

	    bool version() const
	    {
		return _version;
	    }

	    bool help() const
	    {
		return _help;
	    }

	    TableLineStyle table_style() const
	    {
		return _table_style;
	    }

	    OutputFormat output_format() const
	    {
		return _output_format;
	    }

	    string separator() const
	    {
		return _separator;
	    }

	    string config() const
	    {
		return _config;
	    }


	    string root() const
	    {
		return _root;
	    }

	private:

	    void parse_options();

	    TableLineStyle table_style_value() const;

	    OutputFormat output_format_value() const;

	    std::string machine_readable_value() const;

	    std::string separator_value() const;

	    std::string config_value() const;

	    std::string root_value() const;

	    unsigned int table_style_raw() const;

	    bool wrong_table_style() const;

	    bool wrong_machine_readable() const;

	    bool missing_no_dbus() const;

	    std::string table_style_error() const;

	    std::string machine_readable_error() const;

	    std::string missing_no_dbus_error() const;

	    bool _quiet;

	    bool _verbose;

	    bool _utc;

	    bool _iso;

	    bool _no_dbus;

	    bool _version;

	    bool _help;

	    TableLineStyle _table_style;

	    OutputFormat _output_format;

	    string _separator;

	    string _config;

	    string _root;

	};

    }
}

#endif