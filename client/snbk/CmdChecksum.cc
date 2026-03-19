/*
 * Copyright (c) 2026 SUSE LLC
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


#include <boost/algorithm/string.hpp>

#include <snapper/AppUtil.h>
#include <snapper/Exception.h>
#include <snapper/LoggerImpl.h>
#include <snapper/SystemCmd.h>

#include "../utils/text.h"

#include "CmdChecksum.h"


namespace snapper
{

    CmdChecksum::CmdChecksum(const Shell& shell, const string& chksum_bin, const string& path)
	: path(path)
    {
	SystemCmd::Args cmd_args = { chksum_bin, "--", path };
	SystemCmd cmd(shellify(shell, cmd_args));

	if (cmd.retcode() != 0)
	{
	    y2err("command '" << cmd.cmd() << "' failed: " << cmd.retcode());
	    for (const string& tmp : cmd.get_stdout())
		y2err(tmp);
	    for (const string& tmp : cmd.get_stderr())
		y2err(tmp);

	    SN_THROW(Exception(_("Failed to compute checksum.")));
	}

	parse(cmd.get_stdout());

	y2mil(*this);
    }


    void
    CmdChecksum::parse(const vector<string>& lines)
    {
	if (lines.size() != 1)
	    SN_THROW(Exception(_("Invalid number of lines in checksum output.")));

	vector<string> parts;
	boost::split(parts, lines[0], boost::is_any_of(" "), boost::token_compress_on);
	if (parts.size() != 2)
	{
	    y2err("Invalid checksum line: " << lines[0]);
	    SN_THROW(Exception(_("Invalid checksum output format.")));
	}

	checksum = parts[0];
    }


    std::ostream&
    operator<<(std::ostream& s, const CmdChecksum& cmd_checksum)
    {
	s << "path: " << cmd_checksum.path << " checksum: " << cmd_checksum.checksum << '\n';

	return s;
    }


} // namespace snapper
