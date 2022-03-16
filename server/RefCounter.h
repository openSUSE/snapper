/*
 * Copyright (c) [2012-2015] Novell, Inc.
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

#ifndef SNAPPER_REF_COUNTER_H
#define SNAPPER_REF_COUNTER_H


#include <chrono>
#include <boost/thread.hpp>
#include <boost/noncopyable.hpp>


using namespace std::chrono;


class RefCounter : private boost::noncopyable
{
public:

    RefCounter();

    int inc_use_count();
    int dec_use_count();
    void update_use_time();

    int use_count() const;
    milliseconds unused_for() const;

private:

    mutable boost::mutex mutex;

    int counter = 0;

    steady_clock::time_point last_used;

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


#endif
