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
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "sck/Socket.h"

namespace sck
{
    void
    SocketFd::close()
    {
	if (_fd != -1)
	{
	    ::close(_fd);
	    _fd = -1;
	}
    }


    SocketPair::SocketPair()
	: rs(), ws()
    {
	int sv[2];

	if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sv))
	{
	    throw SocketPairException();
	}

	rs.set_fd(sv[0]);
	ws.set_fd(sv[1]);
    }


    Pipe::Pipe()
	: rs(), ws()
    {
	int pipefd[2];

	if (pipe(pipefd))
	{
	    throw SocketPairException();
	}

	// fcntl(pipefd[1], F_SETPIPE_SZ, ???)
	if (fcntl(pipefd[0], F_SETFD, FD_CLOEXEC) < 0 || fcntl(pipefd[1], F_SETFD, FD_CLOEXEC) < 0)
	{
	    throw SocketPairException();
	}

	rs.set_fd(pipefd[0]);
	ws.set_fd(pipefd[1]);
    }

}
