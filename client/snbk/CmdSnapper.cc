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


#include "snapper/AppUtil.h"
#include "snapper/SystemCmd.h"
#include "snapper/Exception.h"
#include "snapper/LoggerImpl.h"
#include "CmdSnapper.h"
#include "JsonFile.h"

#include "../utils/text.h"


namespace snapper
{

    using namespace std;


    CmdSnapperGetConfig::CmdSnapperGetConfig(const string& snapper_bin, const Shell& shell, const string& config)
	: config(config)
    {
	SystemCmd::Args cmd_args = { snapper_bin, "--json", "--config", config, "get-config" };
	SystemCmd cmd(shellify(shell, cmd_args));

	if (cmd.retcode() != 0)
	{
	    y2err("command '" << cmd.cmd() << "' failed: " << cmd.retcode());
	    for (const string& tmp : cmd.get_stdout())
		y2err(tmp);
	    for (const string& tmp : cmd.get_stderr())
		y2err(tmp);

	    SN_THROW(Exception("'snapper get-config' failed"));
	}

	parse(cmd.get_stdout());
    }


    void
    CmdSnapperGetConfig::parse(const vector<string>& lines)
    {
	JsonFile json_file(lines);

	if (!get_child_value(json_file.get_root(), "SUBVOLUME", subvolume))
	    SN_THROW(Exception("\"SUBVOLUME\" not found in json output of 'snapper get-config'"));

	y2mil(*this);
    }


    std::ostream&
    operator<<(std::ostream& s, const CmdSnapperGetConfig& cmd_snapper_get_config)
    {
	s << "config:" << cmd_snapper_get_config.config << " subvolume:"
	  << cmd_snapper_get_config.subvolume << '\n';

	return s;
    }


    CmdSnapperList::CmdSnapperList(const string& snapper_bin, const Shell& shell, const string& config)
	: config(config)
    {
	SystemCmd::Args cmd_args = { snapper_bin, "--json", "--utc", "--config", config, "list",
	    "--disable-used-space" };
	SystemCmd cmd(shellify(shell, cmd_args));

	if (cmd.retcode() != 0)
	{
	    y2err("command '" << cmd.cmd() << "' failed: " << cmd.retcode());
	    for (const string& tmp : cmd.get_stdout())
		y2err(tmp);
	    for (const string& tmp : cmd.get_stderr())
		y2err(tmp);

	    SN_THROW(Exception("'snapper list' failed"));
	}

	parse(cmd.get_stdout());
    }


    void
    CmdSnapperList::parse(const vector<string>& lines)
    {
	JsonFile json_file(lines);

	vector<json_object*> tmp1;
	if (!get_child_nodes(json_file.get_root(), config.c_str(), tmp1))
	    SN_THROW(Exception("\"<config>\" not found in json output of 'snapper list'"));

	for (json_object* tmp2 : tmp1)
	{
	    Entry entry;

	    if (!get_child_value(tmp2, "number", entry.number))
		SN_THROW(Exception("\"number\" not found in json output of 'snapper list'"));

	    if (entry.number != 0)
	    {
		string tmp;
		if (!get_child_value(tmp2, "date", tmp))
		    SN_THROW(Exception("\"date\" not found in json output of 'snapper list'"));

		if ((entry.date = scan_datetime(tmp, true)) == (time_t)(-1))
		    SN_THROW(Exception(_("Failed to parse the date in json output of 'snapper list'")));
	    }

	    entries.push_back(entry);
	}

	y2mil(*this);
    }


    std::ostream&
    operator<<(std::ostream& s, const CmdSnapperList& cmd_snapper_list)
    {
	for (const CmdSnapperList::Entry& entry : cmd_snapper_list.entries)
	    s << entry.number << ' ' << datetime(entry.date, true, true) << '\n';

	return s;
    }

}
