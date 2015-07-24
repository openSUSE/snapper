/*
 * Copyright (c) [2015] Red Hat, Inc.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "FilesTransferTask.h"
#include "FileSerialization.h"

#include <boost/serialization/vector.hpp>

#define BUF_SIZE 65536

FilesTransferTask::FilesTransferTask(XComparison& xcmp)
    : RefHolder(xcmp), xcmp(xcmp), pair(), ws(pair.write_socket(), boost::bind(&FilesTransferTask::append_next, this))
{
}


void
FilesTransferTask::append_next()
{
    if (cit != xcmp.cmp->getFiles().end())
    {
	vector<const File *> vec;

	if(std::distance(cit, xcmp.cmp->getFiles().end()) > BUF_SIZE)
	{
	    std::transform(cit, cit + BUF_SIZE, std::back_inserter(vec), FilesTransferTask::addressor());
	    cit += BUF_SIZE;
	}
	else
	{
	    std::transform(cit, xcmp.cmp->getFiles().end(), std::back_inserter(vec), FilesTransferTask::addressor());
	    cit = xcmp.cmp->getFiles().end();
	}

	ws.append(vec);
    }

    ws.async_write();
}


void
FilesTransferTask::init()
{
    cit = xcmp.cmp->getFiles().begin();

    append_next();
}

