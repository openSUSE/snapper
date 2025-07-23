/*
 * Copyright (c) [2014-2015] Novell, Inc.
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, contact Novell, Inc.
 *
 * To contact Novell about this file by physical or electronic mail, you may
 * find current contact information at www.novell.com.
 */


/*
 * Large parts taken from libyui/YUIException.h
 */


#include <cstring>
#include <cstdio>
#include <sstream>

#include "snapper/Exception.h"
#include "snapper/AppUtil.h"
#include "snapper/LoggerImpl.h"


namespace snapper
{

    std::string
    CodeLocation::asString() const
    {
	// Format as "MySource.cc(myFunc):177"
	std::string str(_file);
	str += "(" + _func + "):" + std::to_string(_line);

	return str;
    }


    std::ostream&
    operator<<(std::ostream& str, const CodeLocation& obj)
    {
	return str << obj.asString();
    }


    Exception::Exception()
    {
    }


    Exception::Exception(const std::string& msg_r)
	: _msg(msg_r)
    {
    }


    Exception::~Exception() throw()
    {
    }


    std::string
    Exception::asString() const
    {
	std::ostringstream str;
	dumpOn(str);
	return str.str();
    }


    std::ostream&
    Exception::dumpOn(std::ostream& str) const
    {
	return str << _msg;
    }


    std::ostream&
    Exception::dumpError(std::ostream& str) const
    {
	return dumpOn(str << _where << ": ");
    }


    std::ostream&
    operator<<(std::ostream& str, const Exception& obj)
    {
	return obj.dumpError(str);
    }


    std::string
    Exception::strErrno(int errno_r)
    {
	return strerror(errno_r);
    }


    std::string
    Exception::strErrno(int errno_r, const std::string& msg)
    {
	return msg + ": " + strErrno(errno_r);
    }


    void
    Exception::log(const Exception& exception, const CodeLocation& location,
		   const char* const prefix)
    {
	y2log_op(LogLevel::WARNING, location.file().c_str(), location.line(),
		 location.func().c_str(), string(prefix) + " " + exception.asString());
    }

}
