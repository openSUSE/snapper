 /*
 * Copyright (c) [2011-2012] Novell, Inc.
 * Copyright (c) 2025 SUSE LLC
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


#ifndef SNAPPER_LOGGER_H
#define SNAPPER_LOGGER_H

#include <string>


namespace snapper
{

    /**
     * enum class with the log levels.
     */
    enum class LogLevel
    {
	DEBUG, MILESTONE, WARNING, ERROR
    };


    /**
     * enum class with the logger types. Only for convenience of library users.
     */
    enum class LoggerType
    {
	NONE, STDOUT, LOGFILE, SYSLOG
    };


    /**
     * The Logger class.
     */
    class Logger
    {
    public:

	Logger() = default;
	virtual ~Logger() = default;

	/**
	 * Function to control whether a log line with the log-level be logged.
	 */
	virtual bool test(LogLevel log_level);

	/**
	 * Function to log a line.
	 */
	virtual void write(LogLevel log_level, const std::string& file, int line, const std::string& function,
			   const std::string& content) = 0;

    };


    /**
     * Get the current logger object.
     */
    Logger* get_logger();


    /**
     * Set the current logger object. The logger object must be valid until replaced by
     * another logger object and in a multi-threaded program even longer.
     */
    void set_logger(Logger* logger);


    /**
     * Get the current logger tresshold.
     */
    LogLevel get_logger_tresshold();


    /**
     * Set the current logger tresshold.
     */
    void set_logger_tresshold(LogLevel tresshold);


    /**
     * Returns a Logger that logs to stdout/stderr.
     */
    Logger* get_stdout_logger();


    /**
     * Returns a Logger that logs to the standard snapper log file ("/var/log/snapper.log"
     * or ~/.snapper.log).
     */
    Logger* get_logfile_logger();


    /**
     * Returns a Logger that logs to the system log.
     *
     * Note that this method only uses the given ident, option and facility the first time
     * it is called.
     */
    Logger* get_syslog_logger(const char* ident, int option, int facility);


    /**
     * Logs test messages with each log level.
     */
    void test_logger();

}

#endif
