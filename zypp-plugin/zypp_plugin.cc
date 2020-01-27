/*
 * Copyright (c) 2019 SUSE LLC
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

#include <iostream>
#include <boost/algorithm/string/trim.hpp>
#include <string>
using namespace std;

#include "snapper/Regex.h"
#include "zypp_plugin.h"

int ZyppPlugin::main() {
    while(true) {
	Message msg = read_message(pin);
	if (pin.eof())
	    break;
	Message reply = dispatch(msg);
	write_message(pout, reply);
    }
    return 0;
}

void ZyppPlugin::write_message(ostream& os, const Message& msg) {
    os << msg.command << endl;
    for(auto it: msg.headers) {
	os << it.first << ':' << it.second << endl;
    }
    os << endl;
    os << msg.body << '\0';
    os.flush();
}

ZyppPlugin::Message ZyppPlugin::read_message(istream& is) {
    enum class State {
		      Start,
		      Headers,
		      Body
    } state = State::Start;

    Message msg;

    while(!is.eof()) {
	string line;

	getline(is, line);
	boost::trim_right(line);

	if (state == State::Start) {
	    if (is.eof())
		return msg; //empty

	    if (line.empty())
		continue;

	    static const snapper::Regex rx_word("^[A-Za-z0-9_]+$");
	    if (rx_word.match(line)) {
		msg = Message();
		msg.command = line;
		state = State::Headers;
	    }
	    else {
		throw runtime_error("Plugin protocol error: expected a command. Got '" + line + "'");
	    }
	}
	else if (state == State::Headers) {
	    if (line.empty()) {
		state = State::Body;
		getline(is, msg.body, '\0');

		return msg;
	    }
	    else {
		static const snapper::Regex rx_header("^([A-Za-z0-9_]+):[ \t]*(.+)$");
		if (rx_header.match(line)) {
		    string key = rx_header.cap(1);
		    string value = rx_header.cap(2);
		    msg.headers[key] = value;
		}
		else {
		    throw runtime_error("Plugin protocol error: expected a header or new line. Got '" + line + "'");
		}
	    }
	}
    }

    throw runtime_error("Plugin protocol error: expected a message, got a part of it");
}

ZyppPlugin::Message ZyppPlugin::dispatch(const Message& msg) {
    if (msg.command == "_DISCONNECT") {
	return ack();
    }
    Message a;
    a.command = "_ENOMETHOD";
    a.headers["Command"] = msg.command;
    return a;
}
