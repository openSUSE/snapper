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
#include <vector>

#include "snapper/FileUtils.h"
#include "config.h"


namespace snapper
{
    using std::string;
    using std::vector;


    class Snapper;
    class MtabData;


    class Filesystem
    {
    public:

	Filesystem(const string& subvolume) : subvolume(subvolume) {}
	virtual ~Filesystem() {}

	static Filesystem* create(const string& fstype, const string& subvolume);

	virtual string fstype() const = 0;

	virtual void createConfig() const = 0;
	virtual void deleteConfig() const = 0;

	virtual string snapshotDir(unsigned int num) const = 0;

	virtual SDir openSubvolumeDir() const;
	virtual SDir openInfosDir() const = 0;
	virtual SDir openInfoDir(unsigned int num) const;
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


#ifdef ENABLE_BTRFS
    class Btrfs : public Filesystem
    {
    public:

	static Filesystem* create(const string& fstype, const string& subvolume);

	Btrfs(const string& subvolume);

	virtual string fstype() const { return "btrfs"; }

	virtual void createConfig() const;
	virtual void deleteConfig() const;

	virtual string snapshotDir(unsigned int num) const;

	virtual SDir openInfosDir() const;
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
#endif


#ifdef ENABLE_EXT4
    class Ext4 : public Filesystem
    {
    public:

	static Filesystem* create(const string& fstype, const string& subvolume);

	Ext4(const string& subvolume);

	virtual string fstype() const { return "ext4"; }

	virtual void createConfig() const;
	virtual void deleteConfig() const;

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

    private:

	vector<string> mount_options;

    };
#endif


#ifdef ENABLE_LVM
    class Lvm : public Filesystem
    {
    public:

	static Filesystem* create(const string& fstype, const string& subvolume);

	Lvm(const string& subvolume, const string& mount_type);

	virtual string fstype() const { return "lvm(" + mount_type + ")"; }

	virtual void createConfig() const;
	virtual void deleteConfig() const;

	virtual string snapshotDir(unsigned int num) const;
	virtual string snapshotLvName(unsigned int num) const;

	virtual SDir openInfosDir() const;
	virtual SDir openSnapshotDir(unsigned int num) const;

	virtual void createSnapshot(unsigned int num) const;
	virtual void deleteSnapshot(unsigned int num) const;

	virtual bool isSnapshotMounted(unsigned int num) const;
	virtual void mountSnapshot(unsigned int num) const;
	virtual void umountSnapshot(unsigned int num) const;

	virtual bool checkSnapshot(unsigned int num) const;

    private:

	const string mount_type;

	bool detectLvmNames(const MtabData& mtab_data);

	string getDevice(unsigned int num) const;

	string vg_name;
	string lv_name;

	vector<string> mount_options;

    };
#endif

}


#endif
