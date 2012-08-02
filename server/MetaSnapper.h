/*
 * Copyright (c) 2012 Novell, Inc.
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
#include <snapper/Snapshot.h>
#include <snapper/Comparison.h>


using namespace std;
using namespace snapper;


class RefCounter : boost::noncopyable
{
public:

    RefCounter();

    int inc_use_count();
    int dec_use_count();
    void update_use_time();

    int use_count() const;
    int unused_for() const;

private:

    mutable boost::mutex mutex;

    int counter;

    time_t last_used;

};


class RefHolder
{
public:

    RefHolder(RefCounter& ref) : ref(ref)
	{ ref.inc_use_count(); }
    ~RefHolder()
	{ ref.dec_use_count(); }

private:

    RefCounter& ref;

};


struct UnknownConfig : public std::exception
{
    explicit UnknownConfig() throw() {}
    virtual const char* what() const throw() { return "unknown config"; }
};


class MetaSnapper : public RefCounter
{
public:

    MetaSnapper(const ConfigInfo& config_info);
    ~MetaSnapper();

    const string& configName() const { return config_info.config_name; }

    const ConfigInfo config_info;

    vector<uid_t> uids;

    Snapper* getSnapper();

    bool is_equal(const Snapper* s) { return snapper && snapper == s; }
    bool is_loaded() const { return snapper; }
    void unload();

private:

    Snapper* snapper;

};


class MetaSnappers
{

public:

    MetaSnappers();
    ~MetaSnappers();

    void init();

    typedef list<MetaSnapper>::iterator iterator;
    typedef list<MetaSnapper>::const_iterator const_iterator;

    iterator begin() { return entries.begin(); }
    const_iterator begin() const { return entries.begin(); }

    iterator end() { return entries.end(); }
    const_iterator end() const { return entries.end(); }

    bool empty() const { return entries.empty(); }

    iterator find(const string& config_name);

    void createConfig(const string& config_name, const string& subvolume, const string& fstype,
		      const string& template_name);
    void deleteConfig(iterator);

private:

    list<MetaSnapper> entries;

};


extern MetaSnappers meta_snappers;


#endif
