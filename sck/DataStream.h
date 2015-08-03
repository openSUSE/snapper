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

#include <boost/asio.hpp>

#include "sck/Socket.h"

namespace sck
{
    using boost::asio::io_service;
    using boost::asio::socket_base;
    using boost::asio::local::stream_protocol;

    struct SocketStreamException : public std::exception
    {
	explicit SocketStreamException() throw() {}
	virtual const char* what() const throw() { return "Socket stream exception"; }
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

	virtual ~SocketStream();

    protected:

	SocketStream(const SocketFd& fd);

	io_service _io_service;
	stream_protocol::socket _socket;
	boost::asio::streambuf _data_buf;

    };


    template <class T>
    SocketStream<T>::SocketStream(const SocketFd& fd)
	: _io_service(), _socket(_io_service, stream_protocol(), fd.get_fd()), _data_buf()
    {
    }


    template <class T>
    SocketStream<T>::~SocketStream()
    {
	try
	{
	    if (_socket.is_open())
		_socket.shutdown(socket_base::shutdown_both);
	}
	catch (const boost::system::system_error&)
	{
	}
    }

}


#endif
