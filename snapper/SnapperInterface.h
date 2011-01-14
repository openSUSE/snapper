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

	Snapshot() : type(SINGLE), num(0), pre_num(0) {}

	SnapshotType type;

	unsigned int num;

	string date;

	string description;	// empty for type=POST

	unsigned int pre_num;	// valid only for type=POST

    };


    bool getSnapshot(unsigned int num, Snapshot& snapshot);

    const list<Snapshot>& getSnapshots();

    void listSnapshots();	// only for testing


    unsigned int createSingleSnapshot(string description);

    unsigned int createPreSnapshot(string description);

    unsigned int createPostSnapshot(unsigned int pre_num);



    enum StatusFlags
    {
	CREATED = 1, DELETED = 2, TYPE = 4, CONTENT = 8, PERMISSIONS = 16, USER = 32,
	GROUP = 64
    };

    enum Cmp
    {
	CMP_PRE_TO_POST, CMP_PRE_TO_SYSTEM, CMP_POST_TO_SYSTEM
    };

    enum Location
    {
	LOC_PRE, LOC_POST, LOC_SYSTEM
    };


    // use num = 0 for system

    void startBackgroundComparsion(unsigned int num1, unsigned int num2);

    bool setComparisonNums(unsigned int num1, unsigned int num2);

    unsigned int getComparisonNum1();
    unsigned int getComparisonNum2();

    const list<string>& getFiles();

    // return bitfield of StatusFlags
    unsigned int getStatus(const string& file, Cmp cmp);

    string getAbsolutePath(const string& file, Location loc);

    void setRollback(const string& file, bool rollback);
    bool getRollback(const string& file);

    // check rollback? (e.g. to be deleted dirs are empty, required type changes)
    bool checkRollback();

    bool doRollback();


    // progress callbacks, e.g. during snapshot comparision

}


#endif
