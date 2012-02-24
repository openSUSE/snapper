/*
 * Copyright (c) [2004-2012] Novell, Inc.
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


#include <pwd.h>
#include <string>
#include <libxml/tree.h>

#include "config.h"

#ifdef HAVE_LIBBLOCXX
#include <blocxx/AppenderLogger.hpp>
#include <blocxx/FileAppender.hpp>
#include <blocxx/Logger.hpp>
#include <blocxx/LogMessage.hpp>
#endif

#include "snapper/Log.h"
#include "snapper/AppUtil.h"


namespace snapper
{
    using namespace std;

#ifdef HAVE_LIBBLOCXX

    using namespace blocxx;

    static const String component = "libsnapper";

#else

    string filename;

#endif


    void createLogger(const string& name, const string& logpath, const string& logfile)
    {
#ifdef HAVE_LIBBLOCXX

	if (logpath != "NULL" && logfile != "NULL")
	{
	    String nm = name.c_str();
	    LoggerConfigMap configItems;
	    LogAppenderRef logApp;
	    if (logpath != "STDERR" && logfile != "STDERR" &&
		logpath != "SYSLOG" && logfile != "SYSLOG")
	    {
		String StrKey;
		String StrPath;
		StrKey.format("log.%s.location", name.c_str());
		StrPath = (logpath + "/" + logfile).c_str();
		configItems[StrKey] = StrPath;
		logApp =
		    LogAppender::createLogAppender(nm, LogAppender::ALL_COMPONENTS,
						   LogAppender::ALL_CATEGORIES,
						   "%d %-5p %c(%P) %F(%M):%L - %m",
						   LogAppender::TYPE_FILE,
						   configItems);
	    }
	    else if (logpath == "STDERR" && logfile == "STDERR")
	    {
		logApp =
		    LogAppender::createLogAppender(nm, LogAppender::ALL_COMPONENTS,
						   LogAppender::ALL_CATEGORIES,
						   "%d %-5p %c(%P) %F(%M):%L - %m",
						   LogAppender::TYPE_STDERR,
						   configItems);
	    }
	    else
	    {
		logApp =
		    LogAppender::createLogAppender(nm, LogAppender::ALL_COMPONENTS,
						   LogAppender::ALL_CATEGORIES,
						   "%d %-5p %c(%P) %F(%M):%L - %m",
						   LogAppender::TYPE_SYSLOG,
						   configItems);
	    }

	    LogAppender::setDefaultLogAppender(logApp);
	}

#else

	filename = logpath + "/" + logfile;

#endif
    }


    bool
    testLogLevel(LogLevel level)
    {
#ifdef HAVE_LIBBLOCXX

	ELogLevel curLevel = LogAppender::getCurrentLogAppender()->getLogLevel();

	switch (level)
	{
	    case DEBUG:
		return false; // curLevel >= E_DEBUG_LEVEL;
	    case MILESTONE:
		return curLevel >= E_INFO_LEVEL;
	    case WARNING:
		return curLevel >= E_WARNING_LEVEL;
	    case ERROR:
		return curLevel >= E_ERROR_LEVEL;
	    default:
		return curLevel >= E_FATAL_ERROR_LEVEL;
	}

#else

	return level != DEBUG;

#endif
    }


    void
    prepareLogStream(ostringstream& stream)
    {
	stream.imbue(std::locale::classic());
	stream.setf(std::ios::boolalpha);
	stream.setf(std::ios::showbase);
    }


    ostringstream*
    logStreamOpen()
    {
	std::ostringstream* stream = new ostringstream;
	prepareLogStream(*stream);
	return stream;
    }


    void
    logStreamClose(LogLevel level, const char* file, unsigned line, const char* func,
		   ostringstream* stream)
    {
#ifdef HAVE_LIBBLOCXX

	ELogLevel curLevel = LogAppender::getCurrentLogAppender()->getLogLevel();
	String category;

	switch (level)
	{
	    case DEBUG:
		if (curLevel >= E_DEBUG_LEVEL)
		    category = Logger::STR_DEBUG_CATEGORY;
		break;
	    case MILESTONE:
		if (curLevel >= E_INFO_LEVEL)
		    category = Logger::STR_INFO_CATEGORY;
		break;
	    case WARNING:
		if (curLevel >= E_WARNING_LEVEL)
		    category = Logger::STR_WARNING_CATEGORY;
		break;
	    case ERROR:
		if (curLevel >= E_ERROR_LEVEL)
		    category = Logger::STR_ERROR_CATEGORY;
		break;
	    default:
		if (curLevel >= E_FATAL_ERROR_LEVEL)
		    category = Logger::STR_FATAL_CATEGORY;
		break;
	}

	if (!category.empty())
	{
	    string tmp = stream->str();

	    string::size_type pos1 = 0;

	    while (true)
	    {
		string::size_type pos2 = tmp.find('\n', pos1);

		if (pos2 != string::npos || pos1 != tmp.length())
		    LogAppender::getCurrentLogAppender()->logMessage(LogMessage(component, category,
										String(tmp.substr(pos1, pos2 - pos1)),
										file, line, func));

		if (pos2 == string::npos)
		    break;

		pos1 = pos2 + 1;
	    }
	}

#else

	static const char* ln[4] = { "DEB", "MIL", "WAR", "ERR" };

	string prefix = sformat("%s %s libsnapper(%d) %s(%s):%d", datetime(time(0), false, true).c_str(),
				ln[level], getpid(), file, func, line);

	FILE* f = fopen(filename.c_str(), "a");

	string tmp = stream->str();

	string::size_type pos1 = 0;

	while (true)
	{
	    string::size_type pos2 = tmp.find('\n', pos1);

	    if (pos2 != string::npos || pos1 != tmp.length())
		fprintf(f, "%s - %s\n", prefix.c_str(), tmp.substr(pos1, pos2 - pos1).c_str());

	    if (pos2 == string::npos)
		break;

	    pos1 = pos2 + 1;
	}

	fclose(f);

#endif

	delete stream;
    }


    void
    xml_error_func(void* ctx, const char* msg, ...)
    {
    }

    xmlGenericErrorFunc xml_error_func_ptr = &xml_error_func;


    void initDefaultLogger()
    {
	string path;
	string file;
	if (geteuid())
	{
	    struct passwd* pw = getpwuid(geteuid());
	    if (pw)
	    {
		path = pw->pw_dir;
		file = "snapper.log";
	    }
	    else
	    {
		path = "/";
		file = "snapper.log";
	    }
	}
	else
	{
	    path = "/var/log";
	    file = "snapper.log";
	}
	createLogger("default", path, file);

	initGenericErrorDefaultFunc(&xml_error_func_ptr);
    }

}
