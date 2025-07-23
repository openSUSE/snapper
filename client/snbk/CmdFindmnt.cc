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


#include "CmdFindmnt.h"
#include "JsonFile.h"

#include "snapper/SystemCmd.h"
#include "snapper/SnapperDefines.h"
#include "snapper/Exception.h"
#include "snapper/LoggerImpl.h"


namespace snapper
{

    CmdFindmnt::CmdFindmnt(const string& findmnt_bin, const Shell& shell, const string& path)
	: path(path)
    {
	SystemCmd::Args cmd_args = { findmnt_bin, "--json", "--target", path };
	SystemCmd cmd(shellify(shell, cmd_args));

	if (cmd.retcode() != 0)
	{
	    y2err("command '" << cmd.cmd() << "' failed: " << cmd.retcode());
	    for (const string& tmp : cmd.get_stdout())
		y2err(tmp);
	    for (const string& tmp : cmd.get_stderr())
		y2err(tmp);

	    SN_THROW(Exception("'findmnt' failed"));
	}

	parse_json(cmd.get_stdout());
    }


    void
    CmdFindmnt::parse_json(const vector<string>& lines)
    {
	JsonFile json_file(lines);

	vector<json_object*> tmp1;

	if (!get_child_nodes(json_file.get_root(), "filesystems", tmp1))
	    SN_THROW(Exception("\"filesystems\" not found in json output of 'findmnt'"));

	for (json_object* tmp2 : tmp1)
	{
	    if (!get_child_value(tmp2, "source", source))
		SN_THROW(Exception("\"source\" not found or invalid"));

	    if (!get_child_value(tmp2, "target", target))
		SN_THROW(Exception("\"target\" not found or invalid"));
	}

	y2mil(*this);
    }


    std::ostream&
    operator<<(std::ostream& s, const CmdFindmnt& cmd_findmnt)
    {
	s << "path:" << cmd_findmnt.path << " source:" << cmd_findmnt.source
	  << " target:" << cmd_findmnt.target;

	return s;
    }

}
