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

#include <cstddef>

#include <vector>
#include <exception>
#include <iostream>

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/function.hpp>

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

    typedef boost::function<void()> callback;


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
    class AsyncWriteStream : public SocketStream<T>
    {
    public:

	AsyncWriteStream(const SocketFd& fd, callback cb);

	void async_write();
	void append(const T& data);
	void run();

    private:

	void handle_write(const boost::system::error_code& ec);
	void terminate();
	callback _cb;

    };


    template <class T>
    class ReadStream : public SocketStream<T>
    {
    public:

	ReadStream(const SocketFd& fd);

	bool incoming();
	T receive();

    private:

	size_t header;

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


    template <class T>
    AsyncWriteStream<T>::AsyncWriteStream(const SocketFd& fd, callback cb)
	: SocketStream<T>(fd), _cb(cb)
    {
	assert(cb != NULL);

	try
	{
	    // will not read receive any data from socket
	    this->_socket.shutdown(socket_base::shutdown_receive);
	}
	catch (const boost::system::system_error&)
	{
	}
    }


    template <class T>
    void AsyncWriteStream<T>::append(const T& data)
    {
	std::ostream oss(&this->_data_buf);

	try
	{
	    boost::archive::text_oarchive oar(oss);
	    oar << data;
	}
	catch (const boost::archive::archive_exception& e)
	{
	    throw SocketStreamSerializationException();
	}
    }


    template <class T>
    void AsyncWriteStream<T>::terminate()
    {
	const size_t terminator = 0;
	try
	{
	    boost::asio::write(this->_socket, boost::asio::buffer(&terminator, sizeof(terminator)));
	}
	catch (const boost::system::system_error& e)
	{
	    throw SocketStreamException();
	}
    }


    template <class T>
    void AsyncWriteStream<T>::async_write()
    {
	size_t header = this->_data_buf.size();

	if (header > 0)
	{
	    std::vector<boost::asio::const_buffer> buffers;

	    buffers.push_back(boost::asio::buffer(&header, sizeof(header)));
	    buffers.push_back(this->_data_buf.data());

	    boost::asio::async_write(this->_socket,
				     buffers,
				     boost::bind(
					&AsyncWriteStream<T>::handle_write,
					this,
					boost::asio::placeholders::error
				    ));
	}
	else
	{
	    terminate();
	}
    }


    template <class T>
    void AsyncWriteStream<T>::handle_write(const boost::system::error_code& ec)
    {
	if (!ec)
	{
	    this->_data_buf.consume(this->_data_buf.size());
	    _cb(); // callback for next batch to send
	}
	else
	{
	    throw SocketStreamException();
	}
    }


    template <class T>
    void AsyncWriteStream<T>::run()
    {
	try
	{
	    this->_io_service.run();
	}
	catch (const boost::system::system_error& e)
	{
	    throw SocketStreamException();
	}
    }


    template <class T>
    ReadStream<T>::ReadStream(const SocketFd& fd)
	: SocketStream<T>(fd), header(0)
    {
	try
	{
	    // will not send any data via socket
	    this->_socket.shutdown(socket_base::shutdown_send);
	}
	catch (const boost::system::system_error&)
	{
	}
    }


    template <class T>
    bool ReadStream<T>::incoming()
    {
	try
	{
	    boost::asio::read(this->_socket, boost::asio::buffer(&header, sizeof(header)));

	    if (header > 0)
	    {
		boost::asio::read(this->_socket, this->_data_buf.prepare(header));
		this->_data_buf.commit(header);
	    }
	}
	catch (const boost::system::system_error& e)
	{
	    throw SocketStreamException();
	}

	return header > 0;
    }


    template <class T>
    T ReadStream<T>::receive()
    {
	T t;
	std::istream is(&this->_data_buf);

	try
	{
	    boost::archive::text_iarchive iar(is);
	    iar >> t;
	}
	catch (const boost::archive::archive_exception& e)
	{
	    throw SocketStreamSerializationException();
	}

	this->_data_buf.consume(header);

	return t;
    }

}


#endif
