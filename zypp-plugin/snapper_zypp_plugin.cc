// getenv
#include <stdlib.h>
// getppid
#include <sys/types.h>
#include <unistd.h>

#include <boost/regex.hpp>

#include <iostream>
#include <map>
#include <string>
using namespace std;

#include <rapidjson/document.h>

#include "dbus/DBusConnection.h"
#include "snapper/Exception.h"
using snapper::Exception;
using snapper::CodeLocation;
#include "client/commands.h"
#include "snapper/Log.h"

#include "zypp_commit_plugin.h"

ostream& operator <<(ostream& os, set<string> ss) {
    bool seen_first = false;
    os << '{';
    for(auto s: ss) {
	if (seen_first) {
	    os << ", ";
	}
	os << s;
	seen_first = true;
    }
    os << '}';
    return os;
}

class SnapperZyppPlugin : public ZyppCommitPlugin {
public:
    SnapperZyppPlugin()
    : dbus_conn(DBUS_BUS_SYSTEM)
    , pre_snapshot_num(0)
    {
	snapshot_description = "zypp(%s)"; // % basename(readlink("/proc/%d/exe" % getppid()))
    }

    Message plugin_begin(const Message& m) override {
	y2mil("PLUGINBEGIN");
	userdata = get_userdata(m);

	return ack();
    }
    Message plugin_end(const Message& m) override {
	y2mil("PLUGINEND");
	return ack();
    }
    Message commit_begin(const Message& msg) override {
	y2mil("COMMITBEGIN");

	set<string> solvables = get_solvables(msg, true);
	y2deb("solvables: " << solvables);

	bool found, important;
	match_solvables(solvables, found, important);
	y2mil("found: " << found << ", important: " << important);

	if (found || important) {
	    userdata["important"] = important ? "yes" : "no";

	    try {
		y2mil("creating pre snapshot");
		pre_snapshot_num = command_create_pre_snapshot(dbus_conn, "root", snapshot_description, cleanup_algorithm,
						      userdata);
		y2deb("created pre snapshot " << pre_snapshot_num);
	    }
	    catch (const Exception& ex) {
		// assumes a logging setup
		SN_CAUGHT(ex);
	    }
	}

	return ack();
    }

    Message commit_end(const Message& msg) override {
	y2mil("COMMITEND");

	if (pre_snapshot_num != 0) {
	    set<string> solvables = get_solvables(msg, false);
	    y2deb("solvables: " << solvables);

	    bool found, important;
	    match_solvables(solvables, found, important);
	    y2mil("found: " << found << ", important: " << important);

	    if (found || important) {
		userdata["important"] = important ? "yes" : "no";

		try {
		    snapper::SMD modification_data;
		    modification_data.description = snapshot_description;
		    modification_data.cleanup = cleanup_algorithm;
		    modification_data.userdata = userdata;
		    command_set_snapshot(dbus_conn, "root", pre_snapshot_num, modification_data);
		}
		catch (const Exception& ex) {
		    y2err("setting snapshot data failed:");
		    // assumes a logging setup
		    SN_CAUGHT(ex);
		}
		try {
		    y2mil("creating post snapshot");
		    unsigned int post_snapshot_num = command_create_post_snapshot(dbus_conn, "root", pre_snapshot_num, "", cleanup_algorithm,
								     userdata);
		    y2deb("created post snapshot " << post_snapshot_num);
		}
		catch (const Exception& ex) {
		    y2err("creating snapshot failed:");
		    // assumes a logging setup
		    SN_CAUGHT(ex);
		}
	    }
	    else {
		try {
		    y2mil("deleting pre snapshot");
		    vector<unsigned int> nums{ pre_snapshot_num };
		    bool verbose = false;
		    command_delete_snapshots(dbus_conn, "root", nums, verbose);
		    y2deb("deleted pre snapshot " << pre_snapshot_num);
		}
		catch (const Exception& ex) {
		    y2err("deleting snapshot failed:");
		    // assumes a logging setup
		    SN_CAUGHT(ex);
		}
	    }
	}
	return ack();
    }

private:
    static const string cleanup_algorithm;

    DBus::Connection dbus_conn;
    unsigned int pre_snapshot_num;
    string snapshot_description;
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

set<string> SnapperZyppPlugin::get_solvables(const Message& msg, bool todo) {
    set<string> result;

    rapidjson::Document doc;
    const char * c_body = msg.body.c_str();
    y2deb("parsing zypp JSON: " << c_body);
    if (doc.Parse(c_body).HasParseError()) {
	y2err("parsing zypp JSON failed");
	return result;
    }
    // https://doc.opensuse.org/projects/libzypp/SLE12SP2/plugin-commit.html
    using rapidjson::Value;
    const Value& steps = doc["TransactionStepList"];
    for (Value::ConstValueIterator it = steps.Begin(); it != steps.End(); ++it) {
	const Value& step = *it;
	if (step.HasMember("type")) {
	    if (todo || step.HasMember("stage")) {
		const Value& solvable = step["solvable"];
		const Value& name = solvable["n"];
		// FIXME: what happens when the doc structure is different?
		result.insert(name.GetString());
	    }
	}
    }

    return result;
}

void SnapperZyppPlugin::match_solvables(const set<string>&, bool& found, bool& important) {
    found = true;
    important = true;
}

bool
log_query(LogLevel level, const string& component)
{
    if (level == DEBUG)
	return getenv("DEBUG") != nullptr;
    else
	return true;
}

int main() {
    setLogQuery(&log_query);
    if (getenv("DISABLE_SNAPPER_ZYPP_PLUGIN") != nullptr) {
	y2mil("$DISABLE_SNAPPER_ZYPP_PLUGIN is set - disabling snapper-zypp-plugin");
	ZyppCommitPlugin plugin;
	return plugin.main();
    }

    SnapperZyppPlugin plugin;
    return plugin.main();
}
