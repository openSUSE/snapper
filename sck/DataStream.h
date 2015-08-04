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

#ifndef SNAPPER_DATASTREAM_H
#define SNAPPER_DATASTREAM_H

#include <exception>
#include <string>

#include <boost/asio.hpp>

#include "sck/Socket.h"

namespace sck
{
    using std::string;
    using boost::asio::io_service;

    struct SocketStreamException : public std::exception
    {
	explicit SocketStreamException() throw() : msg("Stream exception") {}
	explicit SocketStreamException(const char* msg) throw() : msg(msg) {}
	virtual const char* what() const throw() { return msg.c_str(); }
	const string msg;
    };

    struct SocketStreamSerializationException : public SocketStreamException
    {
	explicit SocketStreamSerializationException() throw() {}
	virtual const char* what() const throw() { return "Serialization exception"; }
    };


    template <class T>
    class SocketStream
    {
    public:

	virtual ~SocketStream() {}

    protected:

	SocketStream(): _io_service(), _data_buf() {}

	io_service _io_service;
	boost::asio::streambuf _data_buf;

    };

}


#endif
