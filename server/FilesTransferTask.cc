/*
 * Copyright (c) 2015 Red Hat, Inc.
 * Copyright (c) 2022 SUSE LLC
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


#include <stdio.h>

#include "FilesTransferTask.h"


FilesTransferTask::FilesTransferTask(const Files& files)
    : files(files)
{
}


void
FilesTransferTask::run()
{
    FILE* fout = fdopen(get_write_end().get_fd(), "w");
    if (!fout)
	SN_THROW(StreamException());

    for (const File& file : files)
    {
	if (fprintf(fout, "%s %d\n", DBus::Pipe::escape(file.getName()).c_str(), file.getPreToPostStatus()) < 4)
	    SN_THROW(StreamException());
    }

    if (fflush(fout) != 0)
	SN_THROW(StreamException());

    if (fclose(fout) != 0)
	SN_THROW(StreamException());
}
