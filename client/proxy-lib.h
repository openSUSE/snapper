/*
 * Copyright (c) [2016-2020] SUSE LLC
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
#include <snapper/Comparison.h>


class ProxySnapshotLib : public ProxySnapshot::Impl
{

public:

    ProxySnapshotLib(Snapshots::iterator it)
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

    virtual uint64_t getUsedSpace() const override
    {
	return it->getUsedSpace();
    }

    virtual string mountFilesystemSnapshot(bool user_request) const override
    {
	it->mountFilesystemSnapshot(user_request);
	return it->snapshotDir();
    }

    virtual void umountFilesystemSnapshot(bool user_request) const override
    {
	it->umountFilesystemSnapshot(user_request);
    }

    Snapshots::iterator it;

};


class ProxySnapperLib;


class ProxySnapshotsLib : public ProxySnapshots
{

public:

    virtual iterator getDefault() override;
    virtual const_iterator getDefault() const override;

    virtual iterator getActive() override;
    virtual const_iterator getActive() const override;

    ProxySnapshotsLib(ProxySnapperLib* backref);

    ProxySnapperLib* backref;

};


class ProxySnapperLib : public ProxySnapper
{

public:

    ProxySnapperLib(const string& config_name, const string& target_root)
	: snapper(new Snapper(config_name, target_root)), proxy_snapshots(this)
    {}

    virtual const string& configName() const override { return snapper->configName(); }

    virtual ProxyConfig getConfig() const override;
    virtual void setConfig(const ProxyConfig& proxy_config) override;

    virtual ProxySnapshots::const_iterator createSingleSnapshot(const SCD& scd) override;
    virtual ProxySnapshots::const_iterator createSingleSnapshot(ProxySnapshots::const_iterator parent,
								const SCD& scd) override;
    virtual ProxySnapshots::const_iterator createSingleSnapshotOfDefault(const SCD& scd) override;
    virtual ProxySnapshots::const_iterator createPreSnapshot(const SCD& scd) override;
    virtual ProxySnapshots::const_iterator createPostSnapshot(ProxySnapshots::const_iterator pre,
							      const SCD& scd) override;

    virtual void modifySnapshot(ProxySnapshots::iterator snapshot, const SMD& smd) override;

    virtual void deleteSnapshots(vector<ProxySnapshots::iterator> snapshots, bool verbose) override;

    virtual ProxyComparison createComparison(const ProxySnapshot& lhs, const ProxySnapshot& rhs,
					     bool mount) override;

    virtual void syncFilesystem() const override { snapper->syncFilesystem(); }

    virtual ProxySnapshots& getSnapshots() override { return proxy_snapshots; }
    virtual const ProxySnapshots& getSnapshots() const override { return proxy_snapshots; }

    virtual void setupQuota() override { snapper->setupQuota(); }

    virtual void prepareQuota() const override { snapper->prepareQuota(); }

    virtual QuotaData queryQuotaData() const override { return snapper->queryQuotaData(); }

    virtual FreeSpaceData queryFreeSpaceData() const override { return snapper->queryFreeSpaceData(); }

    virtual void calculateUsedSpace() const override { snapper->calculateUsedSpace(); }

    std::unique_ptr<Snapper> snapper;

private:

    ProxySnapshotsLib proxy_snapshots;

};


class ProxySnappersLib : public ProxySnappers::Impl
{

public:

    ProxySnappersLib(const string& target_root)
	: target_root(target_root)
    {}

    virtual void createConfig(const string& config_name, const string& subvolume,
			      const string& fstype, const string& template_name) override;

    virtual void deleteConfig(const string& config_name) override;

    virtual ProxySnapper* getSnapper(const string& config_name) override;

    virtual map<string, ProxyConfig> getConfigs() const override;

    virtual vector<string> debug() const override { return Snapper::debug(); }

private:

    const string target_root;

    list<std::unique_ptr<ProxySnapperLib>> proxy_snappers;

};


class ProxyComparisonLib : public ProxyComparison::Impl
{

public:

    ProxyComparisonLib(ProxySnapperLib* proxy_snapper, const ProxySnapshot& lhs,
		       const ProxySnapshot& rhs, bool mount);

    virtual const Files& getFiles() const override { return comparison->getFiles(); }

    ProxySnapper* proxy_snapper;

private:

    std::unique_ptr<Comparison> comparison;

};


const ProxySnapshotLib&
to_lib(const ProxySnapshot& proxy_snapshot);


#endif
