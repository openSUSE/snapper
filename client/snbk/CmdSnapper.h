/*
 * Copyright (c) [2017-2026] SUSE LLC
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


#ifndef SNAPPER_CMD_SNAPPER_H
#define SNAPPER_CMD_SNAPPER_H


#include "Shell.h"


namespace snapper
{

    using std::string;
    using std::vector;


    class CmdSnapperGetConfig
    {
    public:

	CmdSnapperGetConfig(const string& snapper_bin, const Shell& shell, const string& config);

	const string& get_subvolume() const { return subvolume; }

	friend std::ostream& operator<<(std::ostream& s, const CmdSnapperGetConfig& cmd_snapper_get_config);

    private:

	void parse(const vector<string>& lines);

	const string config;

	string subvolume;

    };


    class CmdSnapperList
    {
    public:

	struct Entry
	{
	    unsigned int number = 0;
	    time_t date = (time_t)(-1);
	};

	CmdSnapperList(const string& snapper_bin, const Shell& shell, const string& config);

	typedef vector<Entry>::const_iterator const_iterator;

	const_iterator begin() const { return entries.begin(); }
	const_iterator end() const { return entries.end(); }

	friend std::ostream& operator<<(std::ostream& s, const CmdSnapperList& cmd_snapper_list);

    private:

	void parse(const vector<string>& lines);

	const string config;

	vector<Entry> entries;

    };

}

#endif
