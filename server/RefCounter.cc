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

#include "RefCounter.h"

RefCounter::RefCounter()
    : counter(0), last_used(steady_clock::now())
{
}


int
RefCounter::inc_use_count()
{
    boost::lock_guard<boost::mutex> lock(mutex);

    return ++counter;
}


int
RefCounter::dec_use_count()
{
    boost::lock_guard<boost::mutex> lock(mutex);

    assert(counter > 0);

    if (--counter == 0)
	last_used = steady_clock::now();

    return counter;
}


void
RefCounter::update_use_time()
{
    boost::lock_guard<boost::mutex> lock(mutex);

    last_used = steady_clock::now();
}


int
RefCounter::use_count() const
{
    boost::lock_guard<boost::mutex> lock(mutex);

    return counter;
}


milliseconds
RefCounter::unused_for() const
{
    boost::lock_guard<boost::mutex> lock(mutex);

    if (counter != 0)
	return milliseconds(0);

    return duration_cast<milliseconds>(steady_clock::now() - last_used);
}
