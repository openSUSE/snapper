// getenv
#include <stdlib.h>
// getppid
#include <sys/types.h>
#include <unistd.h>

#include <boost/format.hpp>
using boost::format;

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

#include "zypp_commit_plugin.h"

class Logging {
public:
    void debug(const string& s) { cerr << s << endl; }
    void info(const string& s) { cerr << s << endl; }
    void error(const string& s) { cerr << s << endl; }
};
Logging logging;


class SnapperZyppPlugin : public ZyppCommitPlugin {
public:
    SnapperZyppPlugin()
    : dbus_conn(DBUS_BUS_SYSTEM)
    , pre_snapshot_num(0)
    {
	snapshot_description = "zypp(%s)"; // % basename(readlink("/proc/%d/exe" % getppid()))
    }

    Message plugin_begin(const Message& m) override {
	userdata = get_userdata(m);

	return ack();
    }
    Message plugin_end(const Message& m) override {
	logging.info("PLUGINEND");
	return ack();
    }
    Message commit_begin(const Message& msg) override {
	logging.info("COMMITBEGIN");

	set<string> solvables = get_solvables(msg, true);
	logging.debug("solvables: %s" /*% solvables*/);

	bool found, important;
	match_solvables(solvables, found, important);
	logging.info(str(format("found: %s, important: %s") % found % important));

	if (found || important) {
	    userdata["important"] = important ? "yes" : "no";

	    try {
		logging.info("creating pre snapshot");
		pre_snapshot_num = command_create_pre_snapshot(dbus_conn, "root", snapshot_description, cleanup_algorithm,
						      userdata);
		logging.debug(str(format("created pre snapshot %u") % pre_snapshot_num));
	    }
	    catch (const Exception& ex) {
		// assumes a logging setup
		SN_CAUGHT(ex);
	    }
	}

	return ack();
    }

    Message commit_end(const Message& msg) override {
	logging.info("COMMITEND");

	if (pre_snapshot_num != 0) {
	    set<string> solvables = get_solvables(msg, false);
	    logging.debug("solvables: %s" /*% solvables*/);

	    bool found, important;
	    match_solvables(solvables, found, important);
	    logging.info(str(format("found: %s, important: %s") % found % important));

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
		    logging.error("setting snapshot data failed:");
		    // assumes a logging setup
		    SN_CAUGHT(ex);
		}
		try {
		    logging.info("creating post snapshot");
		    unsigned int post_snapshot_num = command_create_post_snapshot(dbus_conn, "root", pre_snapshot_num, "", cleanup_algorithm,
								     userdata);
		    logging.debug(str(format("created post snapshot %u") % post_snapshot_num));
		}
		catch (const Exception& ex) {
		    logging.error("creating snapshot failed:");
		    // assumes a logging setup
		    SN_CAUGHT(ex);
		}
	    }
	    else {
		try {
		    logging.info("deleting pre snapshot");
		    vector<unsigned int> nums{ pre_snapshot_num };
		    bool verbose = false;
		    command_delete_snapshots(dbus_conn, "root", nums, verbose);
		    logging.debug(str(format("deleted pre snapshot %u") % pre_snapshot_num));
		}
		catch (const Exception& ex) {
		    logging.error("deleting snapshot failed:");
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
    if (doc.Parse(c_body).HasParseError()) {
	logging.error("parsing zypp JSON failed");
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

int main() {
    if (getenv("DISABLE_SNAPPER_ZYPP_PLUGIN") != nullptr) {
	logging.info("$DISABLE_SNAPPER_ZYPP_PLUGIN is set - disabling snapper-zypp-plugin");
	ZyppCommitPlugin plugin;
	return plugin.main();
    }

    SnapperZyppPlugin plugin;
    return plugin.main();
}
