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

#include <iostream>

#include "client/utils/text.h"
#include "client/Command/GetConfig.h"
#include "client/Command/GetConfig/Options.h"
#include "client/Command/GetConfig/ConfigData/Table.h"
#include "client/Command/GetConfig/ConfigData/Csv.h"
#include "client/Command/GetConfig/ConfigData/Json.h"

using namespace std;

namespace snapper
{
    namespace cli
    {

	string Command::GetConfig::help()
	{
	    return
		string(
		    _("  Get configs:\n"
	 	      "\tsnapper get-config\n"
		      "\n")
		) + Options::help_text();
	}


	Command::GetConfig::GetConfig(const GlobalOptions& global_options,
	    GetOpts& options_parser, ProxySnappers& snappers) :
	    Command(global_options, options_parser, snappers), _options(new Options(options_parser))
	{}


	Command::GetConfig::GetConfig::~GetConfig()
	{}


	const Command::GetConfig::Options& Command::GetConfig::options() const
	{
	    return *_options.get();
	}


	void Command::GetConfig::run()
	{
	    if (_options_parser.has_args())
	    {
		cerr << _("Command 'get-config' does not take arguments.") << endl;
		exit(EXIT_FAILURE);
	    }

	    string output;

	    switch (global_options().output_format())
	    {
		case GlobalOptions::OutputFormat::TABLE:
		{
		    ConfigData::Table table(*this, global_options().table_style());
		    output = table.output();
		}
		break;

		case GlobalOptions::OutputFormat::CSV:
		{
		    ConfigData::Csv csv(*this, global_options().separator());
		    output = csv.output();
		}
		break;

		case GlobalOptions::OutputFormat::JSON:
		{
		    ConfigData::Json json(*this);
		    output = json.output() + "\n";
		}
		break;
	    }

	    cout << output;
	}

    }
}
