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

#ifndef SNAPPER_SOCKET_H
#define SNAPPER_SOCKET_H

#include <exception>

#include <boost/noncopyable.hpp>

namespace sck
{
    struct SocketPairException : public std::exception
    {
	explicit SocketPairException() throw() {}
	virtual const char* what() const throw() { return "Socket pair exception"; }
    };


    class SocketFd : boost::noncopyable
    {
    public:

	SocketFd() : _fd(-1) {}
	SocketFd(int fd) : _fd(fd) {}
	~SocketFd() { close(); }

	int get_fd() const { return _fd; }
	void set_fd(int fd) { _fd = fd; }
	void close();

    private:

	int _fd;

    };


    class SocketPair
    {
    public:

	SocketPair();

	SocketFd& read_socket() { return rs; }
	SocketFd& write_socket() { return ws; }

    private:

	SocketFd rs;
	SocketFd ws;

    };

    class Pipe
    {
    public:
	Pipe();

	SocketFd& read_socket() { return rs; }
	SocketFd& write_socket() { return ws; }

    private:

	SocketFd rs;
	SocketFd ws;

    };
}

#endif
