/*
 * Copyright (c) [2019] SUSE LLC
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
    using std::vector;
    using std::tuple;

    namespace ZfsUtils
    {
        struct ZfsCallException : public Exception
        {
            explicit ZfsCallException(const string& msg) : Exception(msg) {}
        };

        struct ZfsVolumeInfo
        {
            string pool;
            string dataset;
            string snapshot;
        };

        void create_volume(const string& name, const bool exist_ok = false);
        void delete_volume(const string& name);
        bool volume_exists(const string& name);

        void create_snapshot(const string& fs, const string& name);
        void delete_snapshot(const string& fs, const string& name);
        bool snapshot_exists(const string& fs, const string& name);

        tuple<string, string, string> extract_components(const string& name);
    }
}

#endif /* ZFSUTILS_H */
