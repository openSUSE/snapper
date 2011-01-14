/*
 * Copyright (c) 2011 Novell, Inc.
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


#ifndef SNAPPER_H
#define SNAPPER_H


#include <vector>

#include "snapper/SnapperInterface.h"


namespace snapper
{
    using namespace std;


    extern bool initialized;


    inline bool operator<(const Snapshot& a, const Snapshot& b)
    {
	return a.num < b.num;
    }

    extern list<Snapshot> snapshots;


    extern Snapshot snapshot1;
    extern Snapshot snapshot2;


    class File
    {
    public:

	File(const string& name, unsigned int pre_to_post_status)
	    : name(name), pre_to_post_status(pre_to_post_status), pre_to_system_status(-1),
	      post_to_system_status(-1), rollback(false)
	{}

	string name;

	unsigned int pre_to_post_status; // -1 if invalid
	unsigned int pre_to_system_status; // -1 if invalid
	unsigned int post_to_system_status; // -1 if invalid

	bool rollback;

    };


    inline int operator<(const File& a, const File& b)
    {
	return a.name < b.name;
    }


    class Filelist
    {
    public:

	Filelist() : initialized(false) {}

	void assertInit();

	vector<File>::const_iterator begin() const { return files.begin(); }
	vector<File>::const_iterator end() const { return files.end(); }

	vector<File>::iterator find(const string& name);
	vector<File>::const_iterator find(const string& name) const;

	void append(const string& name, unsigned int status); // should be private

    private:

	void initialize();

	void create();
	bool load();
	bool save();

	bool initialized;

	vector<File> files;

    };

    extern Filelist filelist;


};


#endif
