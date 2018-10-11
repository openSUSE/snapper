/*
 * Copyright (c) [2011-2014] Novell, Inc.
 * Copyright (c) [2015,2018] SUSE LLC
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


#ifndef SNAPPER_EXCEPTION_H
#define SNAPPER_EXCEPTION_H


#include <exception>
#include <string>


namespace snapper
{
    //
    // Macros for application use
    //

    /**
     * Usage summary:
     *
     * Use SN_THROW to throw exceptions.
     * Use SN_CAUGHT If you caught an exceptions in order to handle it.
     * Use SN_RETHROW to rethrow a caught exception.
     *
     * The use of these macros is not mandatory. But SN_THROW and SN_RETHROW
     * will adjust the code location information stored in the exception. All
     * three macros will drop a line in the log file.
     *
     *	43   try
     *	44   {
     *	45	 try
     *	46	 {
     *	47	     SN_THROW(Exception("Something bad happened."));
     *	48	 }
     *	49	 catch (const Exception& exception)
     *	50	 {
     *	51	     SN_RETHROW(exception);
     *	52	 }
     *	53   }
     *	54   catch (const Exception& exception)
     *	55   {
     *	56	 SN_CAUGHT(exception);
     *	57   }
     *
     * The above produces the following log lines:
     *
     *	Main.cc(main):47 THROW:	  Main.cc(main):47: Something bad happened.
     *	Main.cc(main):51 RETHROW: Main.cc(main):47: Something bad happened.
     *	Main.cc(main):56 CAUGHT:  Main.cc(main):51: Something bad happened.
     **/


    /**
     * Create CodeLocation object storing the current location.
     **/
#define SN_EXCEPTION_CODE_LOCATION					\
    CodeLocation(__FILE__, __FUNCTION__, __LINE__)


    /**
     * Drops a log line and throws the Exception.
     **/
#define SN_THROW(EXCEPTION)						\
    _SN_THROW((EXCEPTION), SN_EXCEPTION_CODE_LOCATION)

    /**
     * Drops a log line telling the Exception was caught and handled.
     **/
#define SN_CAUGHT(EXCEPTION)						\
    _SN_CAUGHT((EXCEPTION), SN_EXCEPTION_CODE_LOCATION)


    /**
     * Drops a log line and rethrows, updating the CodeLocation.
     **/
#define SN_RETHROW(EXCEPTION)						\
    _SN_RETHROW((EXCEPTION), SN_EXCEPTION_CODE_LOCATION)


    /**
     * Throw Exception built from a message string.
     **/
#define SN_THROW_MSG(EXCEPTION_TYPE, MSG)				\
    SN_THROW(EXCEPTION_TYPE(MSG))


    /**
     * Throw Exception built from errno.
     **/
#define SN_THROW_ERRNO(EXCEPTION_TYPE)					\
    SN_THROW(EXCEPTION_TYPE(Exception::strErrno(errno)))


    /**
     * Throw Exception built from errno provided as argument.
     **/
#define SN_THROW_ERRNO1(EXCEPTION_TYPE, ERRNO)				\
    SN_THROW(EXCEPTION_TYPE(Exception::strErrno(ERRNO)))


    /**
     * Throw Exception built from errno and a message string.
     **/
#define SN_THROW_ERRNO_MSG(EXCEPTION_TYPE, MSG)				\
    SN_THROW(EXCEPTION_TYPE(Exception::strErrno(errno, MSG)))


    /**
     * Throw Exception built from errno provided as argument and a message
     * string.
     **/
#define SN_THROW_ERRNO_MSG1(EXCEPTION_TYPE, ERRNO, MSG)			\
    SN_THROW(EXCEPTION_TYPE(Exception::strErrno(ERRNO, MSG)))


    /**
     * Helper class for UI exceptions: Store _FILE_, _FUNCTION_ and _LINE_.
     * Construct this using the SN_EXCEPTION_CODE_LOCATION macro.
     **/
    class CodeLocation
    {
    public:
	/**
	 * Constructor.
	 * Commonly called using the SN_EXCEPTION_CODE_LOCATION macro.
	 **/
	CodeLocation(const std::string& file_r, const std::string& func_r, int line_r)
	    : _file(file_r), _func(func_r), _line(line_r) {}

	/**
	 * Default constructor.
	 ***/
	CodeLocation()
	    : _line(0) {}

	/**
	 * Returns the source file name where the exception occured.
	 **/
	const std::string& file() const { return _file; }

	/**
	 * Returns the name of the function where the exception occured.
	 **/
	const std::string& func() const { return _func; }

	/**
	 * Returns the source line number where the exception occured.
	 **/
	int line() const { return _line; }

	/**
	 * Returns the location in normalized string format.
	 **/
	std::string asString() const;

	/**
	 * Stream output
	 **/
	friend std::ostream& operator<<(std::ostream& str, const CodeLocation& obj);

    private:

	std::string _file;
	std::string _func;
	int _line;

    };


    /**
     * CodeLocation stream output
     **/
    std::ostream& operator<<(std::ostream& str, const CodeLocation& obj);


    /**
     * Base class for snapper exceptions.
     *
     * Exception offers to store a message string passed to the constructor.
     * Derived classes may provide additional information.
     * Overload dumpOn to provide a proper error text.
     **/
    class Exception : public std::exception
    {
    public:

	/**
	 * Default constructor.
	 * Use SN_THROW to throw exceptions.
	 **/
	Exception();

	/**
	 * Constructor taking a message.
	 * Use SN_THROW to throw exceptions.
	 **/
	Exception(const std::string& msg);

	/**
	 * Destructor.
	 **/
	virtual ~Exception() throw();

	/**
	 * Return CodeLocation.
	 **/
	const CodeLocation& where() const { return _where; }

	/**
	 * Exchange location on rethrow.
	 **/
	void relocate(const CodeLocation& newLocation) const { _where = newLocation; }

	/**
	 * Return the message string provided to the constructor.
	 * Note: This is not neccessarily the complete error message.
	 * The whole error message is provided by asString or dumpOn.
	 **/
	const std::string& msg() const { return _msg; }

	/**
	 * Set a new message string.
	 **/
	void setMsg(const std::string& msg) { _msg = msg; }

	/**
	 * Error message provided by dumpOn as string.
	 **/
	std::string asString() const;

	/**
	 * Make a string from errno_r.
	 **/
	static std::string strErrno(int errno_r);

	/**
	 * Make a string from errno_r and msg_r.
	 **/
	static std::string strErrno(int errno_r, const std::string& msg);

	/**
	 * Drop a log line on throw, catch or rethrow.
	 * Used by SN_THROW macros.
	 **/
	static void log(const Exception& exception, const CodeLocation& location,
			const char* const prefix);

	/**
	 * Return message string.
	 *
	 * Reimplemented from std::exception.
	 **/
	virtual const char* what() const throw() { return _msg.c_str(); }

    protected:

	/**
	 * Overload this to print a proper error message.
	 **/
	virtual std::ostream& dumpOn(std::ostream& str) const;

    private:

	friend std::ostream& operator<<(std::ostream& str, const Exception& obj);

	mutable CodeLocation _where;
	std::string _msg;

	/**
	 * Called by std::ostream& operator<<() .
	 * Prints CodeLocation and the error message provided by dumpOn.
	 **/
	std::ostream& dumpError(std::ostream& str) const;

    };


    /**
     * Exception stream output
     **/
    std::ostream& operator<<(std::ostream& str, const Exception& obj);


    //
    // Helper templates
    //


    /**
     * Helper for SN_THROW()
     **/
    template<class _Exception>
    void _SN_THROW(const _Exception& exception, const CodeLocation& where)
    {
	exception.relocate(where);
	Exception::log(exception, where, "THROW:");

	throw exception;
    }


    /**
     * Helper for SN_CAUGHT()
     **/
    template<class _Exception>
    void _SN_CAUGHT(const _Exception& exception, const CodeLocation& where)
    {
	Exception::log(exception, where, "CAUGHT:");
    }


    /**
     * Helper for SN_RETHROW()
     **/
    template<class _Exception>
    void _SN_RETHROW(const _Exception& exception, const CodeLocation& where)
    {
	Exception::log(exception, where, "RETHROW:");
	exception.relocate(where);

	throw;
    }


    struct FileNotFoundException : public Exception
    {
	explicit FileNotFoundException() : Exception("file not found") {}
    };


    struct IllegalSnapshotException : public Exception
    {
	explicit IllegalSnapshotException() : Exception("illegal snapshot") {}
    };

    struct BadAllocException : public Exception
    {
	explicit BadAllocException() : Exception("bad alloc") {}
    };

    struct LogicErrorException : public Exception
    {
	explicit LogicErrorException() : Exception("logic error") {}
    };

    struct IOErrorException : public Exception
    {
	explicit IOErrorException(const std::string& msg) : Exception(msg) {}
    };

    struct AclException : public IOErrorException
    {
	explicit AclException() : IOErrorException("ACL error") {}
    };

    struct ProgramNotInstalledException : public Exception
    {
	explicit ProgramNotInstalledException(const std::string& msg) : Exception(msg) {}
    };

    struct XAttributesException : public Exception
    {
	explicit XAttributesException() : Exception("XAttributes error") {}
    };

    struct InvalidUserException : public Exception
    {
	explicit InvalidUserException() : Exception("invalid user") {}
    };

    struct InvalidGroupException : public Exception
    {
	explicit InvalidGroupException() : Exception("invalid group") {}
    };

    struct UnsupportedException : public Exception
    {
        explicit UnsupportedException() : Exception("unsupported") {}
    };

}


#endif
