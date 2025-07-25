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


#ifndef SNAPPER_FILESYSTEM_H
#define SNAPPER_FILESYSTEM_H


#include <string>
#include <vector>
#include <utility>
#include <memory>

#include "snapper/FileUtils.h"
#include "snapper/Compare.h"
#include "snapper/Plugins.h"


namespace snapper
{
    using std::string;
    using std::vector;


    struct MtabData;
    class ConfigInfo;


    class Filesystem
    {
    public:

	Filesystem(const string& subvolume, const string& root_prefix)
	    : subvolume(subvolume), root_prefix(root_prefix) {}
	virtual ~Filesystem() {}

	static std::unique_ptr<Filesystem> create(const string& fstype, const string& subvolume,
						  const string& root_prefix);
	static std::unique_ptr<Filesystem> create(const ConfigInfo& config_info, const string& root_prefix);

	virtual void evalConfigInfo(const ConfigInfo& config_info) {}

	virtual string fstype() const = 0;

	virtual void createConfig() const = 0;
	virtual void deleteConfig() const = 0;

	virtual string snapshotDir(unsigned int num) const = 0;

	virtual SDir openSubvolumeDir() const;
	virtual SDir openInfosDir() const = 0;
	virtual SDir openInfoDir(unsigned int num) const;
	virtual SDir openSnapshotDir(unsigned int num) const = 0;

	virtual void createSnapshot(unsigned int num, unsigned int num_parent, bool read_only,
				    bool quota, bool empty) const = 0;
	virtual void createSnapshotOfDefault(unsigned int num, bool read_only, bool quota) const;
	virtual void deleteSnapshot(unsigned int num) const = 0;

	virtual bool isSnapshotMounted(unsigned int num) const = 0;
	virtual void mountSnapshot(unsigned int num) const = 0;
	virtual void umountSnapshot(unsigned int num) const = 0;

	virtual bool isSnapshotReadOnly(unsigned int num) const = 0;
	virtual void setSnapshotReadOnly(unsigned int num, bool read_only) const = 0;

	virtual bool checkSnapshot(unsigned int num) const = 0;

	virtual void cmpDirs(const SDir& dir1, const SDir& dir2, cmpdirs_cb_t cb) const;

	virtual bool isDefault(unsigned int num) const;

	/**
	 * Query the number of the default snapshot. The first entry of the
	 * pair indicates whether the default snapshot is a snapper snapshot
	 * (not necessarily in the list of snapshots known to snapper).
	 * Currently only available for btrfs.
	 */
	virtual std::pair<bool, unsigned int> getDefault() const;

	virtual void setDefault(unsigned int num, Plugins::Report& report) const;

	virtual std::pair<bool, unsigned int> getActive() const;

	virtual bool isActive(unsigned int num) const;

	virtual void sync() const;

    protected:

	const string subvolume;
	const string root_prefix;

	static vector<string> filter_mount_options(const vector<string>& options);

	static bool mount(const string& device, const SDir& dir, const string& mount_type,
			  const vector<string>& options);
	static bool umount(const SDir& dir, const string& mount_point);

    };

}


#endif
