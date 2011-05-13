/*
 * Copyright (c) [2004-2011] Novell, Inc.
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


#ifndef APP_UTIL_H
#define APP_UTIL_H

#include <libintl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sstream>
#include <locale>
#include <string>
#include <list>
#include <map>


namespace snapper
{
    using std::string;
    using std::list;
    using std::map;


void createPath(const string& Path_Cv);
bool checkNormalFile(const string& Path_Cv);
bool checkDir(const string& Path_Cv);

    list<string> glob(const string& path, int flags);

    FILE* mkstemp(string& path);

    int clonefile(const string& dest, const string& src, mode_t mode);

    int readlink(const string& path, string& buf);
    int symlink(const string& oldpath, const string& newpath);

template<class StreamType>
void classic(StreamType& stream)
{
    stream.imbue(std::locale::classic());
}


enum LogLevel { DEBUG, MILESTONE, WARNING, ERROR };

void createLogger(const string& name, const string& logpath, const string& logfile);

bool testLogLevel(LogLevel level);

void prepareLogStream(std::ostringstream& stream);

std::ostringstream* logStreamOpen();

void logStreamClose(LogLevel level, const char* file, unsigned line,
		    const char* func, std::ostringstream*);

void initDefaultLogger();

#define y2deb(op) y2log_op(snapper::DEBUG, __FILE__, __LINE__, __FUNCTION__, op)
#define y2mil(op) y2log_op(snapper::MILESTONE, __FILE__, __LINE__, __FUNCTION__, op)
#define y2war(op) y2log_op(snapper::WARNING, __FILE__, __LINE__, __FUNCTION__, op)
#define y2err(op) y2log_op(snapper::ERROR, __FILE__, __LINE__, __FUNCTION__, op)

#define y2log_op(level, file, line, func, op)				\
    do {								\
	if (snapper::testLogLevel(level))				\
	{								\
	    std::ostringstream* __buf = snapper::logStreamOpen();	\
	    *__buf << op;						\
	    snapper::logStreamClose(level, file, line, func, __buf);	\
	}								\
    } while (0)


    string hostname();

    string datetime(time_t time, bool utc, bool classic);
    time_t scan_datetime(const string& str, bool utc);


    class StopWatch
    {
    public:

	StopWatch();

	friend std::ostream& operator<<(std::ostream& s, const StopWatch& sw);

    protected:

	struct timeval start_tv;

    };


    string sformat(const string& format, ...);

    string _(const char* msgid);
    string _(const char* msgid, const char* msgid_plural, unsigned long int n);


}

#endif
