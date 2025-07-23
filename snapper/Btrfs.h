/*
 * Copyright (c) [2011-2015] Novell, Inc.
 * Copyright (c) [2016-2023] SUSE LLC
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

	virtual void evalConfigInfo(const ConfigInfo& config_info) override;

	virtual string fstype() const override { return "btrfs"; }

	virtual void createConfig() const override;
	virtual void deleteConfig() const override;

	virtual void addToFstab(const string& default_subvolume_name) const;
	virtual void removeFromFstab() const;

	virtual string snapshotDir(unsigned int num) const override;

	virtual SDir openSubvolumeDir() const override;
	virtual SDir openInfosDir() const override;
	virtual SDir openSnapshotDir(unsigned int num) const override;

	/**
	 * A general read-write directory that can be used for ioctls. The
	 * exact directory can change to adapt to the system changes,
	 * e.g. which subvolumes are read-only.
	 */
	SDir openGeneralDir() const;

	virtual void createSnapshot(unsigned int num, unsigned int num_parent, bool read_only,
				    bool quota, bool empty) const override;
	virtual void createSnapshotOfDefault(unsigned int num, bool read_only, bool quota) const override;
	virtual void deleteSnapshot(unsigned int num) const override;

	virtual bool isSnapshotMounted(unsigned int num) const override;
	virtual void mountSnapshot(unsigned int num) const override;
	virtual void umountSnapshot(unsigned int num) const override;

	virtual bool isSnapshotReadOnly(unsigned int num) const override;
	virtual void setSnapshotReadOnly(unsigned int num, bool read_only) const override;

	virtual bool checkSnapshot(unsigned int num) const override;

	virtual void cmpDirs(const SDir& dir1, const SDir& dir2, cmpdirs_cb_t cb) const override;

	virtual bool isDefault(unsigned int num) const override;

	virtual std::pair<bool, unsigned int> getDefault() const override;

	virtual void setDefault(unsigned int num, Plugins::Report& report) const override;

	virtual bool isActive(unsigned int num) const override;

	virtual std::pair<bool, unsigned int> getActive() const override;

	virtual void sync() const override;

	virtual qgroup_t getQGroup() const { return qgroup; }

    private:

	qgroup_t qgroup;

	mutable vector<subvolid_t> deleted_subvolids;

	void addToFstabHelper(const string& default_subvolume_name) const;
	void removeFromFstabHelper() const;

	/**
	 * Find the snapper snapshot number corresponding to the btrfs
	 * subvol id.
	 */
	std::pair<bool, unsigned int> idToNum(int fd, subvolid_t id) const;

    };

}


#endif
