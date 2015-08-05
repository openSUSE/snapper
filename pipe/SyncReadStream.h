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

#ifndef SNAPPER_SYNC_READ_STREAM_H
#define SNAPPER_SYNC_READ_STREAM_H

#include <cstddef>
#include <iostream>

#include <boost/archive/text_iarchive.hpp>

#include "pipe/Pipe.h"
#include "pipe/DataStream.h"

namespace pipe_stream
{
    using boost::asio::posix::stream_descriptor;

    template <class T>
    class SyncReadStream : public BaseStream<T>
    {
    public:

	SyncReadStream(const FileDescriptor& fd);

	bool incoming();
	T receive();

    private:

	stream_descriptor _pipe;
	size_t header;

    };


    template <class T>
    SyncReadStream<T>::SyncReadStream(const FileDescriptor& fd)
	: BaseStream<T>(), _pipe(this->_io_service, fd.get_fd()),  header(0)
    {
    }


    template <class T>
    bool SyncReadStream<T>::incoming()
    {
	try
	{
	    boost::asio::read(this->_pipe, boost::asio::buffer(&header, sizeof(header)));

	    if (header > 0)
	    {
		boost::asio::read(this->_pipe, this->_data_buf.prepare(header));
		this->_data_buf.commit(header);
	    }
	}
	catch (const boost::system::system_error& e)
	{
	    throw StreamException(boost::system::system_error(e).what());
	}

	return header > 0;
    }


    template <class T>
    T SyncReadStream<T>::receive()
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
	    throw StreamSerializationException();
	}

	this->_data_buf.consume(header);

	return t;
    }

}

#endif

