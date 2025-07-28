/*
 * Copyright (c) [2011-2015] Novell, Inc.
 * Copyright (c) [2016-2025] SUSE LLC
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


#ifndef SNAPPER_SNAPPER_H
#define SNAPPER_SNAPPER_H


#include <memory>
#include <vector>
#include <boost/noncopyable.hpp>

#include "snapper/Snapshot.h"
#include "snapper/Plugins.h"
#include "snapper/ConfigInfo.h"


namespace snapper
{

    class Filesystem;
    class SDir;
    class SelinuxLabelHandle;


    struct ConfigNotFoundException : public Exception
    {
	explicit ConfigNotFoundException() : Exception("config not found") {}
    };

    struct InvalidConfigException : public Exception
    {
	explicit InvalidConfigException() : Exception("invalid config") {}
    };

    struct InvalidConfigdataException : public Exception
    {
	explicit InvalidConfigdataException() : Exception("invalid configdata") {}
    };

    struct InvalidUserdataException : public Exception
    {
	explicit InvalidUserdataException() : Exception("invalid userdata") {}
    };

    struct ListConfigsFailedException : public Exception
    {
	explicit ListConfigsFailedException(const char* msg) : Exception(msg) {}
    };

    struct CreateConfigFailedException : public Exception
    {
	explicit CreateConfigFailedException(const char* msg) : Exception(msg) {}
    };

    struct DeleteConfigFailedException : public Exception
    {
	explicit DeleteConfigFailedException(const char* msg) : Exception(msg) {}
    };

    struct QuotaException : public Exception
    {
	explicit QuotaException(const char* msg) : Exception(msg) {}
    };

    struct FreeSpaceException : public Exception
    {
	explicit FreeSpaceException(const char* msg) : Exception(msg) {}
    };


    struct QuotaData
    {
	uint64_t size;
	uint64_t used;
    };

    struct FreeSpaceData
    {
	uint64_t size;
	uint64_t free;
    };


    class Snapper : private boost::noncopyable
    {
    public:

	Snapper(const string& config_name, const string& root_prefix, bool disable_filters = false);
	~Snapper();

	const string& configName() const { return config_info->get_config_name(); }

	string subvolumeDir() const;

	SDir openSubvolumeDir() const;
	SDir openInfosDir() const;

	Snapshots& getSnapshots() { return snapshots; }
	const Snapshots& getSnapshots() const { return snapshots; }

	Snapshots::const_iterator getSnapshotCurrent() const;

	Snapshots::iterator createSingleSnapshot(const SCD& scd, Plugins::Report& report);
	Snapshots::iterator createSingleSnapshot(Snapshots::const_iterator parent, const SCD& scd,
						 Plugins::Report& report);
	Snapshots::iterator createSingleSnapshotOfDefault(const SCD& scd, Plugins::Report& report);
	Snapshots::iterator createPreSnapshot(const SCD& scd, Plugins::Report& report);
	Snapshots::iterator createPostSnapshot(Snapshots::const_iterator pre, const SCD& scd,
					       Plugins::Report& report);

	void modifySnapshot(Snapshots::iterator snapshot, const SMD& smd, Plugins::Report& report);

	void deleteSnapshot(Snapshots::iterator snapshot, Plugins::Report& report);

	const vector<string>& getIgnorePatterns() const { return ignore_patterns; }

	static ConfigInfo getConfig(const string& config_name, const string& root_prefix);
	static list<ConfigInfo> getConfigs(const string& root_prefix);

	static void createConfig(const string& config_name, const string& root_prefix,
				 const string& subvolume, const string& fstype,
				 const string& template_name, Plugins::Report& report);
	static void deleteConfig(const string& config_name, const string& root_prefix, Plugins::Report& report);

	static bool detectFstype(const string& subvolume, string& fstype);

	const ConfigInfo& getConfigInfo() const { return *config_info; }
	void setConfigInfo(const map<string, string>& raw);

	const Filesystem* getFilesystem() const { return filesystem.get(); }

	void syncAcl() const;

	void syncFilesystem() const;

	void setupQuota();

	void prepareQuota() const;

	QuotaData queryQuotaData() const;

	FreeSpaceData queryFreeSpaceData() const;

	/**
	 * Calculate used spaces. So far only available for btrfs and
	 * only if quota is enabled. In that a btrfs rescan and sync
	 * is triggered.
	 */
	void calculateUsedSpace() const;

	/**
	 * Return the compression algorithm set in the config file or a fallback. Also
	 * checks if the compression is available and uses NONE as a fallback.
	 */
	Compression get_compression() const;

	static const char* compileVersion();
	static const char* compileFlags();

	static vector<string> debug();

    private:

	void filter1(list<Snapshots::iterator>& tmp, time_t min_age);
	void filter2(list<Snapshots::iterator>& tmp);

	void loadIgnorePatterns();

	void syncAcl(const vector<uid_t>& uids, const vector<gid_t>& gids) const;

	void syncSelinuxContexts(SelinuxLabelHandle* selabel_handle, bool skip_snapshot_dir) const;
	void syncSelinuxContextsInInfosDir(SelinuxLabelHandle* selabel_handle, bool skip_snapshot_dir) const;

	void syncInfoDir(SDir& dir) const;

	const std::string config_name;
	const std::string root_prefix;

	std::unique_ptr<ConfigInfo> config_info;

	std::unique_ptr<Filesystem> filesystem;

	vector<string> ignore_patterns;

	Snapshots snapshots;

    };

}


#endif
