/*
 * Copyright (c) [2016,2018] SUSE LLC
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


#include "proxy-lib.h"


using namespace std;


ProxySnapshots::const_iterator
ProxySnapshotsLib::getDefault() const
{
    Snapshots& snapshots = backref->snapper->getSnapshots();

    Snapshots::const_iterator tmp = snapshots.getDefault();

    return tmp != snapshots.end() ? find(tmp->getNum()) : end();
}


ProxySnapshots::const_iterator
ProxySnapshotsLib::getActive() const
{
    Snapshots& snapshots = backref->snapper->getSnapshots();

    Snapshots::const_iterator tmp = snapshots.getActive();

    return tmp != snapshots.end() ? find(tmp->getNum()) : end();
}


ProxyConfig
ProxySnapperLib::getConfig() const
{
    return ProxyConfig(snapper->getConfigInfo().getAllValues());
}


void
ProxySnapperLib::setConfig(const ProxyConfig& proxy_config)
{
    snapper->setConfigInfo(proxy_config.getAllValues());
}


ProxySnapshots::const_iterator
ProxySnapperLib::createSingleSnapshot(const SCD& scd)
{
    proxy_snapshots.emplace_back(new ProxySnapshotLib(snapper->createSingleSnapshot(scd)));

    return --proxy_snapshots.end();
}


ProxySnapshots::const_iterator
ProxySnapperLib::createSingleSnapshot(ProxySnapshots::const_iterator parent, const SCD& scd)
{
    proxy_snapshots.emplace_back(new ProxySnapshotLib(snapper->createSingleSnapshot(to_lib(*parent).it, scd)));

    return --proxy_snapshots.end();
}


ProxySnapshots::const_iterator
ProxySnapperLib::createSingleSnapshotOfDefault(const SCD& scd)
{
    proxy_snapshots.emplace_back(new ProxySnapshotLib(snapper->createSingleSnapshotOfDefault(scd)));

    return --proxy_snapshots.end();
}


ProxySnapshots::const_iterator
ProxySnapperLib::createPreSnapshot(const SCD& scd)
{
    proxy_snapshots.emplace_back(new ProxySnapshotLib(snapper->createPreSnapshot(scd)));

    return --proxy_snapshots.end();
}


ProxySnapshots::const_iterator
ProxySnapperLib::createPostSnapshot(ProxySnapshots::const_iterator pre, const SCD& scd)
{
    proxy_snapshots.emplace_back(new ProxySnapshotLib(snapper->createPostSnapshot(to_lib(*pre).it, scd)));

    return --proxy_snapshots.end();
}


void
ProxySnapperLib::modifySnapshot(ProxySnapshots::iterator snapshot, const SMD& smd)
{
    snapper->modifySnapshot(to_lib(*snapshot).it, smd);
}


void
ProxySnapperLib::deleteSnapshots(vector<ProxySnapshots::iterator> snapshots, bool verbose)
{
    for (ProxySnapshots::iterator& snapshot : snapshots)
	snapper->deleteSnapshot(to_lib(*snapshot).it);

    ProxySnapshots& proxy_snapshots = getSnapshots();
    for (ProxySnapshots::iterator& proxy_snapshot : snapshots)
        proxy_snapshots.erase(proxy_snapshot);
}


ProxyComparison
ProxySnapperLib::createComparison(const ProxySnapshot& lhs, const ProxySnapshot& rhs, bool mount)
{
    return ProxyComparison(new ProxyComparisonLib(this, lhs, rhs, mount));
}


ProxySnapshotsLib::ProxySnapshotsLib(ProxySnapperLib* backref)
    : backref(backref)
{
    Snapshots& tmp = backref->snapper->getSnapshots();
    for (Snapshots::iterator it = tmp.begin(); it != tmp.end(); ++it)
	proxy_snapshots.push_back(new ProxySnapshotLib(it));
}


void
ProxySnappersLib::createConfig(const string& config_name, const string& subvolume,
			       const string& fstype, const string& template_name)
{
    Snapper::createConfig(config_name, target_root, subvolume, fstype, template_name);
}


void
ProxySnappersLib::deleteConfig(const string& config_name)
{
    Snapper::deleteConfig(config_name, target_root);
}


ProxySnapper*
ProxySnappersLib::getSnapper(const string& config_name)
{
    for (unique_ptr<ProxySnapperLib>& proxy_snapper : proxy_snappers)
    {
	if (proxy_snapper->snapper->configName() == config_name)
	    return proxy_snapper.get();
    }

    ProxySnapperLib* ret = new ProxySnapperLib(config_name, target_root);
    proxy_snappers.emplace_back(ret);
    return ret;
}


map<string, ProxyConfig>
ProxySnappersLib::getConfigs() const
{
    map<string, ProxyConfig> ret;

    list<ConfigInfo> config_infos = Snapper::getConfigs(target_root);
    for (const ConfigInfo& config_info : config_infos)
	ret.emplace(make_pair(config_info.getConfigName(), config_info.getAllValues()));

    return ret;
}


ProxyComparisonLib::ProxyComparisonLib(ProxySnapperLib* proxy_snapper, const ProxySnapshot& lhs,
				       const ProxySnapshot& rhs, bool mount)
    : proxy_snapper(proxy_snapper)
{
    comparison.reset(new Comparison(proxy_snapper->snapper.get(), to_lib(lhs).it, to_lib(rhs).it,
				    mount));
}


ProxySnappers
ProxySnappers::createLib(const string& target_root)
{
    return ProxySnappers(new ProxySnappersLib(target_root));
}


const ProxySnapshotLib&
to_lib(const ProxySnapshot& proxy_snapshot)
{
    return dynamic_cast<const ProxySnapshotLib&>(proxy_snapshot.get_impl());
}
