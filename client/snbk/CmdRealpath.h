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


#ifndef SNAPPER_CMD_REALPATH_H
#define SNAPPER_CMD_REALPATH_H


#include "Shell.h"


namespace snapper
{

    using std::string;
    using std::vector;


    /**
     * Class to probe realpath: Call "realpath <path>".
     */
    class CmdRealpath
    {
    public:

	CmdRealpath(const Shell& shell, const string& path);

	const string& get_realpath() const { return realpath; }

	friend std::ostream& operator<<(std::ostream& s, const CmdRealpath& cmd_realpath);

    private:

	void parse(const vector<string>& lines);

	const string path;

	string realpath;

    };

}

#endif
