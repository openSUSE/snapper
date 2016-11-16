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


#ifndef SNAPPER_PROXY_H
#define SNAPPER_PROXY_H


#include <memory>

#include <snapper/Snapshot.h>


using namespace snapper;


/**
 * The four proxy classes here allow clients, so far only the snapper command
 * line interface, to work with and without DBus in a transparent way by
 * providing the same interface in both cases.
 *
 * The main idea for providing the same interface is to have an abstract
 * class, e.g. ProxySnapper, and two concrete implementation ProxySnapperDbus
 * and ProxySnapperLib. This is done for ProxySnapshots, ProxySnapper and
 * ProxySnappers.
 *
 * For ProxySnapshot the implementation is more complicated since
 * ProxySnapshots provides an interface to list<ProxySnapshot> and thus
 * polymorphism does not work. Instead ProxySnapshot has an implementation
 * pointer (pimpl idiom) which ensures polymorphism. Another possibility would
 * be to provide an interface to list<ProxySnapshot*> in ProxySnapshots but
 * that is less intuitive to use.
 *
 * All objects are stored in containers or smart pointers to avoid manual
 * cleanup.
 *
 * The interface copycats the classes Snapshot, Snapshots and Snapper of
 * libsnapper. For consistency one may add a Snappers class to libsnapper.
 *
 * Limitations: The classes are tailored for the snapper command line
 * interface. Other use-cases are not supported.
 */


// TODO add namespace proxy and shorter class names?

// TODO maybe unique error handling, e.g. catch dbus exceptions and throw
// snapper or new exceptions


class ProxySnapshot
{

public:

    SnapshotType getType() const { return impl->getType(); }
    unsigned int getNum() const { return impl->getNum(); }
    time_t getDate() const { return impl->getDate(); }
    uid_t getUid() const { return impl->getUid(); }
    unsigned int getPreNum() const { return impl->getPreNum(); }
    const string& getDescription() const { return impl->getDescription(); }
    const string& getCleanup() const { return impl->getCleanup(); }
    const map<string, string>& getUserdata() const { return impl->getUserdata(); }

    bool isCurrent() const { return impl->isCurrent(); }

public:

    class Impl
    {

    public:

	virtual ~Impl() {}

	virtual SnapshotType getType() const = 0;
	virtual unsigned int getNum() const = 0;
	virtual time_t getDate() const = 0;
	virtual uid_t getUid() const = 0;
	virtual unsigned int getPreNum() const = 0;
	virtual const string& getDescription() const = 0;
	virtual const string& getCleanup() const = 0;
	virtual const map<string, string>& getUserdata() const = 0;

	virtual bool isCurrent() const = 0;

    };

    ProxySnapshot(Impl* impl) : impl(impl) {}

    const Impl& get_impl() const { return *impl; }

private:

    std::unique_ptr<Impl> impl;

};


class ProxySnapshots
{

public:

    virtual ~ProxySnapshots() {}

    typedef list<ProxySnapshot>::const_iterator const_iterator;
    typedef list<ProxySnapshot>::size_type size_type;

    const_iterator begin() const { return proxy_snapshots.begin(); }
    const_iterator end() const { return proxy_snapshots.end(); }

    const_iterator findNum(const string& str) const;

    const_iterator findPost(const_iterator pre) const;

    void emplace_back(ProxySnapshot::Impl* value) { proxy_snapshots.emplace_back(value); }

protected:

    virtual void update() = 0;

    list<ProxySnapshot> proxy_snapshots;

};


class ProxySnapper
{

public:

    virtual ~ProxySnapper() {}

    virtual void setConfigInfo(const map<string, string>& raw) = 0;

    virtual ProxySnapshots::const_iterator createSingleSnapshot(const SCD& scd) = 0;
    virtual ProxySnapshots::const_iterator createPreSnapshot(const SCD& scd) = 0;
    virtual ProxySnapshots::const_iterator createPostSnapshot(const ProxySnapshots::const_iterator pre, const SCD& scd) = 0;

    virtual const ProxySnapshots& getSnapshots() = 0;

};


class ProxySnappers
{

public:

    virtual ~ProxySnappers() {}

    virtual void createConfig(const string& config_name, const string& subvolume,
			      const string& fstype, const string& template_name) = 0;

    virtual void deleteConfig(const string& config_name) = 0;

    virtual ProxySnapper* getSnapper(const string& config_name) = 0;

};


#endif
