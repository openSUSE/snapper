/*
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


#ifndef SNAPPER_PROXY_DBUS_H
#define SNAPPER_PROXY_DBUS_H


#include "dbus/DBusMessage.h"
#include "dbus/DBusConnection.h"

#include "proxy.h"


// TODO move code from types.{cc,h} and commands.{cc,h} to proxy-dbus.{cc,h}


class ProxySnapshotDbus;
class ProxySnapshotsDbus;
class ProxySnapperDbus;
class ProxySnappersDbus;


/**
 * Concrete class of ProxySnapshot for DBus communication. Store all snapshot
 * data in the client to avoid numerous DBus queries.
 */
class ProxySnapshotDbus : public ProxySnapshot::Impl
{

public:

    ProxySnapshotDbus(ProxySnapshotsDbus* backref, SnapshotType type, unsigned int num,
		      time_t date, uid_t uid, unsigned int pre_num, const string& description,
		      const string& cleanup, const map<string, string>& userdata);

    ProxySnapshotDbus(ProxySnapshotsDbus* backref, unsigned int num);

    virtual SnapshotType getType() const override { return type; }
    virtual unsigned int getNum() const override { return num; }
    virtual time_t getDate() const override { return date; }
    virtual uid_t getUid() const override { return uid; }
    virtual unsigned int getPreNum() const override { return pre_num; }
    virtual const string& getDescription() const override { return description; }
    virtual const string& getCleanup() const override { return cleanup; }
    virtual const map<string, string>& getUserdata() const override { return userdata; }

    virtual bool isCurrent() const override { return num == 0; }

    virtual string mountFilesystemSnapshot(bool user_request) const override;
    virtual void umountFilesystemSnapshot(bool user_request) const override;

    DBus::Connection& conn() const;
    const string config_name() const;

    ProxySnapshotsDbus* backref;

    SnapshotType type;
    unsigned int num;
    time_t date;
    uid_t uid;
    unsigned int pre_num;
    string description;
    string cleanup;
    map<string, string> userdata;

};


class ProxySnapshotsDbus : public ProxySnapshots
{

public:

    ProxySnapshotsDbus(ProxySnapperDbus* backref)
	: backref(backref)
    {}

    void update();

    DBus::Connection& conn() const;
    const string config_name() const;

    ProxySnapperDbus* backref;

};


class ProxySnapperDbus : public ProxySnapper
{

public:

    ProxySnapperDbus(ProxySnappersDbus* backref, const string& config_name)
	: backref(backref), config_name(config_name), proxy_snapshots(this)
    {}

    virtual const string& configName() const override { return config_name; }

    virtual ProxyConfig getConfig() const override;
    virtual void setConfig(const ProxyConfig& proxy_config) override;

    virtual ProxySnapshots::const_iterator createSingleSnapshot(const SCD& scd) override;
    virtual ProxySnapshots::const_iterator createPreSnapshot(const SCD& scd) override;
    virtual ProxySnapshots::const_iterator createPostSnapshot(ProxySnapshots::const_iterator pre,
							      const SCD& scd) override;

    virtual void modifySnapshot(ProxySnapshots::iterator snapshot, const SMD& smd) override;

    virtual void deleteSnapshots(list<ProxySnapshots::iterator> snapshots, bool verbose) override;

    virtual ProxyComparison createComparison(const ProxySnapshot& lhs, const ProxySnapshot& rhs,
					     bool mount) override;

    virtual void syncFilesystem() const override;

    virtual ProxySnapshots& getSnapshots() override;

    virtual void setupQuota() override;

    virtual void prepareQuota() const override;

    virtual QuotaData queryQuotaData() const override;

    DBus::Connection& conn() const;

    ProxySnappersDbus* backref;

    string config_name;

    ProxySnapshotsDbus proxy_snapshots;

};


class ProxySnappersDbus : public ProxySnappers::Impl
{

public:

    ProxySnappersDbus()
	: conn(DBUS_BUS_SYSTEM)
    {}

    virtual void createConfig(const string& config_name, const string& subvolume,
			      const string& fstype, const string& template_name) override;

    virtual void deleteConfig(const string& config_name) override;

    virtual ProxySnapper* getSnapper(const string& config_name) override;

    virtual map<string, ProxyConfig> getConfigs() const override;

    virtual std::vector<string> debug() const override;

    mutable DBus::Connection conn;

    list<std::unique_ptr<ProxySnapperDbus>> proxy_snappers;

};


class ProxyComparisonDbus : public ProxyComparison::Impl
{
public:

    ProxyComparisonDbus(ProxySnapperDbus* backref, const ProxySnapshot& lhs,
			const ProxySnapshot& rhs, bool mount);

    ~ProxyComparisonDbus();

    virtual const Files& getFiles() const override { return files; }

    ProxySnapperDbus* backref;

    const ProxySnapshot& lhs;
    const ProxySnapshot& rhs;

    FilePaths file_paths;

    Files files;

};


#endif
