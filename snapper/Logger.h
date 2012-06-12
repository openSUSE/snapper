/*
 * Copyright (c) [2011-2012] Novell, Inc.
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
    using std::string;


    enum LogLevel { DEBUG, MILESTONE, WARNING, ERROR };

    /*
     * Called function should be able to split content at newlines.
     */
    typedef void (*LogDo)(LogLevel level, const string& component, const char* file, int line,
			  const char* func, const string& content);

    typedef bool (*LogQuery)(LogLevel level, const string& component);

    void setLogDo(LogDo log_do);

    void setLogQuery(LogQuery log_query);

    void callLogDo(LogLevel level, const string& component, const char* file, int line,
		   const char* func, const string& text);

    bool callLogQuery(LogLevel level, const string& component);

    void initDefaultLogger();

}

#endif
