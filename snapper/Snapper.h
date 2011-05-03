/*
 * Copyright (c) 2011 Novell, Inc.
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


#ifndef SNAPPER_H
#define SNAPPER_H


#include <vector>

#include "snapper/Snapshot.h"


namespace snapper
{
    using std::vector;


    class SysconfigFile;


    struct CompareCallback
    {
	CompareCallback() {}
	virtual ~CompareCallback() {}

	virtual void start() = 0;
	virtual void stop() = 0;
    };


    struct RollbackCallback
    {
	RollbackCallback() {}
	virtual ~RollbackCallback() {}

	virtual void start() = 0;
	virtual void stop() = 0;

	virtual void createInfo(const string& name) = 0;
	virtual void modifyInfo(const string& name) = 0;
	virtual void deleteInfo(const string& name) = 0;
    };


    struct ConfigInfo
    {
	ConfigInfo(const string& config_name, const string& subvolume)
	    : config_name(config_name), subvolume(subvolume) {}
	string config_name;
	string subvolume;
    };


    struct ConfigNotFoundException : public std::exception
    {
	explicit ConfigNotFoundException() throw() {}
	virtual const char* what() const throw() { return "config not found"; }
    };

    struct InvalidConfigException : public std::exception
    {
	explicit InvalidConfigException() throw() {}
	virtual const char* what() const throw() { return "invalid config"; }
    };

    struct ListConfigsFailedException : public std::exception
    {
	explicit ListConfigsFailedException(const char* msg) throw() : msg(msg) {}
	virtual const char* what() const throw() { return msg; }
	const char* msg;
    };

    struct AddConfigFailedException : public std::exception
    {
	explicit AddConfigFailedException(const char* msg) throw() : msg(msg) {}
	virtual const char* what() const throw() { return msg; }
	const char* msg;
    };


    class Snapper
    {
    public:

	Snapper(const string& config_name = "root");
	~Snapper();

	string subvolumeDir() const;
	string snapshotsDir() const;

	Snapshots& getSnapshots() { return snapshots; }
	const Snapshots& getSnapshots() const { return snapshots; }

	Snapshots::const_iterator getSnapshotCurrent() const;

	Snapshots::iterator createSingleSnapshot(string description);
	Snapshots::iterator createPreSnapshot(string description);
	Snapshots::iterator createPostSnapshot(Snapshots::const_iterator pre);

	void deleteSnapshot(Snapshots::iterator snapshot);

	void startBackgroundComparsion(Snapshots::const_iterator snapshot1,
				       Snapshots::const_iterator snapshot2);

	bool doCleanupNumber();
	bool doCleanupTimeline();
	bool doCleanupEmptyPrePost();

	void setCompareCallback(CompareCallback* p) { compare_callback = p; }
	CompareCallback* getCompareCallback() const { return compare_callback; }

	void setRollbackCallback(RollbackCallback* p) { rollback_callback = p; }
	RollbackCallback* getRollbackCallback() const { return rollback_callback; }

	const vector<string>& getIgnorePatterns() const { return ignore_patterns; }

	static list<ConfigInfo> getConfigs();
	static void addConfig(const string& config_name, const string& subvolume,
			      const string& template_name);

    private:

	void filter1(list<Snapshots::iterator>& tmp, time_t min_age);
	void filter2(list<Snapshots::iterator>& tmp);

	void loadIgnorePatterns();

	const string config_name;

	SysconfigFile* config;

	string subvolume;

	vector<string> ignore_patterns;

	Snapshots snapshots;

	CompareCallback* compare_callback;
	RollbackCallback* rollback_callback;

    };

};


#endif
