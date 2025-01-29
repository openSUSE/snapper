/*
 * Copyright (c) [2019-2024] SUSE LLC
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
 * with this program; if not, contact SUSE LLC.
 *
 * To contact SUSE about this file by physical or electronic mail, you may
 * find current contact information at www.suse.com.
 */


#include <regex>

#include "Stomp.h"


namespace Stomp
{

    using namespace std;


    Message
    read_message(istream& is)
    {
	static const regex rx_command("[A-Za-z0-9_]+", regex::extended);

	enum class State { Start, Headers, Body } state = State::Start;
	bool has_content_length = false;
	ssize_t content_length = 0;

	Message msg;

	while (!is.eof())
	{
	    string line;
	    getline(is, line);
	    line = strip_cr(line);

	    if (state == State::Start)
	    {
		if (is.eof())
		    return msg; // empty

		if (line.empty())
		    continue;

		if (regex_match(line, rx_command))
		{
		    msg = Message(line);
		    state = State::Headers;
		}
		else
		{
		    throw runtime_error("stomp error: expected a command, got '" + line + "'");
		}
	    }
	    else if (state == State::Headers)
	    {
		if (line.empty())
		{
		    state = State::Body;

		    if (has_content_length)
		    {
			if (content_length > 0)
			{
			    vector<char> buf(content_length);
			    is.read(buf.data(), content_length);
			    msg.body.assign(buf.data(), content_length);
			}

			// still read the \0 that terminates the frame
			char buf2 = '-';
			is.read(&buf2, 1);
			if (buf2 != '\0')
			    throw runtime_error("stomp error: missing \\0 at frame end");
		    }
		    else
		    {
			getline(is, msg.body, '\0');
		    }

		    return msg;
		}
		else
		{
		    string::size_type pos = line.find(':');
		    if (pos == string::npos)
			throw runtime_error("stomp error: expected a header or new line, got '" + line + "'");

		    string key = unescape_header(line.substr(0, pos));
		    string value = unescape_header(line.substr(pos + 1));

		    if (key == "content-length")
		    {
			has_content_length = true;
			content_length = stol(value.c_str());
		    }

		    msg.headers[key] = value;
		}
	    }
	}

	throw runtime_error("stomp error: expected a message, got a part of it");
    }


    void
    write_message(ostream& os, const Message& msg)
    {
	os << msg.command << '\n';
	for (auto it : msg.headers)
	    os << escape_header(it.first) << ':' << escape_header(it.second) << '\n';
	os << '\n';
	os << msg.body << '\0';
	os.flush();
    }


    Message
    ack()
    {
	return Message("ACK");
    }


    Message
    nack()
    {
	return Message("NACK");
    }


    string
    strip_cr(const string& in)
    {
	string::size_type length = in.size();

	if (length > 0 && in[length - 1] == '\r')
	    return in.substr(0, length - 1);

	return in;
    }


    string
    escape_header(const string& in)
    {
	string out;

	for (const char c : in)
	{
	    switch (c)
	    {
		case '\r':
		    out += "\\r"; break;
		case '\n':
		    out += "\\n"; break;
		case ':':
		    out += "\\c"; break;
		case '\\':
		    out += "\\\\"; break;

		default:
		    out += c;
	    }
	}

	return out;
    }


    string
    unescape_header(const string& in)
    {
	string out;

	for (string::const_iterator it = in.begin(); it != in.end(); ++it)
	{
	    if (*it == '\\')
	    {
		if (++it == in.end())
		    throw runtime_error("stomp error: invalid start of escape sequence");

		switch (*it)
		{
		    case 'r':
			out += '\r'; break;
		    case 'n':
			out += '\n'; break;
		    case 'c':
			out += ':'; break;
		    case '\\':
			out += '\\'; break;

		    default:
			throw runtime_error("stomp error: unknown escape sequence");
		}
	    }
	    else
	    {
		out += *it;
	    }
	}

	return out;
    }

}
