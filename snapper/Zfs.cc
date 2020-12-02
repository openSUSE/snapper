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

#include <asm/types.h>
#include <boost/algorithm/string.hpp>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "snapper/Log.h"
#include "snapper/Filesystem.h"
#include "snapper/Zfs.h"
#include "snapper/Snapper.h"
#include "snapper/SnapperTmpl.h"
#include "snapper/SystemCmd.h"
#include "snapper/SnapperDefines.h"


namespace snapper
{
    Filesystem*
    Zfs::create(const string& fstype, const string& subvolume, const string& root_prefix)
    {
        if (fstype == "zfs")
            return new Zfs(subvolume, root_prefix);

        return NULL;
    }

    Zfs::Zfs(const string& subvolume, const string& root_prefix) : Filesystem(subvolume, root_prefix)
    {
        if (access(ZFSBIN, X_OK) != 0)
            SN_THROW(ProgramNotInstalledException(ZFSBIN " is not installed"));

        bool found = false;
        MtabData mtab_data;

        if (!getMtabData(subvolume, found, mtab_data))
        {
            y2err("can't get mtab data for " + subvolume);
            SN_THROW(InvalidConfigException());
        }

        if (!found)
        {
            y2err("filesystem not mounted");
            SN_THROW(InvalidConfigException());
        }

        zfs_volume_name = mtab_data.device;
        zfs_snapshot_volume_name = zfs_volume_name + "/.snapshots";
    }

    string
    Zfs::snapshotDir(unsigned int num) const
    {
        return (subvolume == "/" ? "" : subvolume) + "/.snapshots/" + decString(num) + "/snapshot";
    }

    void
    Zfs::createConfig() const
    {
        create_volume(zfs_snapshot_volume_name, true);
    }

    void
    Zfs::deleteConfig() const
    {
        delete_volume(zfs_snapshot_volume_name);
    }

    SDir
    Zfs::openInfosDir() const
    {
        SDir subvolume_dir = openSubvolumeDir();

        if (subvolume_dir.mkdir(".snapshots", 0700) != 0 && errno != EEXIST)
        {
            y2err("mkdir failed errno:" << errno << " (" << strerror(errno) << ")");
            SN_THROW(CreateConfigFailedException("mkdir failed"));
        }

        SDir infos_dir(subvolume_dir, ".snapshots");

        struct stat stat;

        if (infos_dir.stat(&stat) != 0)
        {
            SN_THROW(IOErrorException("failed to stat " + infos_dir.fullname()));
        }

        if (stat.st_uid != 0)
        {
            SN_THROW(IOErrorException(".snapshots must have owner root"));
        }

        if (stat.st_gid != 0 && stat.st_mode & S_IWGRP)
        {
            SN_THROW(IOErrorException(".snapshots must have group root or must not be group-writable"));
        }

        if (stat.st_mode & S_IWOTH)
        {
            SN_THROW(IOErrorException(".snapshots must not be world-writable"));
        }

        return infos_dir;
    }

    SDir
    Zfs::openSnapshotDir(unsigned int num) const
    {
        SDir info_dir = openInfoDir(num);
        SDir snapshot_dir(info_dir, "snapshot");

        return snapshot_dir;
    }

    void
    Zfs::mountSnapshot(unsigned int num) const
    {
    }

    void
    Zfs::umountSnapshot(unsigned int num) const
    {
    }

    /* ZFS snapshots are always mounted int the `.zfs` directory, they are also
     * automatically symlinked by snapper in the datasets' `.snapshot`
     * directory.
     */
    bool
    Zfs::isSnapshotMounted(unsigned int num) const
    {
        return true;
    }

    void
    Zfs::createSnapshot(unsigned int num, unsigned int num_parent, bool read_only, bool quota, bool empty) const
    {
        if (num_parent != 0 || !read_only || quota || empty)
            SN_THROW(UnsupportedException());

        create_snapshot(zfs_volume_name, decString(num));
        symlink("../../.zfs/snapshot/" + decString(num), snapshotDir(num));
    }

    void
    Zfs::deleteSnapshot(unsigned int num) const
    {
        unlink(snapshotDir(num).c_str());
        delete_snapshot(zfs_volume_name, decString(num));
    }

    bool
    Zfs::checkSnapshot(unsigned int num) const
    {
        return snapshot_exists(zfs_volume_name, decString(num));
    }

    /* ZFS does not support mutable snapshots.
     */
    bool
    Zfs::isSnapshotReadOnly(unsigned int num) const
    {
        return true;
    }

    void
    Zfs::sync() const
    {
    }
}
