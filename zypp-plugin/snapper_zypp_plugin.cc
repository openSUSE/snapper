// getenv
#include <stdlib.h>

#include <boost/regex.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <iostream>
#include <map>
#include <string>
using namespace std;

class Logging {
public:
    void debug(const string&);
    void info(const string& s) { cerr << s << endl; }
    void error(const string&);
};
Logging logging;

// Plugin message aka frame
// https://doc.opensuse.org/projects/libzypp/SLE12SP2/zypp-plugins.html
struct Message {
    string command;
    map<string, string> headers;
    string body;
};

class ZyppPlugin {
public:
    virtual int main();
    virtual Message dispatch(const Message&);
    void answer(const Message&);
    virtual ~ZyppPlugin() {}
};


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
		throw "FIXME: expected a command";
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
		
	    }
	}
    }
    return 0;
}

Message ZyppPlugin::dispatch(const Message& msg) {
    Message a;
    a.command = "_ENOMETHOD";
    a.headers["Command"] = msg.command;
    return a;
}

class ZyppCommitPlugin : public ZyppPlugin {
public:
    Message dispatch(const Message& msg);

    virtual Message plugin_begin(Message) {}
    virtual Message plugin_end(Message) {}
    virtual Message commit_begin(Message) {}
    virtual Message commit_end(Message) {}

    Message ack();
};

Message ZyppCommitPlugin::dispatch(const Message& msg) {
    if (msg.command == "PLUGINBEGIN") {
	return plugin_begin(msg);
    }
    if (msg.command == "PLUGINEND") {
	return plugin_end(msg);
    }
    if (msg.command == "COMMITBEGIN") {
	return commit_begin(msg);
    }
    if (msg.command == "COMMITEND") {
	return commit_end(msg);
    }

    return ZyppPlugin::dispatch(msg);
}

class SnapperZyppPlugin : public ZyppCommitPlugin {
public:
    Message plugin_begin(const Message& m) {
	userdata = get_userdata(m);

	return ack();
    }
    Message plugin_end(Message);
    Message commit_begin(Message);
    Message commit_end(Message);

private:
    map<string, string> userdata;

    map<string, string> get_userdata(const Message&);
};

int main() {
    if (getenv("DISABLE_SNAPPER_ZYPP_PLUGIN") != nullptr) {
	logging.info("$DISABLE_SNAPPER_ZYPP_PLUGIN is set - disabling snapper-zypp-plugin");
	ZyppCommitPlugin plugin;
	return plugin.main();
    }

    SnapperZyppPlugin plugin;
    return plugin.main();
}
