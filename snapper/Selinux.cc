/*
 * Copyright (c) [2016] Red Hat, Inc.
 * Copyright (c) 2023 SUSE LLC
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

#include <cerrno>
#include <map>
#include <boost/algorithm/string.hpp>

#include "snapper/AppUtil.h"
#include "snapper/AsciiFile.h"
#include "snapper/LoggerImpl.h"
#include "snapper/Selinux.h"

namespace snapper
{

    using std::map;


    SnapperContexts::SnapperContexts()
    {
	map<string, string> snapperd_contexts;

	try
	{
	    AsciiFileReader ascii_file_reader(selinux_snapperd_contexts_path(), Compression::NONE);

	    string line;
	    while (ascii_file_reader.read_line(line))
	    {
		// commented line
		if (line[0] == '#')
		    continue;

		// do not parse lines w/o '=' symbol
		string::size_type pos = line.find('=');
		if (pos == string::npos)
		    continue;

		if (!snapperd_contexts.insert(make_pair(boost::trim_copy(line.substr(0, pos)),
							boost::trim_copy(line.substr(pos + 1)))).second)
		{
		    SN_THROW(SelinuxException("Duplicate key in contexts file"));
		}
	    }
	}
	catch (const Exception& e)
	{
	    SN_CAUGHT(e);
	    SN_THROW(SelinuxException("Failed to parse contexts file"));
	}

	map<string, string>::const_iterator cit = snapperd_contexts.find(selinux_snapperd_data);
	if (cit == snapperd_contexts.end())
	{
	    SN_THROW(SelinuxException("Snapperd data context not found"));
	}

	subvolume_ctx = context_new(cit->second.c_str());
	if (!subvolume_ctx)
	{
	    SN_THROW(SelinuxException());
	}
    }


    DefaultSelinuxFileContext::DefaultSelinuxFileContext(const char* context)
    {
	if (setfscreatecon(context) < 0)
	{
	    SN_THROW(SelinuxException(string("setfscreatecon(") + context + ") failed"));
	}
    }


    DefaultSelinuxFileContext::~DefaultSelinuxFileContext()
    {
	if (setfscreatecon(NULL))
	    y2err("Failed to reset default file system objects context");
    }


    SelinuxLabelHandle::SelinuxLabelHandle()
	: handle(selabel_open(SELABEL_CTX_FILE, NULL, 0))
    {
	if (!handle)
	{
	    SN_THROW(SelinuxException("Failed to open SELinux labeling handle: " + stringerror(errno)));
	}
    }


    char*
    SelinuxLabelHandle::selabel_lookup(const string& path, int mode)
    {
	char *con;

	if (!::selabel_lookup(handle, &con, path.c_str(), mode))
	{
	    y2deb("found label for path " << path << ": " << con);
	    return con;
	}
	else
	{
	    if (errno == ENOENT)
		y2deb("SELinux context not defined for path " << path);

	    return NULL;
	}
    }


    bool
    _is_selinux_enabled()
    {
	static bool selinux_enabled = false;
	static bool selinux_checked = false;

	if (!selinux_checked)
	{
	    selinux_enabled = (is_selinux_enabled() == 1); // may return -1 on error
	    selinux_checked = true;
	    y2mil("SELinux support " << (selinux_enabled ? "enabled" : "disabled"));
	}

	return selinux_enabled;
    }


    SelinuxLabelHandle*
    SelinuxLabelHandle::get_selinux_handle()
    {
	static SelinuxLabelHandle handle;

	return &handle;
    }

}
