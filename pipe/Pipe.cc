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

#include <fcntl.h>
#include <unistd.h>

#include "pipe/Pipe.h"

namespace pipe_stream
{
    void
    FileDescriptor::close()
    {
	if (_fd != -1)
	{
	    ::close(_fd);
	    _fd = -1;
	}
    }


    Pipe::Pipe()
	: rs(), ws()
    {
	int pipefd[2];

	if (pipe(pipefd))
	{
	    throw PipeException();
	}

	// FIXME: of a race is found to be real issue, use pipe2 instead
	if (fcntl(pipefd[0], F_SETFD, FD_CLOEXEC) < 0 || fcntl(pipefd[1], F_SETFD, FD_CLOEXEC) < 0)
	{
	    throw PipeException();
	}

	rs.set_fd(pipefd[0]);
	ws.set_fd(pipefd[1]);
    }

}
