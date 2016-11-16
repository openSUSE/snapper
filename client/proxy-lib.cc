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


#include <sstream>
#include <iostream>

#include "proxy-lib.h"


using namespace std;


void
ProxySnapperLib::setConfigInfo(const map<string, string>& raw)
{
    snapper->setConfigInfo(raw);
}


ProxySnapshots::const_iterator
ProxySnapperLib::createSingleSnapshot(const SCD& scd)
{
    proxy_snapshots.emplace_back(new ProxySnapshotLib(snapper->createSingleSnapshot(scd)));

    return --proxy_snapshots.end();
}


ProxySnapshots::const_iterator
ProxySnapperLib::createPreSnapshot(const SCD& scd)
{
    proxy_snapshots.emplace_back(new ProxySnapshotLib(snapper->createPreSnapshot(scd)));

    return --proxy_snapshots.end();
}


ProxySnapshots::const_iterator
ProxySnapperLib::createPostSnapshot(const ProxySnapshots::const_iterator pre, const SCD& scd)
{
    const ProxySnapshotLib& x = dynamic_cast<const ProxySnapshotLib&>(pre->get_impl());

    proxy_snapshots.emplace_back(new ProxySnapshotLib(snapper->createPostSnapshot(x.it, scd)));

    return --proxy_snapshots.end();
}


void
ProxySnapshotsLib::update()
{
    proxy_snapshots.clear();

    Snapshots& x = backref->snapper->getSnapshots();
    for (Snapshots::const_iterator it = x.begin(); it != x.end(); ++it)
	proxy_snapshots.push_back(new ProxySnapshotLib(it));
}


const ProxySnapshots&
ProxySnapperLib::getSnapshots()
{
    proxy_snapshots.update();

    return proxy_snapshots;
}


void
ProxySnappersLib::createConfig(const string& config_name, const string& subvolume,
			       const string& fstype, const string& template_name)
{
    Snapper::createConfig(config_name, "/", subvolume, fstype, template_name);
}


void
ProxySnappersLib::deleteConfig(const string& config_name)
{
    Snapper::deleteConfig(config_name, "/");
}


ProxySnapper*
ProxySnappersLib::getSnapper(const string& config_name)
{
    for (unique_ptr<ProxySnapperLib>& proxy_snapper : proxy_snappers)
    {
	if (proxy_snapper->snapper->configName() == config_name)
	    return proxy_snapper.get();
    }

    ProxySnapperLib* ret = new ProxySnapperLib(config_name);
    proxy_snappers.push_back(unique_ptr<ProxySnapperLib>(ret));
    return ret;
}
