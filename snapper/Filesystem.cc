/*
 * Copyright (c) 2011 Novell, Inc.
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


#include <sys/stat.h>
#include <sys/types.h>

#include "snapper/Filesystem.h"
#include "snapper/Snapper.h"
#include "snapper/SnapperTmpl.h"
#include "snapper/SystemCmd.h"
#include "snapper/SnapperDefines.h"


namespace snapper
{

    string
    Btrfs::infosDir() const
    {
	return snapper->subvolumeDir() + "/.snapshots";
    }


    string
    Btrfs::snapshotDir(unsigned int num) const
    {
	return snapper->subvolumeDir() + "/.snapshots/" + decString(num) + "/snapshot";
    }


    void
    Btrfs::createFilesystemSnapshot(unsigned int num) const
    {
	SystemCmd cmd(BTRFSBIN " subvolume snapshot " + snapper->subvolumeDir() + " " + snapshotDir(num));
	if (cmd.retcode() != 0)
	    throw CreateSnapshotFailedException();
    }


    void
    Btrfs::deleteFilesystemSnapshot(unsigned int num) const
    {
	SystemCmd cmd(BTRFSBIN " subvolume delete " + snapshotDir(num));
	if (cmd.retcode() != 0)
	    throw DeleteSnapshotFailedException();
    }


    void
    Btrfs::mountFilesystemSnapshot(unsigned int num) const
    {
    }


    void
    Btrfs::umountFilesystemSnapshot(unsigned int num) const
    {
    }


    bool
    Btrfs::checkFilesystemSnapshot(unsigned int num) const
    {
	return checkDir(snapshotDir(num));
    }


    string
    Ext4::infosDir() const
    {
	return snapper->subvolumeDir() + "/.snapshots-info";
    }


    string
    Ext4::snapshotDir(unsigned int num) const
    {
	return snapper->subvolumeDir() + "@" + decString(num);
    }


    string
    Ext4::snapshotFile(unsigned int num) const
    {
	return snapper->subvolumeDir() + "/.snapshots/" + decString(num);
    }


    void
    Ext4::createFilesystemSnapshot(unsigned int num) const
    {
	SystemCmd cmd1(TOUCHBIN " " + snapshotFile(num));
	if (cmd1.retcode() != 0)
	    throw CreateSnapshotFailedException();

	SystemCmd cmd2(CHSNAPBIN " +S " + snapshotFile(num));
	if (cmd2.retcode() != 0)
	    throw CreateSnapshotFailedException();
    }


    void
    Ext4::deleteFilesystemSnapshot(unsigned int num) const
    {
	SystemCmd cmd(CHSNAPBIN " -S " + snapshotFile(num));
	if (cmd.retcode() != 0)
            throw DeleteSnapshotFailedException();

	// TODO
    }


    void
    Ext4::mountFilesystemSnapshot(unsigned int num) const
    {
	SystemCmd cmd1(CHSNAPBIN " +n " + snapshotFile(num));
	if (cmd1.retcode() != 0)
	    throw MountSnapshotFailedException();

	mkdir(snapshotDir(num).c_str(), 0755);

	SystemCmd cmd2(MOUNTBIN " -t ext4 -r -o loop,noload " + snapshotFile(num) +
		       " " + snapshotDir(num));
	if (cmd2.retcode() != 0)
	    throw MountSnapshotFailedException();
    }


    void
    Ext4::umountFilesystemSnapshot(unsigned int num) const
    {
	// TODO
    }


    bool
    Ext4::checkFilesystemSnapshot(unsigned int num) const
    {
	return checkNormalFile(snapshotFile(num));
    }

}
