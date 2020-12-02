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


#include "config.h"

#include <assert.h>
#include <fcntl.h>

#include "snapper/Regex.h"
#include "snapper/Snapper.h"
#include "snapper/SystemCmd.h"
#include "snapper/ZfsUtils.h"


namespace snapper
{
    namespace ZfsUtils
    {
        void
        create_volume(const string& name, const bool exist_ok)
        {
            SystemCmd cmd(ZFSBIN " create " + string(exist_ok ? "-p " : "") + " " + quote(name));

            if (cmd.retcode() != 0)
                SN_THROW(ZfsCallException("Creating ZFS subvol failed"));
        }

        void
        delete_volume(const string& name)
        {
            if (!volume_exists(name))
                return;

            SystemCmd cmd(ZFSBIN " destroy " + quote(name));

            if (cmd.retcode() != 0)
                SN_THROW(ZfsCallException("Deleting ZFS vol failed"));
        }

        bool
        volume_exists(const string& name)
        {
            SystemCmd cmd(ZFSBIN " get -H -d 1 -o name name " + name);

            if (cmd.retcode() != 0)
                return false;

            return true;
        }

        /*
         * Create a ZFS snapshot.
         *
         * @throws ZfsCallException if creation fails.
         */
        void
        create_snapshot(const string& fs, const string& name)
        {
            SystemCmd cmd(ZFSBIN " snapshot " + quote(fs + "@" + name));

            if (cmd.retcode() != 0)
                SN_THROW(ZfsCallException("Creating ZFS snapshot failed"));
        }

        /*
         * Delete a ZFS snapshot.
         *
         * @throws ZfsCallException if deleting fails.
         */
        void
        delete_snapshot(const string& fs, const string& name)
        {
            delete_volume(fs + "@" + name);
        }

        bool
        snapshot_exists(const string& fs, const string& name)
        {
            return volume_exists(quote(fs + "@" + name));
        }

        /* Extract component names from volume name.
         *
         * The volume name must be of the form pool[/dataset][@snapshot].
         *
         * @returns a std::tuple with the pool name, dataset name and snapshot name
         */
        tuple<string, string, string>
        extract_components(const string& name)
        {
            string pool, dataset, snapshot;

            string::size_type first_slash = name.find_first_of('/');
            string::size_type first_at = name.find_first_of('@');

            pool = string(name, 0, first_slash);

            if (first_slash != string::npos)
                dataset = string(name, first_slash + 1, first_at - first_slash - 1);

            if (first_at != string::npos)
                snapshot = string(name, first_at + 1);

            return std::make_tuple(pool, dataset, snapshot);
        }
    }
}
