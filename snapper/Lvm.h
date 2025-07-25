/*
 * Copyright (c) [2011-2014] Novell, Inc.
 * Copyright (c) 2023 SUSE LLC
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


#ifndef SNAPPER_LVM_H
#define SNAPPER_LVM_H

#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>

#include "snapper/Filesystem.h"
#include "snapper/Exception.h"


namespace snapper
{

    struct LvmActivationException : public Exception
    {
	explicit LvmActivationException() : Exception("lvm snapshot activation exception") {}
    };


    struct LvmDeactivatationException : public Exception
    {
	explicit LvmDeactivatationException() : Exception("lvm snapshot deactivation exception") {}
    };


    struct lvm_version
    {
	lvm_version(uint16_t maj, uint16_t min, uint16_t rev)
	    : version(rev | ((uint32_t)min << 16)  | ((uint64_t)maj << 32)) {}

	const uint64_t version;
    };

    bool operator>=(const lvm_version& a, const lvm_version& b);


    class LvmCache;


    class LvmCapabilities : public boost::noncopyable
    {
    public:

	static LvmCapabilities* get_lvm_capabilities();

	const string& get_ignoreactivationskip() const { return ignoreactivationskip; }

    private:

	LvmCapabilities();

	// empty or "--ignoreactivationskip" if lvm supports ignore activation skip flag
	string ignoreactivationskip;

    };


    class Lvm : public Filesystem
    {
    public:

	static std::unique_ptr<Filesystem> create(const string& fstype, const string& subvolume,
						  const string& root_prefix);

	Lvm(const string& subvolume, const string& root_prefix, const string& mount_type);

	virtual string fstype() const override { return "lvm(" + mount_type + ")"; }

	virtual void createConfig() const override;
	virtual void deleteConfig() const override;

	virtual string snapshotDir(unsigned int num) const override;
	virtual string snapshotLvName(unsigned int num) const;

	virtual SDir openInfosDir() const override;
	virtual SDir openSnapshotDir(unsigned int num) const override;

	virtual void createSnapshot(unsigned int num, unsigned int num_parent, bool read_only,
				    bool quota, bool empty) const override;
	virtual void deleteSnapshot(unsigned int num) const override;

	virtual bool isSnapshotMounted(unsigned int num) const override;
	virtual void mountSnapshot(unsigned int num) const override;
	virtual void umountSnapshot(unsigned int num) const override;

	virtual bool isSnapshotReadOnly(unsigned int num) const override;
	virtual void setSnapshotReadOnly(unsigned int num, bool read_only) const override;

	virtual bool checkSnapshot(unsigned int num) const override;

    private:

	mutable boost::mutex mount_mutex;

	const string mount_type;
	LvmCache* cache;

	bool detectThinVolumeNames(const MtabData& mtab_data);
	void activateSnapshot(const string& vg_name, const string& lv_name) const;
	void deactivateSnapshot(const string& vg_name, const string& lv_name) const;
	bool detectInactiveSnapshot(const string& vg_name, const string& lv_name) const;
	void createLvmConfig(const SDir& subvolume_dir, int mode) const;

	string getDevice(unsigned int num) const;

	string vg_name;
	string lv_name;

	vector<string> mount_options;

    };

}

#endif
