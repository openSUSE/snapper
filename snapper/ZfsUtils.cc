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


#include "config.h"

#include <assert.h>
#include <fcntl.h>

#include "snapper/Log.h"
#include "snapper/ZfsUtils.h"
#include "snapper/SystemCmd.h"
#include "snapper/Snapper.h"
#include "snapper/AppUtil.h"


namespace snapper
{
    namespace ZfsUtils
    {


        void
        create_volume(const string& mountpoint, const string& volume)
        {

            SystemCmd cmd(ZFSBIN " create -o mountpoint=" +
                          quote(mountpoint) + " " +
                          quote(volume));

            if (cmd.retcode() != 0)
                SN_THROW(ZfsCallException("Making ZFS subvol failed"));
        }


        void
        create_snapshot(const string& fs, const string& name)
        {
            SystemCmd cmd(ZFSBIN " snapshot " + quote(fs + "@" + name));

            if (cmd.retcode() != 0)
                SN_THROW(ZfsCallException("Creating snapshot failed"));
        }


        void
        delete_volume(const string& name)
        {
            SystemCmd cmd(ZFSBIN " destroy " + quote(name));

            if (cmd.retcode() != 0)
                SN_THROW(ZfsCallException("Deleting ZFS vol failed"));
        }


        uint_fast64_t
        get_freeing(const string& pool) {
            // TODO (for sync function)            
        }


        bool
        is_mounted(const string& name)
        {
            SystemCmd cmd(ZFSBIN " list -H -t filesystem -o mounted " +
                          name);
            if (cmd.retcode() != 0)
            {
                SN_THROW(ZfsCallException("Error getting mount status of " + quote(name)));
            }

            string output = cmd.stdout()[0];
            return output == "yes";
        }


        string
        get_mountpoint(const string& name)
        {
            SystemCmd cmd(ZFSBIN " list -H -t filesystem -o mountpoint " +
                          name);
            if (cmd.retcode() != 0)
            {
                SN_THROW(ZfsCallException("Error getting mountpoint of " + quote(name)));
            }

            return cmd.stdout()[0];
        }


        bool
        mount_vol(const string& name)
        {
            // returns true on success, false if already mounted, and 
            // throws exception for all other problems
            SystemCmd cmd(ZFSBIN " mount " + quote(name));
            if (cmd.retcode() == 0) return true;
            if (cmd.stderr().empty())
                SN_THROW(ZfsCallException("Error mounting " + quote(name)));


            string str = cmd.stderr()[0];
            string check = "filesystem already mounted";
            if (str.size() >= check.size() &&
                str.compare(str.size() - check.size(), check.size(), check) == 0)
                return true;


            SN_THROW(ZfsCallException("Error mounting " + quote(name)));
            return false;
        }


        SDir
        follow_symlink(const string& base_path)
        {
            string bpath = base_path;
            return SDir(realpath(bpath));
        }


        SDir
        follow_symlink(const SDir& dir, const string& name)
        {
            string bpath = dir.fullname() + "/" + name;
            return SDir(realpath(bpath));
        }


        string
        poolname(const string& name)
        {
            string::size_type pos = name.find_first_of('/');
            string root;
            if (pos == string::npos)
                root = name;
            root = string(name, 0, pos);
            pos = root.find_first_of('@');
            if (pos == string::npos)
                return root;
            return string(root, 0, pos);
        }

    }
}
