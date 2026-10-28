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
#include <openssl/sha.h>

#include <snapper/AppUtil.h>
#include <snapper/Enum.h>
#include <snapper/LoggerImpl.h>
#include <snapper/SystemCmd.h>
#include <snapper/XmlFile.h>

#include "CmdMetaParse.h"

#include "../misc.h"


namespace snapper
{
    const vector<string> EnumInfo<SnapshotMeta::State>::names({ "invalid", "valid" });


    CmdMetaParse::CmdMetaParse(const Shell& shell, const string& snapshot_dir,
                               const string& cat_bin)
        : path(snapshot_dir + "/info.xml")
    {
	SystemCmd::Args cmd_args = { cat_bin, "--", path };
	SystemCmd cmd(shellify(shell, cmd_args));

	if (cmd.retcode() != 0)
	{
	    y2err("command '" << cmd.cmd() << "' failed: " << cmd.retcode());
	    for (const string& tmp : cmd.stdout())
		y2err(tmp);
	    for (const string& tmp : cmd.stderr())
		y2err(tmp);

	    SN_THROW(Exception(_("Failed to load info.xml.")));
	}

	if (!cmd.stdout().empty())
	    content = boost::join(cmd.stdout(), "\n") + "\n";

	y2mil(*this);
    }


    string CmdMetaParse::get_checksum() const
    {
	if (!content.length())
	{
	    y2err(sformat("The content of %s is empty.", path.c_str()));
	    SN_THROW(Exception(_("The content of info.xml is empty.")));
	}

	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256(reinterpret_cast<const unsigned char*>(content.c_str()), content.length(), hash);

	char buf[2 * SHA256_DIGEST_LENGTH + 1];

	for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
	    snprintf(&buf[2 * i], 3, "%02x", hash[i]);

	return buf;
    }


    SnapshotMeta CmdMetaParse::get_meta() const
    {
	if (!content.length())
	{
	    y2err(sformat("The content of %s is empty.", path.c_str()));
	    SN_THROW(Exception(_("The content of info.xml is empty.")));
	}

	SnapshotMeta meta;

	XmlFile file(XmlFile::FromString, content);
	const xmlNode* node = file.getRootElement();
	string tmp;

	time_t tmp_date;
	if (!getChildValue(node, "date", tmp) ||
	    (tmp_date = scan_datetime(tmp, true)) == (time_t)(-1))
	{
	    y2err("The date attribute is missing or invalid from " << path);
	    SN_THROW(Exception(_("Failed to parse the date attribute from `info.xml`")));
	}
	else
	{
	    meta.date = tmp_date;
	}

	if (!getChildValue(node, "type", tmp) || !toValue(tmp, meta.type, true))
	{
	    y2err("The type attribute is missing from " << path);
	    SN_THROW(Exception(_("The type attribute is missing from `info.xml`.")));
	}

	getChildValue(node, "pre_num", meta.pre_num);
	getChildValue(node, "cleanup", meta.cleanup);

	for (const xmlNode* tmp_node : getChildNodes(node, "userdata"))
	{
	    string key, value;
	    getChildValue(tmp_node, "key", key);
	    getChildValue(tmp_node, "value", value);
	    if (!key.empty())
		meta.userdata[key] = value;
	}

	meta.state = SnapshotMeta::State::VALID;

	return meta;
    }


    std::ostream& operator<<(std::ostream& s, const CmdMetaParse& cmd_metaparse)
    {
	SnapshotMeta meta = cmd_metaparse.get_meta();

	s << "path: " << cmd_metaparse.path
	  << ", checksum: " << cmd_metaparse.get_checksum()
	  << ", state: " << toString(meta.state) << ", date: " << meta.date
	  << ", type: " << toString(meta.type) << ", pre_num: " << meta.pre_num
	  << ", cleanup: " << meta.cleanup
	  << ", userdata: " << show_userdata(meta.userdata) << '\n';

	return s;
    }


} // namespace snapper
