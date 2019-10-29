// getenv
#include <stdlib.h>
// getppid
#include <sys/types.h>
#include <unistd.h>

#include <boost/format.hpp>
using boost::format;
#include <boost/regex.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <iostream>
#include <map>
#include <string>
using namespace std;

class Logging {
public:
    void debug(const string& s) { cerr << s << endl; }
    void info(const string& s) { cerr << s << endl; }
    void error(const string& s) { cerr << s << endl; }
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
		//string key, value;
		// TODO: parse the header
		//msg.headers[key] = value;
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
    Message dispatch(const Message& msg) override;

    virtual Message plugin_begin(const Message& m) {
	return ack();
    }
    virtual Message plugin_end(const Message& m) {
	return ack();
    }
    virtual Message commit_begin(const Message& m) {
	return ack();
    }
    virtual Message commit_end(const Message& m) {
	return ack();
    }

    Message ack() {
	Message a;
	a.command = "ACK";
	return a;
    }
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
    Message plugin_begin(const Message& m) override {
	userdata = get_userdata(m);

	return ack();
    }
    Message plugin_end(const Message& m) override {
	return ack();
    }
    Message commit_begin(const Message& msg) override {
        logging.info("COMMITBEGIN");

        auto solvables = get_solvables(msg, true);
        //logging.debug("solvables: %s" % solvables);

	bool found, important;
        match_solvables(solvables, found, important);
        logging.info(str(format("found: %s, important: %s") % found % important));

        if (found || important) {
            userdata["important"] = important ? "yes" : "no";

            try {
                logging.info("creating pre snapshot");
		string description = "zypp(%s)"; // % basename(readlink("/proc/%d/exe" % getppid()))

                pre_snapshot_num = create_pre_snapshot("root", description, cleanup_algorithm,
                                                      userdata);
                logging.debug(str(format("created pre snapshot %u") % pre_snapshot_num));
	    }
	    catch (...) {
                logging.error("creating snapshot failed:");
                //logging.error("  %s", e)
	    }
	}

	return ack();
    }
    Message commit_end(const Message& m) override {
	return ack();
    }

private:
    static const string cleanup_algorithm;

    unsigned int pre_snapshot_num;
    map<string, string> userdata;

    map<string, string> get_userdata(const Message&);

    // FIXME: what does the todo flag mean?
    set<string> get_solvables(const Message&, bool todo);

    void match_solvables(const set<string>&, bool& found, bool& important);

    unsigned int create_pre_snapshot(string config_name, string description, string cleanup, map<string, string> userdata);
};

const string SnapperZyppPlugin::cleanup_algorithm = "number";

map<string, string> SnapperZyppPlugin::get_userdata(const Message&) {
    map<string, string> result;
    return result;
}

set<string> SnapperZyppPlugin::get_solvables(const Message&, bool todo) {
    set<string> result;
    return result;
}

void SnapperZyppPlugin::match_solvables(const set<string>&, bool& found, bool& important) {
    found = true;
    important = true;
}

unsigned int SnapperZyppPlugin::create_pre_snapshot(string config_name, string description, string cleanup, map<string, string> userdata) {
    // call DBus;
    unsigned int snapshot_num = 0;
    return snapshot_num;
}

int main() {
    if (getenv("DISABLE_SNAPPER_ZYPP_PLUGIN") != nullptr) {
	logging.info("$DISABLE_SNAPPER_ZYPP_PLUGIN is set - disabling snapper-zypp-plugin");
	ZyppCommitPlugin plugin;
	return plugin.main();
    }

    SnapperZyppPlugin plugin;
    return plugin.main();
}
