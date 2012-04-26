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


#ifndef SNAPPER_JOB_H
#define SNAPPER_JOB_H


#include <list>
#include <boost/thread.hpp>


using namespace std;
using namespace boost::posix_time;


struct Job
{
public:

    virtual ~Job() {}

    void start() { t = boost::thread(boost::ref(*this)); }

    bool is_running() { return !t.timed_join(seconds(0)); }

    virtual void done() = 0;

    virtual void operator()() = 0;

protected:

    boost::thread t;

};


class Jobs
{
public:

    void add(Job* job);

    void handle();

    size_t size() const { return entries.size(); }
    bool empty() const { return entries.empty(); }

private:

    list<Job*> entries;

};


#endif
