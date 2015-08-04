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

#ifndef SNAPPER_PIPE_H
#define SNAPPER_PIPE_H

#include <exception>

#include <boost/noncopyable.hpp>

namespace pipe_stream
{
    struct PipeException : public std::exception
    {
	explicit PipeException() throw() {}
	virtual const char* what() const throw() { return "Socket pair exception"; }
    };


    class FileDescriptor : boost::noncopyable
    {
    public:

	FileDescriptor() : _fd(-1) {}
	FileDescriptor(int fd) : _fd(fd) {}
	~FileDescriptor() { close(); }

	int get_fd() const { return _fd; }
	void set_fd(int fd) { _fd = fd; }
	void close();

    private:

	int _fd;

    };


    class Pipe
    {
    public:
	Pipe();

	FileDescriptor& read_end() { return rs; }
	FileDescriptor& write_end() { return ws; }

    private:

	FileDescriptor rs;
	FileDescriptor ws;

    };
}

#endif
