/*
 * Copyright (c) 2015 Red Hat, Inc.
 * Copyright (c) [2022-2026] SUSE LLC
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
 */


#include <cstdio>

#include "FilesTransferTask.h"


FilesTransferTask::FilesTransferTask(const Files& files)
    : files(files)
{
}


void
FilesTransferTask::run()
{
    DBus::File fout(get_write_end(), "w");
    if (!fout)
	SN_THROW(StreamException());

    for (const File& file : files)
    {
	if (fout.printf("%s %d\n", DBus::Pipe::escape(file.getName()).c_str(), file.getPreToPostStatus()) < 4)
	    SN_THROW(StreamException());
    }

    if (fout.flush() != 0)
	SN_THROW(StreamException());

    if (fout.close() != 0)
	SN_THROW(StreamException());
}
