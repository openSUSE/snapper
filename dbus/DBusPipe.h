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


#ifndef SNAPPER_DBUSPIPE_H
#define SNAPPER_DBUSPIPE_H


#include <boost/noncopyable.hpp>

#include "DBusMessage.h"


namespace DBus
{

    class FileDescriptor : boost::noncopyable
    {
    public:

	FileDescriptor() = default;
	FileDescriptor(int fd) : _fd(fd) {}
	~FileDescriptor() { close(); }

	void close();

	int get_fd() const { return _fd; }
	void set_fd(int fd) { _fd = fd; }

    private:

	int _fd = -1;

    };


    Unmarshaller& operator>>(Unmarshaller& marshaller, FileDescriptor& data);
    Marshaller& operator<<(Marshaller& marshaller, const FileDescriptor& data);


    struct PipeException : public Exception
    {
	explicit PipeException() : Exception("pipe exception") {}
    };


    class Pipe : boost::noncopyable
    {
    public:

	Pipe();

	FileDescriptor& get_read_end() { return _read_end; }
	FileDescriptor& get_write_end() { return _write_end; }

	static string escape(const string&);
	static string unescape(const string&);

    private:

	FileDescriptor _read_end;
	FileDescriptor _write_end;

    };

}

#endif
