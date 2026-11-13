/*
 * Copyright (c) [2011-2015] Novell, Inc.
 * Copyright (c) [2016-2020] SUSE LLC
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


#include <iostream>
#include <boost/algorithm/string.hpp>

#include "../proxy/proxy.h"
#include "../utils/text.h"
#include "GlobalOptions.h"

#include <snapper/AppUtil.h>
#include <snapper/AsciiFile.h>

#include "MyFiles.h"


namespace snapper
{

    void
    MyFiles::bulk_process(FILE* file, GetOpts& get_opts, bool recursive,
			  std::function<void(File& file)> callback)
    {
	if (file)
	{
	    AsciiFileReader asciifile(file, Compression::NONE);

	    string line;
	    while (asciifile.read_line(line))
	    {
		if (line.empty())
		    continue;

		string name = line;

		// strip optional status
		if (name[0] != '/')
		{
		    string::size_type pos = name.find(" ");
		    if (pos == string::npos)
			continue;

		    name.erase(0, pos + 1);
		}

		process(name, recursive, callback);
	    }
	}
	else
	{
	    if (get_opts.num_args() == 0)
	    {
		for (Files::iterator it = begin(); it != end(); ++it)
		    callback(*it);
	    }
	    else
	    {
		while (get_opts.num_args() > 0)
		{
		    string name = get_opts.pop_arg();

		    process(name, recursive, callback);
		}
	    }
	}
    }


    void
    MyFiles::process(const string& name, bool recursive, std::function<void(File& file)> callback)
    {
	if (!recursive)
	{
	    Files::iterator it = findAbsolutePath(name);
	    if (it == end())
	    {
		cerr << sformat(_("File '%s' not found."), name.c_str()) << endl;
		exit(EXIT_FAILURE);
	    }

	    callback(*it);
	    return;
	}

	// Strip trailing slashes so that e.g. "/etc/" matches "/etc" and the
	// files below it.

	string path = name;
	while (path.size() > 1 && boost::ends_with(path, "/"))
	    path.pop_back();

	bool found = false;

	if (!path.empty())
	{
	    const string prefix = path == "/" ? path : path + "/";

	    for (Files::iterator it = begin(); it != end(); ++it)
	    {
		const string absolute_path = it->getAbsolutePath(LOC_SYSTEM);

		if (absolute_path == path || boost::starts_with(absolute_path, prefix))
		{
		    callback(*it);
		    found = true;
		}
	    }
	}

	if (!found)
	{
	    cerr << sformat(_("File '%s' not found."), name.c_str()) << endl;
	    exit(EXIT_FAILURE);
	}
    }

}
