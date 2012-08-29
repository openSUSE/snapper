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


#ifndef SNAPPER_FILESYSTEM_H
#define SNAPPER_FILESYSTEM_H


#include <string>

#include "snapper/FileUtils.h"


namespace snapper
{
    using std::string;


    class Snapper;


    class Filesystem
    {
    public:

	Filesystem(const string& subvolume) : subvolume(subvolume) {}
	virtual ~Filesystem() {}

	static Filesystem* create(const string& fstype, const string& subvolume);

	virtual string name() const = 0;

	virtual void createConfig() const = 0;
	virtual void deleteConfig() const = 0;

	virtual string infosDir() const = 0;
	virtual string snapshotDir(unsigned int num) const = 0;

	virtual SDir openInfosDir() const = 0;
	virtual SDir openSnapshotDir(unsigned int num) const = 0;

	virtual void createSnapshot(unsigned int num) const = 0;
	virtual void deleteSnapshot(unsigned int num) const = 0;

	virtual bool isSnapshotMounted(unsigned int num) const = 0;
	virtual void mountSnapshot(unsigned int num) const = 0;
	virtual void umountSnapshot(unsigned int num) const = 0;

	virtual bool checkSnapshot(unsigned int num) const = 0;

    protected:

	const string subvolume;

    };


    class Btrfs : public Filesystem
    {
    public:

	Btrfs(const string& subvolume);

	virtual string name() const { return "btrfs"; }

	virtual void createConfig() const;
	virtual void deleteConfig() const;

	virtual string infosDir() const;
	virtual string snapshotDir(unsigned int num) const;

	virtual SDir openSubvolumeDir() const;
	virtual SDir openInfosDir() const;
	virtual SDir openInfoDir(unsigned int num) const;
	virtual SDir openSnapshotDir(unsigned int num) const;

	virtual void createSnapshot(unsigned int num) const;
	virtual void deleteSnapshot(unsigned int num) const;

	virtual bool isSnapshotMounted(unsigned int num) const;
	virtual void mountSnapshot(unsigned int num) const;
	virtual void umountSnapshot(unsigned int num) const;

	virtual bool checkSnapshot(unsigned int num) const;

    private:

	bool create_subvolume(int fddst, const string& name) const;
	bool create_snapshot(int fd, int fddst, const string& name) const;
	bool delete_subvolume(int fd, const string& name) const;

    };


    class Ext4 : public Filesystem
    {
    public:

	Ext4(const string& subvolume);

	virtual string name() const { return "ext4"; }

	virtual void createConfig() const;
	virtual void deleteConfig() const;

	virtual string infosDir() const;
	virtual string snapshotDir(unsigned int num) const;
	virtual string snapshotFile(unsigned int num) const;

	virtual SDir openInfosDir() const;
	virtual SDir openSnapshotDir(unsigned int num) const;

	virtual void createSnapshot(unsigned int num) const;
	virtual void deleteSnapshot(unsigned int num) const;

	virtual bool isSnapshotMounted(unsigned int num) const;
	virtual void mountSnapshot(unsigned int num) const;
	virtual void umountSnapshot(unsigned int num) const;

	virtual bool checkSnapshot(unsigned int num) const;

    };

}


#endif
