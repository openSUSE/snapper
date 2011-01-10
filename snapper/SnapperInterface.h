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


#ifndef SNAPPER_INTERFACE_H
#define SNAPPER_INTERFACE_H


#include <string>
#include <map>
#include <list>


namespace snapper
{
    using std::string;
    using std::map;
    using std::list;


    enum SnapshotType { SINGLE, PRE, POST };


    class Snapshot
    {
    public:

	Snapshot() : type(SINGLE), pre_num(0) {}

	SnapshotType type;

	string date;

	string description;	// empty for type=POST

	unsigned int pre_num;	// valid only for type=POST

    };


    const map<unsigned int, Snapshot>& getSnapshots();

    void listSnapshots();	// only for testing


    unsigned int createSingleSnapshot(string description);

    unsigned int createPreSnapshot(string description);

    unsigned int createPostSnapshot(unsigned int pre_num);

}


#endif
