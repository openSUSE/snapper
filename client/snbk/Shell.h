/*
 * Copyright (c) 2024 SUSE LLC
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


#ifndef SNAPPER_SHELL_H
#define SNAPPER_SHELL_H


#include <string>
#include <vector>

#include "snapper/SystemCmd.h"


namespace snapper
{

    using std::string;
    using std::vector;


    struct Shell
    {
	enum class Mode
	{
	    DIRECT, SSH
	};

	Mode mode = Mode::DIRECT;
	vector<string> ssh_options;
    };


    SystemCmd::Args
    shellify(const Shell& shell, const SystemCmd::Args& args);


    SystemCmd::Args
    shellify_pipe(const SystemCmd::Args& args1, const Shell& shell2, const SystemCmd::Args& args2);

}

#endif
