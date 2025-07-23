/*
 * Copyright (c) [2004-2012] Novell, Inc.
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


#include "snapper/LoggerImpl.h"


namespace snapper
{
    using namespace std;


    bool
    query_log_level(LogLevel log_level)
    {
	Logger* logger = get_logger();
	if (logger)
	{
	    return logger->test(log_level);
	}

	return false;
    }


    void
    prepare_log_stream(ostringstream& stream)
    {
	stream.imbue(std::locale::classic());
	stream.setf(std::ios::boolalpha);
	stream.setf(std::ios::showbase);
    }


    ostringstream*
    open_log_stream()
    {
	std::ostringstream* stream = new ostringstream;
	prepare_log_stream(*stream);
	return stream;
    }


    void
    close_log_stream(LogLevel log_level, const char* file, unsigned int line, const char* func,
		     ostringstream* stream)
    {
	Logger* logger = get_logger();
	if (logger)
	{
	    string content = stream->str();
	    string::size_type pos1 = 0;
	    while (true)
	    {
		string::size_type pos2 = content.find('\n', pos1);
		if (pos2 != string::npos || pos1 != content.length())
		    logger->write(log_level, file, line, func, content.substr(pos1, pos2 - pos1));
		if (pos2 == string::npos)
		    break;
		pos1 = pos2 + 1;
	    }
	}

	delete stream;
	stream = nullptr;
    }

}
