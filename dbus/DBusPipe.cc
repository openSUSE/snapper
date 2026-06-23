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


#include <fcntl.h>
#include <unistd.h>

#include "DBusPipe.h"
#include "DBusMessage.h"


namespace DBus
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


    Unmarshaller&
    operator>>(Unmarshaller& unmarshaller, FileDescriptor& data)
    {
	if (unmarshaller.get_type() != DBUS_TYPE_UNIX_FD)
	    SN_THROW(MarshallingException());

	dbus_message_iter_get_basic(unmarshaller.top(), &data._fd);
	dbus_message_iter_next(unmarshaller.top());

	return unmarshaller;
    }


    Marshaller&
    operator<<(Marshaller& marshaller, const FileDescriptor& data)
    {
	if (!dbus_message_iter_append_basic(marshaller.top(), DBUS_TYPE_UNIX_FD, &data._fd))
	    SN_THROW(FatalException());

	return marshaller;
    }


    File::File(FileDescriptor& fd, const char* mode)
	: _file(fdopen(fd._fd, mode))
    {
	if (_file)
	    fd._fd = -1;
    }


    ssize_t
    File::getline(char** lineptr, size_t* n)
    {
	return ::getline(lineptr, n, _file);
    }


    int
    File::printf(const char* format, ...)
    {
	va_list args;
	va_start(args, format);
	int ret = vfprintf(_file, format, args);
	va_end(args);
	return ret;
    }


    int
    File::close()
    {
	int ret = 0;

	if (_file != nullptr)
	{
	    ret = fclose(_file);
	    if (ret != -1)
		_file = nullptr;
	}

	return ret;
    }


    Pipe::Pipe()
    {
	int pipefd[2];

	if (pipe2(pipefd, O_CLOEXEC) != 0)
	    SN_THROW(PipeException());

	_read_end._fd = pipefd[0];
	_write_end._fd = pipefd[1];
    }


    string
    Pipe::escape(const string& in)
    {
	string out;

	for (const char c : in)
	{
	    if (c == '\\')
	    {
		out += "\\\\";
	    }
	    else if ((unsigned char)(c) <= ' ')
	    {
		char s[5];
		snprintf(s, 5, "\\x%02x", (unsigned char)(c));
		out += string(s);
	    }
	    else
	    {
		out += c;
	    }
	}

	return out;
    }


    string
    Pipe::unescape(const string& in)
    {
	return Unmarshaller::unescape(in);
    }

}
