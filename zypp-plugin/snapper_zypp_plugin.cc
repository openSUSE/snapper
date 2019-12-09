// getenv
#include <stdlib.h>
// getppid
#include <sys/types.h>
#include <unistd.h>
// fnmatch
#include <fnmatch.h>

#include <boost/regex.hpp>
// split
#include <boost/algorithm/string.hpp>

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
#include "client/errors.h"
#include "snapper/XmlFile.h"

#include "zypp_commit_plugin.h"

ostream& operator <<(ostream& os, set<string> ss) {
    bool seen_first = false;
    os << '{';
    for (auto s: ss) {
	if (seen_first) {
	    os << ", ";
	}
	os << s;
	seen_first = true;
    }
    os << '}';
    return os;
}

// Normally the only configuration this program needs is
// the /etc/snapper/zypp-plugin.conf file.
// But for testing we need more places to inject mocks.
// This is done with SNAPPER_ZYPP_PLUGIN_* environment variables.
// (Using argv is not useful since libzypp does not use it in the
// plugin protocol.)
class ProgramOptions {
public:
    string plugin_config;
    string snapper_config;
    DBusBusType bus;

    ProgramOptions()
    : plugin_config("/etc/snapper/zypp-plugin.conf")
    , snapper_config("root")
    , bus(DBUS_BUS_SYSTEM)
    {
	const char * s;

	s = getenv("SNAPPER_ZYPP_PLUGIN_CONFIG");
	if (s != nullptr) {
	    plugin_config = s;
	}

	s = getenv("SNAPPER_ZYPP_PLUGIN_SNAPPER_CONFIG");
	if (s != nullptr) {
	    snapper_config = s;
	}

	s = getenv("SNAPPER_ZYPP_PLUGIN_DBUS_SESSION");
	if (s != nullptr) {
            bus = DBUS_BUS_SESSION;
        }
    }
};

class SnapperZyppPlugin : public ZyppCommitPlugin {
public:
    struct SolvableMatcher {
	enum class Kind {
	    GLOB,
	    REGEX
	};
	string pattern;
	Kind kind;
	bool important;

	SolvableMatcher(const string& apattern, Kind akind, bool aimportant)
	: pattern(apattern)
	, kind(akind)
	, important(aimportant)
	{}

	bool match(const string& solvable) {
	    cerr << "DEBUG:" << "match? " << solvable << " by " << ((kind == Kind::GLOB)? "GLOB '": "REGEX '") << pattern << '\'' << endl;
	    bool res;
	    if (kind == Kind::GLOB) {
		static const int flags = 0;
		res = fnmatch(pattern.c_str(), solvable.c_str(), flags) == 0;
	    }
	    else {
		// POSIX Extended Regular Expression Syntax
		boost::regex rx_pattern(pattern, boost::regex::extended);
		res = boost::regex_match(solvable, rx_pattern);
	    }
	    cerr << "DEBUG:" << "-> " << res << endl;
	    return res;
	}

	static vector<SolvableMatcher> load_config(const string& cfg_filename) {
	    vector<SolvableMatcher> result;

	    cerr << "DEBUG:" << "parsing " << cfg_filename << endl;
	    XmlFile config(cfg_filename);
	    // FIXME test parse errors
	    const xmlNode* root = config.getRootElement();
	    const xmlNode* solvables_n = getChildNode(root, "solvables");
	    const list<const xmlNode*> solvables_l = getChildNodes(solvables_n, "solvable");
	    for (auto node: solvables_l) {
		string pattern;
		Kind kind;
		bool important = false;

		getAttributeValue(node, "important", important);
		string kind_s;
		getAttributeValue(node, "match", kind_s);
		getValue(node, pattern);
		if (kind_s == "w") { // w for wildcard
		    kind = Kind::GLOB;
		}
		else if (kind_s == "re") { // Regular Expression
		    kind = Kind::REGEX;
		}
		else {
		    cerr << "ERROR:" << "Unknown match attribute '" << kind_s << "', disregarding pattern '"<< pattern << "'" << endl;
		    continue;
		}

		result.emplace_back(SolvableMatcher(pattern, kind, important));
	    }
	    return result;
	}
    };

    SnapperZyppPlugin(const ProgramOptions& opts)
    : snapper_cfg(opts.snapper_config)
    , dbus_conn(opts.bus)
    , pre_snapshot_num(0)
    , solvable_matchers(SolvableMatcher::load_config(opts.plugin_config))
    {
	string caller_prog;
	readlink(sformat("/proc/%d/exe", getppid()), caller_prog);
	snapshot_description = sformat("zypp(%s)", basename(caller_prog).c_str());
    }

    Message plugin_begin(const Message& m) override {
	cerr << "INFO:" << "PLUGINBEGIN" << endl;
	userdata = get_userdata(m);

	return ack();
    }
    Message plugin_end(const Message& m) override {
	cerr << "INFO:" << "PLUGINEND" << endl;
	return ack();
    }
    Message commit_begin(const Message& msg) override {
	cerr << "INFO:" << "COMMITBEGIN" << endl;

	set<string> solvables = get_solvables(msg, true);
	cerr << "DEBUG:" << "solvables: " << solvables << endl;

	bool found, important;
	match_solvables(solvables, found, important);
	cerr << "INFO:" << "found: " << found << ", important: " << important << endl;

	if (found || important) {
	    userdata["important"] = important ? "yes" : "no";

	    try {
		cerr << "INFO:" << "creating pre snapshot" << endl;
		pre_snapshot_num = command_create_pre_snapshot(
                    dbus_conn, snapper_cfg,
                    snapshot_description, cleanup_algorithm, userdata
                );
		cerr << "DEBUG:" << "created pre snapshot " << pre_snapshot_num << endl;
	    }
            catch (const DBus::ErrorException& ex) {
		SN_CAUGHT(ex);
		cerr << "ERROR:" << error_description(ex) << endl;
	    }
	    catch (const Exception& ex) {
		SN_CAUGHT(ex);
	    }
	}

	return ack();
    }

    Message commit_end(const Message& msg) override {
	cerr << "INFO:" << "COMMITEND" << endl;

	if (pre_snapshot_num != 0) {
	    set<string> solvables = get_solvables(msg, false);
	    cerr << "DEBUG:" << "solvables: " << solvables << endl;

	    bool found, important;
	    match_solvables(solvables, found, important);
	    cerr << "INFO:" << "found: " << found << ", important: " << important << endl;

	    if (found || important) {
		userdata["important"] = important ? "yes" : "no";

		try {
		    cerr << "INFO:" << "setting snapshot data" << endl;
		    snapper::SMD modification_data;
		    modification_data.description = snapshot_description;
		    modification_data.cleanup = cleanup_algorithm;
		    modification_data.userdata = userdata;
		    command_set_snapshot(
                        dbus_conn, snapper_cfg,
                        pre_snapshot_num, modification_data
                    );
		}
		catch (const DBus::ErrorException& ex) {
		    SN_CAUGHT(ex);
		    cerr << "ERROR:" << error_description(ex) << endl;
		}
		catch (const Exception& ex) {
		    SN_CAUGHT(ex);
		}
		try {
		    cerr << "INFO:" << "creating post snapshot" << endl;
		    unsigned int post_snapshot_num = command_create_post_snapshot(
                        dbus_conn, snapper_cfg,
                        pre_snapshot_num, "", cleanup_algorithm, userdata
                    );
		    cerr << "DEBUG:" << "created post snapshot " << post_snapshot_num << endl;
		}
		catch (const DBus::ErrorException& ex) {
		    SN_CAUGHT(ex);
		    cerr << "ERROR:" << error_description(ex) << endl;
		}
		catch (const Exception& ex) {
		    SN_CAUGHT(ex);
		}
	    }
	    else {
		try {
		    cerr << "INFO:" << "deleting pre snapshot" << endl;
		    vector<unsigned int> nums{ pre_snapshot_num };
		    bool verbose = false;
		    command_delete_snapshots(dbus_conn, snapper_cfg, nums, verbose);
		    cerr << "DEBUG:" << "deleted pre snapshot " << pre_snapshot_num << endl;
		}
		catch (const DBus::ErrorException& ex) {
		    SN_CAUGHT(ex);
		    cerr << "ERROR:" << error_description(ex) << endl;
		}
		catch (const Exception& ex) {
		    SN_CAUGHT(ex);
		}
	    }
	}
	return ack();
    }

private:
    static const string cleanup_algorithm;

    string snapper_cfg;

    DBus::Connection dbus_conn;
    unsigned int pre_snapshot_num;
    string snapshot_description;
    map<string, string> userdata;

    vector<SolvableMatcher> solvable_matchers;

    map<string, string> get_userdata(const Message&);

    // FIXME: what does the todo flag mean?
    set<string> get_solvables(const Message&, bool todo);

    void match_solvables(const set<string>& solvables, bool& found, bool& important);

    unsigned int create_pre_snapshot(string config_name, string description, string cleanup, map<string, string> userdata);
};

const string SnapperZyppPlugin::cleanup_algorithm = "number";

map<string, string> SnapperZyppPlugin::get_userdata(const Message& msg) {
    map<string, string> result;
    auto it = msg.headers.find("userdata");
    if (it != msg.headers.end()) {
	const string& userdata_s = it->second;
        vector<string> key_values;
	boost::split(key_values, userdata_s, boost::is_any_of(","));
	for (auto kv: key_values) {
	    static const boost::regex rx_keyval("([^=]*)=(.+)");
	    boost::smatch what;
	    if (boost::regex_match(kv, what, rx_keyval)) {
		string key = boost::trim_copy(string(what[1]));
		string value = boost::trim_copy(string(what[2]));
		result[key] = value;
	    }
	    else {
		cerr << "ERROR:" << "invalid userdata: expecting comma separated key=value pairs" << endl;
	    }
	}
    }
    return result;
}

set<string> SnapperZyppPlugin::get_solvables(const Message& msg, bool todo) {
    set<string> result;

    rapidjson::Document doc;
    const char * c_body = msg.body.c_str();
    cerr << "DEBUG:" << "parsing zypp JSON: " << c_body << endl;
    if (doc.Parse(c_body).HasParseError()) {
	cerr << "ERROR:" << "parsing zypp JSON failed" << endl;
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

void SnapperZyppPlugin::match_solvables(const set<string>& solvables, bool& found, bool& important) {
    found = false;
    important = false;
    for (auto s: solvables) {
	for (auto matcher: solvable_matchers) {
	    if (matcher.match(s)) {
		found = true;
		important = important || matcher.important;
		if (found && important)
		    return; // short circuit
	    }
	}
    }
}

int main() {
    if (getenv("DISABLE_SNAPPER_ZYPP_PLUGIN") != nullptr) {
	cerr << "INFO:" << "$DISABLE_SNAPPER_ZYPP_PLUGIN is set - disabling snapper-zypp-plugin" << endl;
	ZyppCommitPlugin plugin;
	return plugin.main();
    }

    ProgramOptions options;
    SnapperZyppPlugin plugin(options);
    return plugin.main();
}
