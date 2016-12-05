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


#ifndef SNAPPER_PROXY_LIB_H
#define SNAPPER_PROXY_LIB_H


#include "proxy.h"

#include <snapper/Snapper.h>


class ProxySnapshotLib : public ProxySnapshot::Impl
{

public:

    ProxySnapshotLib(Snapshots::const_iterator it)
	: it(it)
    {}

    virtual SnapshotType getType() const override { return it->getType(); }
    virtual unsigned int getNum() const override { return it->getNum(); }
    virtual time_t getDate() const override { return it->getDate(); }
    virtual uid_t getUid() const override { return it->getUid(); }
    virtual unsigned int getPreNum() const override { return it->getPreNum(); }
    virtual const string& getDescription() const override { return it->getDescription(); }
    virtual const string& getCleanup() const override { return it->getCleanup(); }
    virtual const map<string, string>& getUserdata() const override { return it->getUserdata(); }

    virtual bool isCurrent() const override { return it->isCurrent(); }

    virtual void mountFilesystemSnapshot(bool user_request) const override {
	it->mountFilesystemSnapshot(user_request);
    }

    virtual void umountFilesystemSnapshot(bool user_request) const override {
	it->umountFilesystemSnapshot(user_request);
    }

    Snapshots::const_iterator it;

};


class ProxySnapperLib;


class ProxySnapshotsLib : public ProxySnapshots
{

public:

    ProxySnapshotsLib(ProxySnapperLib* backref)
	: backref(backref)
    {}

    void update();

    ProxySnapperLib* backref;

};


class ProxySnapperLib : public ProxySnapper
{

public:

    ProxySnapperLib(const string& config_name)
	: snapper(new Snapper(config_name, "/")), proxy_snapshots(this)
    {}

    virtual void setConfigInfo(const map<string, string>& raw) override;

    virtual ProxySnapshots::const_iterator createSingleSnapshot(const SCD& scd) override;
    virtual ProxySnapshots::const_iterator createPreSnapshot(const SCD& scd) override;
    virtual ProxySnapshots::const_iterator createPostSnapshot(const ProxySnapshots::const_iterator pre, const SCD& scd) override;

    virtual const ProxySnapshots& getSnapshots() override;

    Snapper* snapper;

    ProxySnapshotsLib proxy_snapshots;

};


class ProxySnappersLib : public ProxySnappers
{

public:

    ProxySnappersLib(const string& target_root)
	: target_root(target_root)
    {}

    virtual void createConfig(const string& config_name, const string& subvolume,
			      const string& fstype, const string& template_name) override;

    virtual void deleteConfig(const string& config_name) override;

    virtual ProxySnapper* getSnapper(const string& config_name) override;

    const string target_root;

    list<std::unique_ptr<ProxySnapperLib>> proxy_snappers;

};


#endif
