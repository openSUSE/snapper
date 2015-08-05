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

#ifndef SNAPPER_ASYNC_WRITE_STREAM_H
#define SNAPPER_ASYNC_WRITE_STREAM_H

#include <cstddef>
#include <iostream>

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include <boost/archive/text_oarchive.hpp>

#include "pipe/Pipe.h"
#include "pipe/DataStream.h"

namespace pipe_stream
{
    using boost::asio::posix::stream_descriptor;

    typedef boost::function<void()> callback;

    template <class T>
    class AsyncWriteStream : public BaseStream<T>
    {
    public:

	AsyncWriteStream(const FileDescriptor& fd, callback cb);

	void async_write();
	void append(const T& data);
	void run();

    private:

	void handle_write(const boost::system::error_code& ec);
	void terminate();

	callback _cb;
	stream_descriptor _pipe;

    };


    template <class T>
    AsyncWriteStream<T>::AsyncWriteStream(const FileDescriptor& fd, callback cb)
	: BaseStream<T>(), _cb(cb), _pipe(this->_io_service, fd.get_fd())
    {
	assert(cb != NULL);
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
	    throw StreamSerializationException();
	}
    }


    template <class T>
    void AsyncWriteStream<T>::terminate()
    {
	const size_t terminator = 0;

	try
	{
	    boost::asio::write(this->_pipe, boost::asio::buffer(&terminator, sizeof(terminator)));
	}
	catch (const boost::system::system_error& e)
	{
	    throw StreamException(e.what());
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

	    boost::asio::async_write(this->_pipe,
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
	    throw StreamException(boost::system::system_error(ec).what());
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
	    throw StreamException(e.what());
	}
    }

}

#endif

