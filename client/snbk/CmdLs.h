/*
 * Copyright (c) [2004-2015] Novell, Inc.
 * Copyright (c) [2016-2025] SUSE LLC
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


#ifndef SNAPPER_CMD_LS_H
#define SNAPPER_CMD_LS_H


#include "Shell.h"


namespace snapper
{
    using std::string;
    using std::vector;


    /**
     * A sequence of file names found in a pathname.
     */
    class CmdLs
    {
    public:

	CmdLs(const string& ls_bin, const Shell& shell, const string& path);

	typedef vector<string>::const_iterator const_iterator;

	const_iterator begin() const { return entries.begin(); }
	const_iterator end() const { return entries.end(); }

	friend std::ostream& operator<<(std::ostream& s, const CmdLs& cmd_ls);

    private:

	void parse(const vector<string>& lines);

	const string path;

	vector<string> entries;

    };

}

#endif
