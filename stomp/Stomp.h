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


#ifndef SNAPPER_STOMP_H
#define SNAPPER_STOMP_H


#include <istream>
#include <ostream>
#include <string>
#include <map>


/**
 * A tiny STOMP (https://stomp.github.io/) implementation.
 */

namespace Stomp
{

    struct Message
    {
	Message() {}
	Message(const std::string& command) : command(command) {}

	std::string command;
	std::map<std::string, std::string> headers;
	std::string body;
    };


    Message read_message(std::istream& is);
    void write_message(std::ostream& os, const Message& msg);

    Message ack();
    Message nack();

    std::string strip_cr(const std::string& in);

    std::string escape_header(const std::string& in);
    std::string unescape_header(const std::string& in);

}


#endif
