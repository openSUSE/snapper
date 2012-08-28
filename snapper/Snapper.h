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


#ifndef SNAPPER_SNAPPER_H
#define SNAPPER_SNAPPER_H


#include <vector>
#include <boost/noncopyable.hpp>

#include "snapper/Snapshot.h"


namespace snapper
{
    using std::vector;


    class SysconfigFile;
    class Filesystem;
    class Comparison;
    class SDir;


    struct ConfigInfo
    {
	ConfigInfo(const string& config_name, const string& subvolume,
		   const map<string, string>& raw)
	    : config_name(config_name), subvolume(subvolume), raw(raw) {}

	string config_name;
	string subvolume;

	map<string, string> raw;
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

	Snapper(const string& config_name = "root", bool disable_filters = false);
	~Snapper();

	string configName() const { return config_name; }

	string subvolumeDir() const;
	string infosDir() const;

#ifndef SWIG
	SDir openSubvolumeDir() const;
	SDir openInfosDir() const;
#endif

	Snapshots& getSnapshots() { return snapshots; }
	const Snapshots& getSnapshots() const { return snapshots; }

	Snapshots::const_iterator getSnapshotCurrent() const;

	Snapshots::iterator createSingleSnapshot(string description);
	Snapshots::iterator createPreSnapshot(string description);
	Snapshots::iterator createPostSnapshot(string description, Snapshots::const_iterator pre);

	void deleteSnapshot(Snapshots::iterator snapshot);

	void startBackgroundComparsion(Snapshots::const_iterator snapshot1,
				       Snapshots::const_iterator snapshot2);

	const vector<string>& getIgnorePatterns() const { return ignore_patterns; }

	static ConfigInfo getConfig(const string& config_name);
	static list<ConfigInfo> getConfigs();
	static void createConfig(const string& config_name, const string& subvolume,
				 const string& fstype, const string& template_name);
	static void deleteConfig(const string& config_name);

	static bool detectFstype(const string& subvolume, string& fstype);

	const Filesystem* getFilesystem() const { return filesystem; }

	const SysconfigFile* getSysconfigFile() const { return config; }

    private:

	void filter1(list<Snapshots::iterator>& tmp, time_t min_age);
	void filter2(list<Snapshots::iterator>& tmp);

	void loadIgnorePatterns();

	const string config_name;

	SysconfigFile* config;

	string subvolume;

	Filesystem* filesystem;

	vector<string> ignore_patterns;

	Snapshots snapshots;

    };

};


#endif
