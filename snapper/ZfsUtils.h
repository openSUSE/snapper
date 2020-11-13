/*
 * Copyright (c) [2011-2013] Novell, Inc.
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

#ifndef ZFSUTILS_H
#define ZFSUTILS_H

#include <string>
#include "snapper/Exception.h"
#include "snapper/FileUtils.h"

namespace snapper
{
    using std::string;


    namespace ZfsUtils
    {

        struct ZfsCallException : public Exception
        {

            explicit
            ZfsCallException (const string& msg) : Exception (msg) { }
        };


        void create_volume (const string& mountpoint, const string& volume);
        void create_snapshot (const string& fs, const string& name);
        void delete_volume (const string& name);

        uint_fast64_t get_freeing (const string& pool);


        bool is_mounted (const string& name);
        bool mount_vol (const string& name);

        SDir follow_symlink (const string& base_path);
        SDir follow_symlink (const SDir& dir, const string& name);

        string poolname (const string& name);
    }
}

#endif /* ZFSUTILS_H */

