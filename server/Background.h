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


#ifndef SNAPPER_BACKGROUND_H
#define SNAPPER_BACKGROUND_H


#include <boost/thread.hpp>

#include "MetaSnapper.h"


using namespace std;
using namespace snapper;


class Backgrounds : private boost::noncopyable
{

public:

    Backgrounds();
    ~Backgrounds();

    struct Task
    {
	Task(MetaSnapper* meta_snapper, Snapshots::const_iterator snapshot1,
	     Snapshots::const_iterator snapshot2)
	    : meta_snapper(meta_snapper), snapshot1(snapshot1), snapshot2(snapshot2) {}

	MetaSnapper* meta_snapper;
	Snapshots::const_iterator snapshot1;
	Snapshots::const_iterator snapshot2;
    };

    typedef list<Task>::const_iterator const_iterator;

    const_iterator begin() const { return tasks.begin(); }

    const_iterator end() const { return tasks.end(); }

    bool empty() const { return tasks.empty(); }

    void add_task(MetaSnapper* meta_snapper, Snapshots::const_iterator snapshot1,
		  Snapshots::const_iterator snapshot2);

private:

    void worker();

    boost::condition_variable condition;
    boost::mutex mutex;
    boost::thread thread;
    list<Task> tasks;

};


extern Backgrounds backgrounds;


#endif
