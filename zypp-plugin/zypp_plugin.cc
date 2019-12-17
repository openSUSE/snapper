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

void ZyppPlugin::answer(const Message& msg) {
    cout << msg.command << endl;
    for(auto it: msg.headers) {
	cout << it.first << ':' << it.second << endl;
    }
    cout << endl;
    cout << msg.body << '\0';
    cout.flush();
}

int ZyppPlugin::main() {
    enum class State {
		      Start,
		      Headers,
		      Body
    } state = State::Start;

    Message msg;
    static const snapper::Regex rx_word("^[A-Za-z0-9_]+$");
    while(!cin.eof()) {
	string line;

	getline(cin, line);
	boost::trim_right(line);

	if (state == State::Start) {
	    if (line.empty())
		continue;
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
		getline(cin, msg.body, '\0');

		Message reply = dispatch(msg);
		answer(reply);

		state = State::Start;
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
    return 0;
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
