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


#include <functional>

#include <snapper/File.h>

#include "../utils/GetOpts.h"


namespace snapper
{

    struct MyFiles : public Files
    {

	MyFiles(const Files& files) : Files(files) {}

	/**
	 * Calls callback for the files given in file or as arguments, or for all
	 * files if neither is given. If recursive is true, callback is also called
	 * for all files below the given paths.
	 */
	void bulk_process(FILE* file, GetOpts& get_opts, bool recursive,
			  std::function<void(File& file)> callback);

    private:

	void process(const string& name, bool recursive, std::function<void(File& file)> callback);

    };

}
