/*
 * Copyright (c) [2012-2015] Novell, Inc.
 * Copyright (c) [2018-2024] SUSE LLC
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


#ifndef SNAPPER_META_SNAPPER_H
#define SNAPPER_META_SNAPPER_H


#include <boost/thread.hpp>

#include <snapper/Snapper.h>

#include "RefCounter.h"


using namespace std;
using namespace snapper;


class Clients;


struct UnknownConfig : public Exception
{
    explicit UnknownConfig() : Exception("unknown config") {}
};


class MetaSnapper : public RefCounter
{
public:

    MetaSnapper(const ConfigInfo& config_info);
    ~MetaSnapper();

    const string& configName() const { return config_info.get_config_name(); }

    const ConfigInfo& getConfigInfo() const { return config_info; }
    void setConfigInfo(const map<string, string>& raw);

    void updateConfigInfo(const char* key);

    const vector<uid_t>& get_allowed_uids() const { return allowed_uids; }
    const vector<gid_t>& get_allowed_gids() const { return allowed_gids; }

    Snapper* getSnapper();

    bool is_equal(const Snapper* s) { return snapper && snapper == s; }
    bool is_loaded() const { return snapper; }
    void unload();

    bool is_locked(const Clients& clients) const;

private:

    void set_permissions();

    ConfigInfo config_info;

    Snapper* snapper = nullptr;

    vector<uid_t> allowed_uids;
    vector<gid_t> allowed_gids;

};


class MetaSnappers
{

public:

    MetaSnappers();
    ~MetaSnappers();

    void init();

    void unload();

    typedef list<MetaSnapper>::iterator iterator;
    typedef list<MetaSnapper>::const_iterator const_iterator;

    iterator begin() { return entries.begin(); }
    const_iterator begin() const { return entries.begin(); }

    iterator end() { return entries.end(); }
    const_iterator end() const { return entries.end(); }

    bool empty() const { return entries.empty(); }

    iterator find(const string& config_name);

    void createConfig(const string& config_name, const string& subvolume, const string& fstype,
		      const string& template_name, Plugins::Report& report);
    void deleteConfig(iterator, Plugins::Report& report);

private:

    list<MetaSnapper> entries;

};


extern MetaSnappers meta_snappers;


#endif
