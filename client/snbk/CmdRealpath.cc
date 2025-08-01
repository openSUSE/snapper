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


#include "CmdRealpath.h"

#include "snapper/SystemCmd.h"
#include "snapper/SnapperDefines.h"
#include "snapper/Exception.h"
#include "snapper/LoggerImpl.h"


namespace snapper
{

    CmdRealpath::CmdRealpath(const string& realpath_bin, const Shell& shell, const string& path)
	: path(path)
    {
	SystemCmd::Args cmd_args = { realpath_bin, "--canonicalize-existing", "--", path };
	SystemCmd cmd(shellify(shell, cmd_args));

	if (cmd.retcode() != 0)
	{
	    y2err("command '" << cmd.cmd() << "' failed: " << cmd.retcode());
	    for (const string& tmp : cmd.get_stdout())
		y2err(tmp);
	    for (const string& tmp : cmd.get_stderr())
		y2err(tmp);

	    SN_THROW(Exception("'realpath' failed"));
	}

	parse(cmd.get_stdout());
    }


    void
    CmdRealpath::parse(const vector<string>& lines)
    {
	if (lines.size() != 1)
	    SN_THROW(Exception("failed to parse output of 'realpath'"));

	realpath = lines[0];

	y2mil(*this);
    }


    std::ostream&
    operator<<(std::ostream& s, const CmdRealpath& cmd_realpath)
    {
	s << "path:" << cmd_realpath.path << " realpath:" << cmd_realpath.realpath;

	return s;
    }

}
