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


#ifndef SNAPPER_DBUS_PIPE_H
#define SNAPPER_DBUS_PIPE_H


#include <boost/noncopyable.hpp>

#include "DBusMessage.h"


namespace DBus
{

    /*
     * Minimalistic RAII for a file descriptor.
     */
    class FileDescriptor : boost::noncopyable
    {
    public:

	FileDescriptor() = default;
	~FileDescriptor() { close(); }

	void close();

	friend class File;
	friend class Pipe;

	friend Unmarshaller& operator>>(Unmarshaller& marshaller, FileDescriptor& data);
	friend Marshaller& operator<<(Marshaller& marshaller, const FileDescriptor& data);

    private:

	int _fd = -1;

    };


    /*
     * Minimalistic RAII for a FILE*.
     */
    class File
    {
    public:

	File(FileDescriptor& fd, const char* mode);
	~File() { close(); }

	ssize_t getline(char** lineptr, size_t* n);
	int printf(const char* format, ...);
	int eof() { return feof(_file); }
	int flush() { return fflush(_file); }
	int close();

	explicit operator bool() const { return _file; }

    private:

	FILE* _file = nullptr;

    };


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
