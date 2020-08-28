/*
 * Copyright (c) [2011-2015] Novell, Inc.
 * Copyright (c) [2016-2020] SUSE LLC
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

#include "utils/text.h"
#include "GlobalOptions.h"
#include "proxy.h"
#include "Command/GetConfig.h"


namespace snapper
{

    using namespace std;


    void
    help_get_config()
    {
	cout << cli::Command::GetConfig::help() << endl;
    }


    void
    command_get_config(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers* snappers, ProxySnapper*)
    {
	cli::Command::GetConfig command(global_options, get_opts, *snappers);

	command.run();
    }

}
