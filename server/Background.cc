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


#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <snapper/Log.h>
#include <snapper/Comparison.h>

#include "MetaSnapper.h"
#include "Background.h"


Backgrounds backgrounds;


Backgrounds::Backgrounds()
{
}


Backgrounds::~Backgrounds()
{
    thread.interrupt();
    if (thread.joinable())
	thread.join();
}


void
Backgrounds::add_task(MetaSnappers::iterator meta_snapper, Snapshots::const_iterator snapshot1,
		      Snapshots::const_iterator snapshot2)
{
    if (thread.get_id() == boost::thread::id())
	thread = boost::thread(boost::bind(&Backgrounds::worker, this));

    boost::unique_lock<boost::mutex> lock(mutex);
    tasks.push_back(Task(meta_snapper, snapshot1, snapshot2));
    meta_snapper->inc_use_count();
    lock.unlock();

    condition.notify_one();
}


enum {
    IOPRIO_CLASS_NONE,
    IOPRIO_CLASS_RT,
    IOPRIO_CLASS_BE,
    IOPRIO_CLASS_IDLE,
};

enum {
    IOPRIO_WHO_PROCESS = 1,
    IOPRIO_WHO_PGRP,
    IOPRIO_WHO_USER,
};

#define IOPRIO_CLASS_SHIFT (13)
#define IOPRIO_PRIO_MASK ((1UL << IOPRIO_CLASS_SHIFT) - 1)

#define IOPRIO_PRIO_CLASS(mask) ((mask) >> IOPRIO_CLASS_SHIFT)
#define IOPRIO_PRIO_DATA(mask) ((mask) & IOPRIO_PRIO_MASK)
#define IOPRIO_PRIO_VALUE(ioclass, data) (((ioclass) << IOPRIO_CLASS_SHIFT) | data)


void
Backgrounds::worker()
{
    /* According to POSIX threads have the same nice value (see pthreads(7))
       but with linux 3.4 and glibc 2.15 the value can be set per thread. */

    pid_t tid = syscall(SYS_gettid);

    int priority = 20;
    if (setpriority(PRIO_PROCESS, tid, priority) != 0)
    {
	y2war("failed to set priority errno:" << errno);
    }

    int ioclass = IOPRIO_CLASS_IDLE;
    int data = 0;
    if (syscall(SYS_ioprio_set, IOPRIO_WHO_PROCESS, tid, IOPRIO_PRIO_VALUE(ioclass, data)) != 0)
    {
	y2war("failed to set io-priority errno:" << errno);
    }

    try
    {
	while (true)
	{
	    boost::unique_lock<boost::mutex> lock(mutex);
	    while (tasks.empty())
		condition.wait(lock);
	    Task task = tasks.front();
	    lock.unlock();

	    Snapper* snapper = task.meta_snapper->getSnapper();
	    Comparison comparison(snapper, task.snapshot1, task.snapshot2);
	    task.meta_snapper->dec_use_count();

	    lock.lock();
	    tasks.pop_front();
	    lock.unlock();
	}
    }
    catch (const boost::thread_interrupted&)
    {
	y2deb("worker interrupted");
    }
}
