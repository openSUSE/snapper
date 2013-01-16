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


#ifndef SNAPPER_BTRFS_H
#define SNAPPER_BTRFS_H


#include "snapper/Filesystem.h"
#include "config.h"


namespace snapper
{

    class Btrfs : public Filesystem
    {
    public:

	static Filesystem* create(const string& fstype, const string& subvolume);

	Btrfs(const string& subvolume);

	virtual string fstype() const { return "btrfs"; }

	virtual void createConfig() const;
	virtual void deleteConfig() const;

	virtual string snapshotDir(unsigned int num) const;

	virtual SDir openSubvolumeDir() const;
	virtual SDir openInfosDir() const;
	virtual SDir openSnapshotDir(unsigned int num) const;

	virtual void createSnapshot(unsigned int num) const;
	virtual void deleteSnapshot(unsigned int num) const;

	virtual bool isSnapshotMounted(unsigned int num) const;
	virtual void mountSnapshot(unsigned int num) const;
	virtual void umountSnapshot(unsigned int num) const;

	virtual bool checkSnapshot(unsigned int num) const;

    private:

	bool is_subvolume(const struct stat& stat) const;

	bool create_subvolume(int fddst, const string& name) const;
	bool create_snapshot(int fd, int fddst, const string& name) const;
	bool delete_subvolume(int fd, const string& name) const;

    };

}


#endif
