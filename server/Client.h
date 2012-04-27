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


#ifndef SNAPPER_CLIENT_H
#define SNAPPER_CLIENT_H


#include <string>
#include <list>
#include <set>

#include <snapper/Snapper.h>
#include <snapper/Snapshot.h>
#include <snapper/Factory.h>
#include <snapper/Comparison.h>


using namespace std;
using namespace snapper;


struct NoComparison : public std::exception
{
    explicit NoComparison() throw() {}
    virtual const char* what() const throw() { return "no comparison"; }
};


class Client
{
public:

    Client(const string& name);
    ~Client();

    Comparison* find_comparison(const string& config_name, unsigned int number1,
				unsigned int number2);

    Comparison* find_comparison(Snapper* snapper, Snapshots::const_iterator snapshot1,
				Snapshots::const_iterator snapshot2);
    
    void add_lock(const string& config_name);
    void remove_lock(const string& config_name);
    bool has_lock(const string& config_name) const;

    string name;

    list<Comparison*> comparisons;

    set<string> locks;

};


class Clients
{
public:

    typedef list<Client>::iterator iterator;
    typedef list<Client>::const_iterator const_iterator;

    iterator begin() { return entries.begin(); }
    const_iterator begin() const { return entries.begin(); }
    
    iterator end() { return entries.end(); }
    const_iterator end() const { return entries.end(); }

    bool empty() const { return entries.empty(); }

    iterator find(const string& name);

    void add(const string& name);
    void remove(const string& name);

private:

    list<Client> entries;

};
    

#endif
