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


#ifndef SNAPPER_EXT4_H
#define SNAPPER_EXT4_H


#include "snapper/Filesystem.h"


namespace snapper
{

    class Ext4 : public Filesystem
    {
    public:

	static std::unique_ptr<Filesystem> create(const string& fstype, const string& subvolume,
						  const string& root_prefix);

	Ext4(const string& subvolume, const string& root_prefix);

	virtual string fstype() const override { return "ext4"; }

	virtual void createConfig() const override;
	virtual void deleteConfig() const override;

	virtual string snapshotDir(unsigned int num) const override;
	virtual string snapshotFile(unsigned int num) const;

	virtual SDir openInfosDir() const override;
	virtual SDir openSnapshotDir(unsigned int num) const override;

	virtual void createSnapshot(unsigned int num, unsigned int num_parent, bool read_only,
				    bool quota, bool empty) const override;
	virtual void deleteSnapshot(unsigned int num) const override;

	virtual bool isSnapshotMounted(unsigned int num) const override;
	virtual void mountSnapshot(unsigned int num) const override;
	virtual void umountSnapshot(unsigned int num) const override;

	virtual bool isSnapshotReadOnly(unsigned int num) const override;
	virtual void setSnapshotReadOnly(unsigned int num, bool read_only, Plugins::Report& report) const override;

	virtual bool checkSnapshot(unsigned int num) const override;

    private:

	vector<string> mount_options;

    };

}


#endif
