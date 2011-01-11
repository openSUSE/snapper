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


#include "snapper/SnapperInterface.h"


namespace snapper
{
    using namespace std;


    extern bool initialized;


    extern list<Snapshot> snapshots;


    extern Snapshot& snapshot1;
    extern Snapshot& snapshot2;

    extern list<string> files;


    struct Statuses
    {
	unsigned int pre_to_post;
	unsigned int pre_to_system;
	unsigned int post_to_system;
    };

    extern map<string, Statuses> statuses;

    extern list<string> files_to_rollback;

};


#endif
