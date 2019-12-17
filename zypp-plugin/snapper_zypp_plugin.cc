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

// getenv
#include <stdlib.h>
// getppid
#include <sys/types.h>
#include <unistd.h>

// split
#include <boost/algorithm/string.hpp>

#include <iostream>
#include <map>
#include <string>
using namespace std;

#include <json.h>
// a collision with client/errors.h
#ifdef error_description
#undef error_description
#endif

#include "dbus/DBusConnection.h"
#include "snapper/Regex.h"
#include "snapper/Exception.h"
using snapper::Exception;
using snapper::CodeLocation;
#include "client/commands.h"
#include "client/errors.h"

#include "zypp_commit_plugin.h"
#include "solvable_matcher.h"

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

	set<string> solvables = get_solvables(msg, Phase::BEFORE);
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
	    set<string> solvables = get_solvables(msg, Phase::AFTER);
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

    enum class Phase { BEFORE, AFTER };

    set<string> get_solvables(const Message&, Phase phase);

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
	    static const snapper::Regex rx_keyval("^([^=]*)=(.+)$");
	    if (rx_keyval.match(kv)) {
		string key = boost::trim_copy(rx_keyval.cap(1));
		string value = boost::trim_copy(rx_keyval.cap(2));
		result[key] = value;
	    }
	    else {
		cerr << "ERROR:" << "invalid userdata: expecting comma separated key=value pairs" << endl;
	    }
	}
    }
    return result;
}

static
json_object * object_get(json_object * obj, const char * name) {
    json_object * result;
    if (!json_object_object_get_ex(obj, name, &result)) {
	cerr << "ERROR:" << '"' << name << "\" not found" << endl;
	return NULL;
    }
    return result;
}

set<string> SnapperZyppPlugin::get_solvables(const Message& msg, Phase phase) {
    set<string> result;

    json_tokener * tok = json_tokener_new();
    json_object * zypp = json_tokener_parse_ex(tok, msg.body.c_str(), msg.body.size());
    json_tokener_error jerr = json_tokener_get_error(tok);
    if (jerr != json_tokener_success) {
	cerr << "ERROR:" << "parsing zypp JSON failed: "
			 << json_tokener_error_desc(jerr) << endl;
	return result;
    }

    // JSON structure:
    // {"TransactionStepList":[{"type":"?","stage":"?","solvable":{"n":"mypackage"}}]}
    // https://doc.opensuse.org/projects/libzypp/SLE12SP2/plugin-commit.html
    json_object * steps = object_get(zypp, "TransactionStepList");
    if (!steps)
	return result;

    if (json_object_get_type(steps) == json_type_array) {
	size_t i, len = json_object_array_length(steps);
	printf("steps: %zu\n", len);
	for (i = 0; i < len; ++i) {
	    json_object * step = json_object_array_get_idx(steps, i);
	    bool have_type = json_object_object_get_ex(step, "type", NULL);
	    bool have_stage = json_object_object_get_ex(step, "stage", NULL);
	    if (have_type && (phase == Phase::BEFORE || have_stage)) {
		json_object * solvable = object_get(step, "solvable");
		if (!solvable) {
		    cerr << "ERROR:" << "in item #" << i << endl;
		    continue;
		}
		json_object * name = object_get(solvable, "n");
		if (!name) {
		    cerr << "ERROR:" << "in item #" << i << endl;
		    continue;
		}
		if (json_object_get_type(name) != json_type_string) {
		    cerr << "ERROR:" << "\"n\" is not a string" << endl;
		    cerr << "ERROR:" << "in item #" << i << endl;
		    continue;
		}
		else {
		    const char * prize = json_object_get_string(name);
		    result.insert(prize);
		}
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
