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


#include "snapper/Log.h"
#include "snapper/AppUtil.h"


namespace snapper
{
    using namespace std;


    // Intentionally leaving this behind as a mem leak (bsc#940154)
    const string* component = new string("libsnapper");


    bool
    testLogLevel(LogLevel level)
    {
	return callLogQuery(level, *component);
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
	callLogDo(level, *component, file, line, func, stream->str());
	delete stream;
	stream = nullptr;
    }

}
