/*
 * Copyright (c) [2016] Red Hat, Inc.
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
#include "snapper/Log.h"
#include "snapper/Selinux.h"

namespace snapper
{

    SnapperContexts::SnapperContexts()
	: subvolume_ctx(NULL)
    {
	std::map<string,string> snapperd_contexts;

	try
	{
	    AsciiFileReader asciifile(selinux_snapperd_contexts_path());

	    string line;
	    while (asciifile.getline(line))
	    {
		// commented line
		if (line[0] == '#')
		    continue;

		// do not parse lines w/o '=' symbol
		string::size_type pos = line.find('=');
		if (pos == string::npos)
		    continue;

		if (!snapperd_contexts.insert(make_pair(boost::trim_copy(line.substr(0, pos)), boost::trim_copy(line.substr(pos + 1)))).second)
		{
		    SN_THROW(SelinuxException("Duplicate key in contexts file"));
		}
	    }
	}
	catch (const FileNotFoundException& e)
	{
	    SN_CAUGHT(e);
	    SN_THROW(SelinuxException("Failed to parse contexts file"));
	}

	std::map<string,string>::const_iterator cit = snapperd_contexts.find(selinux_snapperd_data);
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


    DefaultSelinuxFileContext::DefaultSelinuxFileContext(char* context)
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
		y2deb("Selinux context not defined for path " << path);

	    return NULL;
	}
    }


    int
    _is_selinux_enabled()
    {
	static int selinux_checked = 0, selinux_enabled = 0;

	if (!selinux_checked)
	{
	    selinux_enabled = is_selinux_enabled();
	    selinux_checked = 1;
	    y2mil("Selinux support " << (selinux_enabled ? "en" : "dis") << "abled");
	}

	return selinux_enabled;
    }


    SelinuxLabelHandle*
    SelinuxLabelHandle::get_selinux_handle()
    {
	if (_is_selinux_enabled())
	{
	    static SelinuxLabelHandle handle;
	    return &handle;
	}

	return NULL;
    }

}
