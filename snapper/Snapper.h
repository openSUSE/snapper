/*
 * Copyright (c) [2011-2015] Novell, Inc.
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


#include <vector>
#include <boost/noncopyable.hpp>

#include "snapper/Snapshot.h"
#include "snapper/AsciiFile.h"


namespace snapper
{
    using std::vector;


    class Filesystem;
    class SDir;


    class ConfigInfo : public SysconfigFile
    {
    public:

	explicit ConfigInfo(const string& config_name, const string& root_prefix);

	const string& getConfigName() const { return config_name; }
	const string& getSubvolume() const { return subvolume; }

	virtual void checkKey(const string& key) const;

    private:

	const string config_name;

	string subvolume;

    };


    struct ConfigNotFoundException : public SnapperException
    {
	explicit ConfigNotFoundException() throw() {}
	virtual const char* what() const throw() { return "config not found"; }
    };

    struct InvalidConfigException : public SnapperException
    {
	explicit InvalidConfigException() throw() {}
	virtual const char* what() const throw() { return "invalid config"; }
    };

    struct InvalidConfigdataException : public SnapperException
    {
	explicit InvalidConfigdataException() throw() {}
	virtual const char* what() const throw() { return "invalid configdata"; }
    };

    struct InvalidUserdataException : public SnapperException
    {
	explicit InvalidUserdataException() throw() {}
	virtual const char* what() const throw() { return "invalid userdata"; }
    };

    struct ListConfigsFailedException : public SnapperException
    {
	explicit ListConfigsFailedException(const char* msg) throw() : msg(msg) {}
	virtual const char* what() const throw() { return msg; }
	const char* msg;
    };

    struct CreateConfigFailedException : public SnapperException
    {
	explicit CreateConfigFailedException(const char* msg) throw() : msg(msg) {}
	virtual const char* what() const throw() { return msg; }
	const char* msg;
    };

    struct DeleteConfigFailedException : public SnapperException
    {
	explicit DeleteConfigFailedException(const char* msg) throw() : msg(msg) {}
	virtual const char* what() const throw() { return msg; }
	const char* msg;
    };


    class Snapper : private boost::noncopyable
    {
    public:

	Snapper(const string& config_name, const string& root_prefix, bool disable_filters = false);
	~Snapper();

	const string& configName() const { return config_info->getConfigName(); }

	string subvolumeDir() const;

	SDir openSubvolumeDir() const;
	SDir openInfosDir() const;

	Snapshots& getSnapshots() { return snapshots; }
	const Snapshots& getSnapshots() const { return snapshots; }

	Snapshots::const_iterator getSnapshotCurrent() const;

	Snapshots::iterator createSingleSnapshot(const SCD& scd);
	Snapshots::iterator createSingleSnapshot(Snapshots::const_iterator parent, const SCD& scd);
	Snapshots::iterator createSingleSnapshotOfDefault(const SCD& scd);
	Snapshots::iterator createPreSnapshot(const SCD& scd);
	Snapshots::iterator createPostSnapshot(Snapshots::const_iterator pre, const SCD& scd);

	void modifySnapshot(Snapshots::iterator snapshot, const SMD& smd);

	void deleteSnapshot(Snapshots::iterator snapshot);

	const vector<string>& getIgnorePatterns() const { return ignore_patterns; }

	static ConfigInfo getConfig(const string& config_name, const string& root_prefix);
	static list<ConfigInfo> getConfigs(const string& root_prefix);

	static void createConfig(const string& config_name, const string& root_prefix,
				 const string& subvolume, const string& fstype,
				 const string& template_name);
	static void deleteConfig(const string& config_name, const string& root_prefix);

	static bool detectFstype(const string& subvolume, string& fstype);

	const Filesystem* getFilesystem() const { return filesystem; }

	void setConfigInfo(const map<string, string>& raw);

	void syncAcl() const;

	void syncFilesystem() const;

	static const char* compileVersion();
	static const char* compileFlags();

    private:

	void filter1(list<Snapshots::iterator>& tmp, time_t min_age);
	void filter2(list<Snapshots::iterator>& tmp);

	void loadIgnorePatterns();

	void syncAcl(const vector<uid_t>& uids, const vector<gid_t>& gids) const;

	ConfigInfo* config_info;

	Filesystem* filesystem;

	vector<string> ignore_patterns;

	Snapshots snapshots;

    };

}


#endif
