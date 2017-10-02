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


#ifndef SNAPPER_ZFS_H
#define SNAPPER_ZFS_H


#include "snapper/Filesystem.h"
#include "snapper/ZfsUtils.h"


namespace snapper
{

    using namespace ZfsUtils;

    enum ZfsMountType
    {
        SYMLINK, BIND, LEGACY
    };

    class Zfs : public Filesystem
    {
    public:

        static Filesystem* create (const string& fstype, const string& subvolume,
                                   const string& root_prefix);

        Zfs (const string& subvolume, const string& root_prefix);

        virtual void evalConfigInfo (const ConfigInfo& config_info);

        virtual string
        fstype () const
        {
          return "zfs";
        }

        virtual void createConfig () const;
        virtual void deleteConfig () const;


        virtual string snapshotName (unsigned int num) const;
        virtual string snapshotDir (unsigned int num) const;


        virtual SDir openInfosDir () const;
        virtual SDir openSnapshotDir (unsigned int num) const;

        virtual void createSnapshot (unsigned int num, unsigned int num_parent,
                                     bool read_only, bool quota) const;
        virtual void deleteSnapshot (unsigned int num) const;

        virtual bool isSnapshotMounted (unsigned int num) const;
        virtual void mountSnapshot (unsigned int num) const;
        virtual void umountSnapshot (unsigned int num) const;

        virtual bool isSnapshotReadOnly (unsigned int num) const;

        virtual bool checkSnapshot (unsigned int num) const;

        virtual void sync () const;

    private:

        string zfs_subvol_fs;
        string zfs_snapshot_fs;
        string zfs_zpool;

        ZfsMountType mount_type = SYMLINK;
    };
}


#endif
