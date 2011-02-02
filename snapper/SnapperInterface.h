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


namespace snapper
{
    using std::string;


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


    // use num = 0 for current system

    void startBackgroundComparsion(unsigned int num1, unsigned int num2);

    bool setComparisonNums(unsigned int num1, unsigned int num2);

    unsigned int getComparisonNum1();
    unsigned int getComparisonNum2();


    // return bitfield of StatusFlags
    unsigned int getStatus(const string& name, Cmp cmp);

    string getAbsolutePath(const string& name, Location loc);

    void setRollback(const string& name, bool rollback);
    bool getRollback(const string& name);

    // check rollback? (e.g. to be deleted dirs are empty, required type changes)
    bool checkRollback();

    bool doRollback();


    // progress callbacks, e.g. during snapshot comparision, during rollback,
    // errors during rollback

    struct CompareCallback
    {
	CompareCallback() {}
	virtual ~CompareCallback() {}

	virtual void start() {}
	virtual void stop() {}
    };

    void setCompareCallback(CompareCallback* p);


}


#endif
