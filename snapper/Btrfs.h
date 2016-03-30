/*
 * Copyright (c) [2011-2015] Novell, Inc.
 * Copyright (c) 2016 SUSE LLC
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
#include "snapper/BtrfsUtils.h"


namespace snapper
{

    using namespace BtrfsUtils;


    class Btrfs : public Filesystem
    {
    public:

	static Filesystem* create(const string& fstype, const string& subvolume,
				  const string& root_prefix);

	Btrfs(const string& subvolume, const string& root_prefix);

	virtual void evalConfigInfo(const ConfigInfo& config_info);

	virtual string fstype() const { return "btrfs"; }

	virtual void createConfig() const;
	virtual void deleteConfig() const;

	virtual void addToFstab(const string& default_subvolume_name) const;
	virtual void removeFromFstab() const;

	virtual string snapshotDir(unsigned int num) const;

	virtual SDir openSubvolumeDir() const;
	virtual SDir openInfosDir() const;
	virtual SDir openSnapshotDir(unsigned int num) const;

	virtual void createSnapshot(unsigned int num, unsigned int num_parent, bool read_only,
				    bool quota) const;
	virtual void createSnapshotOfDefault(unsigned int num, bool read_only, bool quota) const;
	virtual void deleteSnapshot(unsigned int num) const;

	virtual bool isSnapshotMounted(unsigned int num) const;
	virtual void mountSnapshot(unsigned int num) const;
	virtual void umountSnapshot(unsigned int num) const;

	virtual bool isSnapshotReadOnly(unsigned int num) const;

	virtual bool checkSnapshot(unsigned int num) const;

	virtual void cmpDirs(const SDir& dir1, const SDir& dir2, cmpdirs_cb_t cb) const;

	virtual void setDefault(unsigned int num) const;

	virtual void sync() const;

	virtual qgroup_t getQGroup() const { return qgroup; }

    private:

	qgroup_t qgroup;

	mutable vector<subvolid_t> deleted_subvolids;

	void addToFstabHelper(const string& default_subvolume_name) const;
	void removeFromFstabHelper() const;

    };

}


#endif
