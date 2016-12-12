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
#include <vector>
#include <list>
#include <map>

#include <snapper/Snapshot.h>
#include <snapper/Snapper.h>
#include <snapper/File.h>


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


class ProxyConfig
{

public:

    ProxyConfig(const map<string, string>& values) : values(values) {}

    const map<string, string>& getAllValues() const { return values; }

    string getSubvolume() const;

    bool getValue(const string& key, string& value) const;

protected:

    map<string, string> values;

};


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

    void mountFilesystemSnapshot(bool user_request) const {
	impl->mountFilesystemSnapshot(user_request);
    }

    void umountFilesystemSnapshot(bool user_request) const {
	impl->umountFilesystemSnapshot(user_request);
    }

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

	virtual string mountFilesystemSnapshot(bool user_request) const = 0;
	virtual void umountFilesystemSnapshot(bool user_request) const = 0;

    };

    ProxySnapshot(Impl* impl) : impl(impl) {}

    Impl& get_impl() { return *impl; }
    const Impl& get_impl() const { return *impl; }

private:

    std::unique_ptr<Impl> impl;

};


class ProxySnapshots
{

public:

    virtual ~ProxySnapshots() {}

    typedef list<ProxySnapshot>::iterator iterator;
    typedef list<ProxySnapshot>::const_iterator const_iterator;
    typedef list<ProxySnapshot>::size_type size_type;

    iterator begin() { return proxy_snapshots.begin(); }
    const_iterator begin() const { return proxy_snapshots.begin(); }

    iterator end() { return proxy_snapshots.end(); }
    const_iterator end() const { return proxy_snapshots.end(); }

    iterator find(unsigned int i);
    const_iterator find(unsigned int i) const;

    iterator findNum(const string& str);
    const_iterator findNum(const string& str) const;

    std::pair<iterator, iterator> findNums(const string& str, const string& delim = "..");

    const_iterator findPre(const_iterator post) const;

    iterator findPost(iterator pre);
    const_iterator findPost(const_iterator pre) const;

    void emplace_back(ProxySnapshot::Impl* value) { proxy_snapshots.emplace_back(value); }

protected:

    virtual void update() = 0;

    list<ProxySnapshot> proxy_snapshots;

};


class ProxyComparison;


class ProxySnapper
{

public:

    virtual ~ProxySnapper() {}

    virtual const string& configName() const = 0;

    virtual ProxyConfig getConfig() const = 0;
    virtual void setConfig(const ProxyConfig& proxy_config) = 0;

    virtual ProxySnapshots::const_iterator createSingleSnapshot(const SCD& scd) = 0;
    virtual ProxySnapshots::const_iterator createPreSnapshot(const SCD& scd) = 0;
    virtual ProxySnapshots::const_iterator createPostSnapshot(ProxySnapshots::const_iterator pre, const SCD& scd) = 0;

    virtual void modifySnapshot(ProxySnapshots::iterator snapshot, const SMD& smd) = 0;

    virtual void deleteSnapshots(list<ProxySnapshots::iterator> snapshots, bool verbose) = 0;

    virtual ProxyComparison createComparison(const ProxySnapshot& lhs, const ProxySnapshot& rhs, bool mount) = 0;

    virtual void syncFilesystem() const = 0;

    virtual ProxySnapshots& getSnapshots() = 0;

    virtual void setupQuota() = 0;

    virtual void prepareQuota() const = 0;

    virtual QuotaData queryQuotaData() const = 0;

};


class ProxySnappers
{

public:

    static ProxySnappers createDbus();
    static ProxySnappers createLib(const string& target_root);

    void createConfig(const string& config_name, const string& subvolume,
		      const string& fstype, const string& template_name)
	{ return impl->createConfig(config_name, subvolume, fstype, template_name); }

    void deleteConfig(const string& config_name)
	{ return impl->deleteConfig(config_name); }

    ProxySnapper* getSnapper(const string& config_name)
	{ return impl->getSnapper(config_name); }

    map<string, ProxyConfig> getConfigs() const
	{ return impl->getConfigs(); }

    std::vector<string> debug() const
	{ return impl->debug(); }

public:

    class Impl
    {

    public:

	virtual ~Impl() {}

	virtual void createConfig(const string& config_name, const string& subvolume,
				  const string& fstype, const string& template_name) = 0;

	virtual void deleteConfig(const string& config_name) = 0;

	virtual ProxySnapper* getSnapper(const string& config_name) = 0;

	virtual map<string, ProxyConfig> getConfigs() const = 0;

	virtual std::vector<string> debug() const = 0;

    };

    ProxySnappers(Impl* impl) : impl(impl) {}

    Impl& get_impl() { return *impl; }
    const Impl& get_impl() const { return *impl; }

private:

    std::unique_ptr<Impl> impl;

};


class ProxyComparison
{

public:

    const Files& getFiles() const { return impl->getFiles(); }

public:

    class Impl
    {

    public:

	virtual ~Impl() {}

	virtual const Files& getFiles() const = 0;

    };

    ProxyComparison(Impl* impl) : impl(impl) {}

    Impl& get_impl() { return *impl; }
    const Impl& get_impl() const { return *impl; }

private:

    std::unique_ptr<Impl> impl;

};


#endif
