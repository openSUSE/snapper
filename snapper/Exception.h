/*
 * Copyright (c) 2011 Novell, Inc.
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


#ifndef EXCEPTION_H
#define EXCEPTION_H


#include <exception>


namespace snapper
{

    struct SnapperException : public std::exception
    {
	explicit SnapperException() throw() {}
	virtual const char* what() const throw() { return "generic snapper exception"; }
    };


    struct FileNotFoundException : public SnapperException
    {
	explicit FileNotFoundException() throw() {}
	virtual const char* what() const throw() { return "file not found"; }
    };


    struct IllegalSnapshotException : public SnapperException
    {
	explicit IllegalSnapshotException() throw() {}
	virtual const char* what() const throw() { return "illegal snapshot"; }
    };

    struct BadAllocException : public SnapperException
    {
	explicit BadAllocException() throw() {}
	virtual const char* what() const throw() { return "bad alloc"; }
    };

    struct LogicErrorException : public SnapperException
    {
	explicit LogicErrorException() throw() {}
	virtual const char* what() const throw() { return "logic error"; }
    };

    struct IOErrorException : public SnapperException
    {
	explicit IOErrorException() throw() {}
	virtual const char* what() const throw() { return "IO error"; }
    };

    struct ProgramNotInstalledException : public SnapperException
    {
	explicit ProgramNotInstalledException(const char* msg) throw() : msg(msg) {}
	virtual const char* what() const throw() { return msg; }
	const char* msg;
    };

}


#endif
