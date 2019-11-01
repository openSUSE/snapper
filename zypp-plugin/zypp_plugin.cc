#include <iostream>
#include <boost/regex.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <string>
using namespace std;

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
    static const boost::regex rx_word("[A-Za-z0-9_]+");
    while(!cin.eof()) {
	string line;

	getline(cin, line);
	boost::trim_right(line);

	if (state == State::Start) {
	    if (line.empty())
		continue;
	    if (boost::regex_match(line, rx_word)) {
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
		static const boost::regex rx_header("([A-Za-z0-9_]+):[ \t]*(.+)");
		boost::smatch what;
		if (boost::regex_match(line, what, rx_header)) {
		    string key = what[1];
		    string value = what[2];
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
    Message a;
    a.command = "_ENOMETHOD";
    a.headers["Command"] = msg.command;
    return a;
}

