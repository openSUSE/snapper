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
#include <snapper/SnapperDefines.h>
#include <snapper/SystemCmd.h>

#include "../utils/text.h"

#include "CmdMetaHash.h"


namespace snapper
{

    CmdMetaHash::CmdMetaHash(const Shell& shell, const string& chksum_bin,
                             const string& sh_bin, const string& path)
        : path(path)
    {
	SystemCmd::Args cmd_args = {
	    sh_bin, "-c",
	    sformat("for d in $(ls -1 %s); do %s %s/$d/info.xml; done", path.c_str(),
	            chksum_bin.c_str(), path.c_str())
	};
	SystemCmd cmd(shellify(shell, cmd_args));

	if (cmd.retcode() != 0)
	{
	    y2err("command '" << cmd.cmd() << "' failed: " << cmd.retcode());
	    for (const string& tmp : cmd.get_stdout())
		y2err(tmp);
	    for (const string& tmp : cmd.get_stderr())
		y2err(tmp);

	    SN_THROW(Exception(_("Hashing for snapshot metadata failed.")));
	}

	parse(cmd.get_stdout());

	y2mil(*this);
    }


    const string& CmdMetaHash::get_hash(unsigned int num) const
    {
	auto pair = lookup.find(num);
	if (pair == lookup.end())
	{
	    string error = sformat(_("Meta hash of snapshot %d not found."), num);
	    SN_THROW(Exception(error));
	}

	return pair->second;
    }


    void CmdMetaHash::parse(const vector<string>& lines)
    {
	for (const string& line : lines)
	{
	    // Extract the hash and the path
	    vector<string> parts;
	    boost::split(parts, line, boost::is_any_of(" "), boost::token_compress_on);
	    if (parts.size() != 2)
	    {
		y2err("Invalid hash string: " << line);
		SN_THROW(Exception(_("Invalid hash output format.")));
	    }

	    const string& hash = parts.front();
	    const string& path = parts.back();

	    // Split the path into components
	    vector<string> comps;
	    boost::split(comps, path, boost::is_any_of("/"), boost::token_compress_on);
	    if (comps.size() < 2)
	    {
		SN_THROW(Exception(_("Unexpected path format.")));
	    }

	    // Convert the snapshot number (second‑to‑last component)
	    unsigned int num = stoi(comps[comps.size() - 2]);

	    // Store the hash
	    lookup[num] = hash;
	}
    }


    std::ostream& operator<<(std::ostream& s, const CmdMetaHash& cmd_metahash)
    {
	s << "path: " << cmd_metahash.path << '\n';
	for (const auto& [num, hash] : cmd_metahash.lookup)
	{
	    s << sformat("  num: %d, hash: %s\n", num, hash.c_str());
	}

	return s;
    }


} // namespace snapper
