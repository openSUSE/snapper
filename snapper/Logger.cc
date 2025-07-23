/*
 * Copyright (c) [2004-2013] Novell, Inc.
 * Copyright (c) [2018-2025] SUSE LLC
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
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <cstring>
#include <sys/types.h>
#include <string>
#include <iostream>
#include <boost/thread.hpp>

#include "snapper/LoggerImpl.h"
#include "snapper/AppUtil.h"


namespace snapper
{
    using namespace std;


    typedef typename underlying_type<LogLevel>::type log_level_underlying_type;


    // Unfortunately we face the static deinitialization order fiasco here: Destructors of
    // global objects may use logging functions and could be run after the destructors of
    // the logging functions have been run. Mitigated simply by using the "no destruction"
    // rule.


    namespace
    {

	Logger* current_logger = nullptr;

	LogLevel log_level_tresshold = LogLevel::ERROR;

    }


    Logger*
    get_logger()
    {
	return current_logger;
    }


    void
    set_logger(Logger* logger)
    {
	current_logger = logger;
    }


    LogLevel
    get_logger_tresshold()
    {
	return log_level_tresshold;
    }


    void
    set_logger_tresshold(LogLevel tresshold)
    {
	log_level_tresshold = tresshold;
    }


    bool
    Logger::test(LogLevel log_level)
    {
	return log_level >= log_level_tresshold;
    }


    class StdoutLogger : public Logger
    {

    public:

	virtual void write(LogLevel log_level, const string& file, int line, const string& function,
			   const string& content) override;

    };


    void
    StdoutLogger::write(LogLevel log_level, const string& file, int line, const string& function,
			const string& content)
    {
	cerr << datetime(time(nullptr), false, false) << " <" << static_cast<log_level_underlying_type>(log_level)
	     << "> " << file << "(" << function << "):" << line << " " << content << endl;
    }


    Logger*
    get_stdout_logger()
    {
	static StdoutLogger stdout_logger;

	return &stdout_logger;
    }


    class LogfileLogger : public Logger
    {

    public:

	LogfileLogger();
	virtual ~LogfileLogger();

	virtual void write(LogLevel log_level, const string& file, int line, const string& function,
			   const string& content) override;

    private:

	struct Data
	{
	    string filename;
	    boost::mutex mutex;
	};

	Data* data = nullptr;

    };


    LogfileLogger::LogfileLogger()
	: data(new Data)
    {
	if (geteuid())
	{
	    string dir;
	    if (get_uid_dir(geteuid(), dir))
		data->filename = dir + "/.snapper.log";
	}
	else
	{
	    data->filename = "/var/log/snapper.log";
	}
    }


    LogfileLogger::~LogfileLogger()
    {
	// data is not deleted to avoid static deinitialization order fiasco
    }


    void
    LogfileLogger::write(LogLevel log_level, const string& file, int line, const string& function,
			 const string& content)
    {
	boost::lock_guard<boost::mutex> lock(data->mutex);

	int fd = open(data->filename.c_str(), O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC, 0640);
	if (fd < 0)
	    return;

	FILE* f = fdopen(fd, "ae");
	if (!f)
	{
	    close(fd);
	    return;
	}

	fprintf(f, "%s <%d> %s(%s):%d %s\n", datetime(time(nullptr), false, false).c_str(),
		static_cast<log_level_underlying_type>(log_level), file.c_str(), function.c_str(),
		line, content.c_str());

	fclose(f);
    }


    Logger*
    get_logfile_logger()
    {
	static LogfileLogger logfile_logger;

	return &logfile_logger;
    }


    class SyslogLogger : public Logger
    {

    public:

	SyslogLogger(const char* ident, int option, int facility);
	~SyslogLogger();

	virtual void write(LogLevel log_level, const string& file, int line, const string& function,
			   const string& content) override;

    };


    SyslogLogger::SyslogLogger(const char* ident, int option, int facility)
    {
	openlog(ident, option, facility);
    }


    SyslogLogger::~SyslogLogger()
    {
	// closelog is not called to avoid static deinitialization order fiasco
    }


    void
    SyslogLogger::write(LogLevel log_level, const string& file, int line, const string& function,
			const string& content)
    {
	int priority = 0;

	switch (log_level)
	{
	    case LogLevel::DEBUG:     priority = LOG_DEBUG;   break;
	    case LogLevel::MILESTONE: priority = LOG_NOTICE;  break;
	    case LogLevel::WARNING:   priority = LOG_WARNING; break;
	    case LogLevel::ERROR:     priority = LOG_ERR;     break;
	}

	syslog(priority, "%s", content.c_str());
    }


    Logger*
    get_syslog_logger(const char* ident, int option, int facility)
    {
	static SyslogLogger syslog_logger(ident, option, facility);

	return &syslog_logger;
    }


    void
    test_logger()
    {
	y2deb("logger test debug");
	y2mil("logger test milestone");
	y2war("logger test warning");
	y2err("logger test error");
    }

}
