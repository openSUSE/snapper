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

/* ZFS Snapper 2017 Daniel Sullivan (mumblepins@gmail.com)
 * 
 * TODO:
 *  - Deal with ACLs
 *  - implement bind and legacy mounts *  
 */

#include "config.h"

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <boost/algorithm/string.hpp>

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


    Zfs::Zfs(const string& subvolume, const string& root_prefix)
    : Filesystem(subvolume, root_prefix)
    {
        if (access(ZFSBIN, X_OK) != 0)
        {
            throw ProgramNotInstalledException(ZFSBIN " not installed");
        }

        bool found = false;
        MtabData mtab_data;

        if (!getMtabData(subvolume, found, mtab_data))
            throw InvalidConfigException();

        if (!found)
        {
            y2err("filesystem not mounted");
            throw InvalidConfigException();
        }

        zfs_subvol_fs = mtab_data.device;
        zfs_snapshot_fs = zfs_subvol_fs + "/.snapshots";
        zfs_zpool = poolname(zfs_subvol_fs);
    }


    void
    Zfs::evalConfigInfo(const ConfigInfo& config_info) {
        // TODO: Add multiple "mount" types
    }


    void
    Zfs::createConfig() const
    {


        SDir subvolume_dir = openSubvolumeDir();

        int r1 = subvolume_dir.mkdir(".snapshots", 0700);
        if (r1 != 0 && errno != EEXIST)
        {
            y2err("mkdir failed errno:" << errno << " (" << strerror(errno) << ")");
            SN_THROW(CreateConfigFailedException("mkdir failed"));
        }

        SDir infos_dir = openInfosDir();

        create_volume(infos_dir.fullname(), zfs_snapshot_fs);
    }


    void
    Zfs::deleteConfig() const
    {
        SDir subvolume_dir = openSubvolumeDir();
        delete_volume(zfs_snapshot_fs);
        subvolume_dir.unlink(".snapshots", AT_REMOVEDIR);
    }


    string
    Zfs::snapshotDir(unsigned int num) const
    {
        return (subvolume == "/" ? "" : subvolume) + "/.snapshots/" + decString(num) +
            "/snapshot";
    }


    SDir
    Zfs::openInfosDir() const
    {
        SDir subvolume_dir = openSubvolumeDir();
        SDir infos_dir(subvolume_dir, ".snapshots");

        struct stat stat;
        if (infos_dir.stat(&stat) != 0)
        {
            throw IOErrorException("stat on .snapshots failed");
        }

        if (stat.st_uid != 0)
        {
            y2err(".snapshots must have owner root");
            throw IOErrorException(".snapshots must have owner root");
        }

        if (stat.st_gid != 0 && stat.st_mode & S_IWGRP)
        {
            y2err(".snapshots must have group root or must not be group-writable");
            throw IOErrorException(".snapshots must have group root or must not be group-writable");
        }

        if (stat.st_mode & S_IWOTH)
        {
            y2err(".snapshots must not be world-writable");
            throw IOErrorException(".snapshots must not be world-writable");
        }

        return infos_dir;
    }


    SDir
    Zfs::openSnapshotDir(unsigned int num) const
    {
        SDir info_dir = openInfoDir(num);
        SDir snapshot_dir = info_dir;
        if (mount_type == SYMLINK)
        {
            snapshot_dir = follow_symlink(info_dir, "snapshot");
        }
        else
        {
            snapshot_dir = SDir(info_dir, "snapshot");
        }
        return snapshot_dir;
    }


    string
    Zfs::snapshotName(unsigned int num) const
    {
        return "snapper-" + decString(num);
    }


    void
    Zfs::createSnapshot(unsigned int num, unsigned int num_parent, bool read_only, bool quota) const
    {
        if (num_parent != 0 || !read_only)
            throw std::logic_error("not implemented");

        SDir subvolume_dir = openSubvolumeDir();
        SDir info_dir = openInfoDir(num);
        create_snapshot(zfs_subvol_fs, snapshotName(num));
        switch (mount_type)
        {
        case SYMLINK:
            symlink("../../.zfs/snapshot/" + snapshotName(num), info_dir.fullname() + "/snapshot");
            break;
        case BIND:
            // TODO
            break;
        case LEGACY:
            // TODO
            break;
        }
    }


    void
    Zfs::deleteSnapshot(unsigned int num) const
    {

        delete_volume(zfs_subvol_fs + "@" + snapshotName(num));

        switch (mount_type)
        {
        case SYMLINK:
        {
            // need to delete symlink
            SDir info_dir = openInfoDir(num);
            info_dir.unlink("snapshot", 0);
        }
            break;
        case BIND:
            // TODO
            break;
        case LEGACY:
            // TODO
            break;
        }
    }


    bool
    Zfs::isSnapshotMounted(unsigned int num) const
    {
        switch (mount_type)
        {
        case SYMLINK:
            // TODO
            break;
        case BIND:
            // TODO
            break;
        case LEGACY:
            // TODO
            break;
        }
        return true;
    }


    void
    Zfs::mountSnapshot(unsigned int num) const
    {
        switch (mount_type)
        {
        case SYMLINK:
            // TODO
            break;
        case BIND:
            // TODO
            break;
        case LEGACY:
            // TODO
            break;
        }
    }


    void
    Zfs::umountSnapshot(unsigned int num) const
    {
        switch (mount_type)
        {
        case SYMLINK:
            // TODO
            break;
        case BIND:
            // TODO
            break;
        case LEGACY:
            // TODO
            break;
        }
    }


    bool
    Zfs::isSnapshotReadOnly(unsigned int num) const
    {
        return true;
    }


    bool
    Zfs::checkSnapshot(unsigned int num) const
    {
        try
        {
            //TODO make better
            SDir info_dir = openInfoDir(num);

            struct stat stat;
            int r = info_dir.stat("snapshot", &stat, AT_SYMLINK_NOFOLLOW);
            return r == 0;
        }
        catch (const IOErrorException& e)
        {
            return false;
        }
    }


    void
    Zfs::sync() const {
        // TODO
    }

}


