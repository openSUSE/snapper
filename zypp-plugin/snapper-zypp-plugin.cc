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


#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <map>
#include <set>
#include <string>
#include <regex>
#include <boost/algorithm/string.hpp>

#include <json.h>
// a collision with client/proxy/errors.h
#ifdef error_description
#undef error_description
#endif

#include "dbus/DBusConnection.h"
#include "snapper/Exception.h"
#include "snapper/Log.h"
#include "client/proxy/commands.h"
#include "client/proxy/errors.h"

#include "snapper-zypp-plugin.h"

using namespace std;
using namespace snapper;


ostream&
operator<<(ostream& os, const set<string>& ss)
{
    os << '{';
    for (typename set<string>::const_iterator it = ss.begin(); it != ss.end(); ++it)
    {
	if (it != ss.begin())
	    os << ", ";
	os << *it;
    }
    os << '}';
    return os;
}


ProgramOptions::ProgramOptions()
{
    const char* s;

    s = getenv("SNAPPER_ZYPP_PLUGIN_CONFIG");
    if (s)
	plugin_config = s;
    else
	plugin_config = locate_file("zypp-plugin.conf", "/etc/snapper", "/usr/share/snapper");

    s = getenv("SNAPPER_ZYPP_PLUGIN_SNAPPER_CONFIG");
    if (s)
	snapper_config = s;

    s = getenv("SNAPPER_ZYPP_PLUGIN_DBUS_SESSION");
    if (s)
	bus = DBUS_BUS_SESSION;
}


SnapperZyppCommitPlugin::SnapperZyppCommitPlugin(const ProgramOptions& opts)
    : snapper_cfg(opts.snapper_config), dbus_conn(opts.bus), pre_snapshot_num(0),
      solvable_matchers(SolvableMatcher::load_config(opts.plugin_config))
{
    string caller_prog;
    readlink(sformat("/proc/%d/exe", getppid()), caller_prog);
    snapshot_description = sformat("zypp(%s)", basename(caller_prog).c_str());
}


ZyppCommitPlugin::Message
SnapperZyppCommitPlugin::plugin_begin(const Message& msg)
{
    y2mil("PLUGIN BEGIN");

    userdata = get_userdata(msg);

    return ack();
}


ZyppCommitPlugin::Message
SnapperZyppCommitPlugin::plugin_end(const Message& msg)
{
    y2mil("PLUGIN END");

    return ack();
}


ZyppCommitPlugin::Message
SnapperZyppCommitPlugin::commit_begin(const Message& msg)
{
    y2mil("COMMIT BEGIN");

    set<string> solvables = get_solvables(msg, Phase::BEFORE);
    y2deb("solvables: " << solvables);

    bool found, important;
    match_solvables(solvables, found, important);
    y2mil("found: " << found << ", important: " << important);

    if (found || important)
    {
	try
	{
	    y2deb("lock config");
	    command_lock_config(dbus_conn, snapper_cfg);
	}
	catch (const Exception& ex)
	{
	    SN_CAUGHT(ex);
	}

	userdata["important"] = important ? "yes" : "no";

	try
	{
	    y2mil("creating pre snapshot");
	    pre_snapshot_num = command_create_pre_snapshot(dbus_conn, snapper_cfg, snapshot_description,
							   cleanup_algorithm, userdata);
	    y2deb("created pre snapshot " << pre_snapshot_num);
	}
	catch (const DBus::ErrorException& ex)
	{
	    SN_CAUGHT(ex);
	    y2err(error_description(ex));
	    y2err(ex.name());
	    y2err(ex.message());
	}
	catch (const Exception& ex)
	{
	    SN_CAUGHT(ex);
	}
    }

    return ack();
}


ZyppCommitPlugin::Message
SnapperZyppCommitPlugin::commit_end(const Message& msg)
{
    y2mil("COMMIT END");

    if (pre_snapshot_num != 0)
    {
	set<string> solvables = get_solvables(msg, Phase::AFTER);
	y2deb("solvables: " << solvables);

	bool found, important;
	match_solvables(solvables, found, important);
	y2mil("found: " << found << ", important: " << important);

	if (found || important)
	{
	    userdata["important"] = important ? "yes" : "no";

	    try
	    {
		y2mil("setting snapshot data");
		snapper::SMD modification_data;
		modification_data.description = snapshot_description;
		modification_data.cleanup = cleanup_algorithm;
		modification_data.userdata = userdata;
		command_set_snapshot(dbus_conn, snapper_cfg, pre_snapshot_num, modification_data);
	    }
	    catch (const DBus::ErrorException& ex)
	    {
		SN_CAUGHT(ex);
		y2err(error_description(ex));
		y2err(ex.name());
		y2err(ex.message());
	    }
	    catch (const Exception& ex)
	    {
		SN_CAUGHT(ex);
	    }

	    try
	    {
		y2mil("creating post snapshot");
		unsigned int post_snapshot_num = command_create_post_snapshot(dbus_conn, snapper_cfg,
									      pre_snapshot_num, "",
									      cleanup_algorithm,
									      userdata);
		y2deb("created post snapshot " << post_snapshot_num);
	    }
	    catch (const DBus::ErrorException& ex)
	    {
		SN_CAUGHT(ex);
		y2err(error_description(ex));
		y2err(ex.name());
		y2err(ex.message());
	    }
	    catch (const Exception& ex)
	    {
		SN_CAUGHT(ex);
	    }
	}
	else
	{
	    try
	    {
		y2mil("deleting pre snapshot");
		vector<unsigned int> nums{ pre_snapshot_num };
		bool verbose = false;
		command_delete_snapshots(dbus_conn, snapper_cfg, nums, verbose);
		y2deb("deleted pre snapshot " << pre_snapshot_num);
	    }
	    catch (const DBus::ErrorException& ex)
	    {
		SN_CAUGHT(ex);
		y2err(error_description(ex));
		y2err(ex.name());
		y2err(ex.message());
	    }
	    catch (const Exception& ex)
	    {
		SN_CAUGHT(ex);
	    }
	}

	try
	{
	    y2deb("unlock config");
	    command_unlock_config(dbus_conn, snapper_cfg);
	}
	catch (const Exception& ex)
	{
	    SN_CAUGHT(ex);
	}
    }

    return ack();
}


const string SnapperZyppCommitPlugin::cleanup_algorithm = "number";


// The user can provide userdata e.g. using zypper ('zypper --userdata foo=bar install
// barrel').

map<string, string>
SnapperZyppCommitPlugin::get_userdata(const Message& msg)
{
    map<string, string> result;

    auto it = msg.headers.find("userdata");
    if (it != msg.headers.end())
    {
	const string& userdata_s = it->second;
	vector<string> key_values;
	boost::split(key_values, userdata_s, boost::is_any_of(","));
	for (const string& kv : key_values)
	{
	    static const regex rx_keyval("([^=]*)=(.+)", regex::extended);
	    smatch match;

	    if (regex_match(kv, match, rx_keyval))
	    {
		string key = boost::trim_copy(match[1].str());
		string value = boost::trim_copy(match[2].str());
		result[key] = value;
	    }
	    else
	    {
		y2err("invalid userdata: expecting comma separated key=value pairs");
	    }
	}
    }

    return result;
}


static json_object*
object_get(json_object* obj, const char* name)
{
    json_object* result;
    if (!json_object_object_get_ex(obj, name, &result))
    {
	y2err('"' << name << "\" not found");
	return NULL;
    }
    return result;
}


class JsonTokener
{
public:

    JsonTokener()
	: p(json_tokener_new())
    {
	if (!p)
	    throw runtime_error("out of memory");
    }

    ~JsonTokener()
    {
	json_tokener_free(p);
    }

    json_tokener* get() { return p; }

private:

    json_tokener* p;

};


set<string>
SnapperZyppCommitPlugin::get_solvables(const Message& msg, Phase phase) const
{
    set<string> result;

    JsonTokener tokener;
    json_object* zypp = json_tokener_parse_ex(tokener.get(), msg.body.c_str(), msg.body.size());
    json_tokener_error jerr = json_tokener_get_error(tokener.get());
    if (jerr != json_tokener_success)
    {
	y2err("parsing zypp JSON failed: " << json_tokener_error_desc(jerr));
	return result;
    }

    // JSON structure:
    // {"TransactionStepList":[{"type":"?","stage":"?","solvable":{"n":"mypackage"}}]}
    // https://doc.opensuse.org/projects/libzypp/SLE12SP2/plugin-commit.html
    json_object* steps = object_get(zypp, "TransactionStepList");
    if (!steps)
	return result;

    if (json_object_get_type(steps) == json_type_array)
    {
	size_t len = json_object_array_length(steps);
	for (size_t i = 0; i < len; ++i)
	{
	    json_object* step = json_object_array_get_idx(steps, i);
	    bool have_type = json_object_object_get_ex(step, "type", NULL);
	    bool have_stage = json_object_object_get_ex(step, "stage", NULL);
	    if (have_type && (phase == Phase::BEFORE || have_stage))
	    {
		json_object* solvable = object_get(step, "solvable");
		if (!solvable)
		{
		    y2err("in item #" << i);
		    continue;
		}

		json_object* name = object_get(solvable, "n");
		if (!name)
		{
		    y2err("in item #" << i);
		    continue;
		}

		if (json_object_get_type(name) != json_type_string)
		{
		    y2err("\"n\" is not a string");
		    y2err("in item #" << i);
		    continue;
		}
		else
		{
		    const char* prize = json_object_get_string(name);
		    result.insert(prize);
		}
	    }
	}
    }

    return result;
}


void
SnapperZyppCommitPlugin::match_solvables(const set<string>& solvables, bool& found, bool& important) const
{
    found = important = false;

    for (const string& solvable : solvables)
    {
	for (const SolvableMatcher& solvable_matcher : solvable_matchers)
	{
	    if (solvable_matcher.match(solvable))
	    {
		found = true;

		if (solvable_matcher.is_important())
		{
		    important = true;
		    return; // short circuit
		}
	    }
	}
    }
}


static bool log_debug = false;


static bool
simple_log_query(LogLevel level, const string& component)
{
    return log_debug || level != DEBUG;
}


static void
simple_log_do(LogLevel level, const string& component, const char* file, int line,
	      const char* func, const string& text)
{
    static const char* ln[4] = { "DEB", "MIL", "WAR", "ERR" };

    cerr << ln[level] << ' ' << text << endl;
}


int
main()
{
    setLogQuery(simple_log_query);
    setLogDo(simple_log_do);

    if (getenv("SNAPPER_ZYPP_PLUGIN_DEBUG"))
    {
	y2mil("enabling debug logging of snapper-zypp-plugin");

	log_debug = true;
    }

    if (getenv("DISABLE_SNAPPER_ZYPP_PLUGIN"))
    {
	y2mil("$DISABLE_SNAPPER_ZYPP_PLUGIN is set - disabling snapper-zypp-plugin");

	DummyZyppCommitPlugin plugin;
	return plugin.main();
    }

    ProgramOptions options;
    SnapperZyppCommitPlugin plugin(options);
    return plugin.main();
}
