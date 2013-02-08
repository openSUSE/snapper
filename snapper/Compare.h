/*
 * Copyright (c) [2011-2012] Novell, Inc.
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


#ifndef SNAPPER_COMPARE_H
#define SNAPPER_COMPARE_H


#include <string>
#include <vector>
#include <functional>

#include "snapper/FileUtils.h"


namespace snapper
{
    using std::string;


    typedef std::function<void(const string& name, unsigned int status)> cmpdirs_cb_t;


    /* Compares the two files. */
    unsigned int
    cmpFiles(const SFile& file1, const SFile& file2);

    /* Compares the two directories. All file-operations use the openat
       et.al. functions. */
    void
    cmpDirs(const SDir& dir1, const SDir& dir2, cmpdirs_cb_t cb);

}


#endif
