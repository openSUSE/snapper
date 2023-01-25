/*
 * Copyright (c) [2019-2023] SUSE LLC
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

#ifndef SNAPPER_ZYPP_COMMIT_PLUGIN_H
#define SNAPPER_ZYPP_COMMIT_PLUGIN_H

#include "zypp-commit-plugin.h"
#include "solvable-matcher.h"


// Normally the only configuration this program needs is
// the zypp-plugin.conf file in /etc/snapper or /usr/share/snapper.
// But for testing we need more places to inject mocks.
// This is done with SNAPPER_ZYPP_PLUGIN_* environment variables.
// (Using argv is not useful since libzypp does not use it in the
// plugin protocol.)
class ProgramOptions
{
public:

    ProgramOptions();

    string plugin_config;
    string snapper_config = "root";
    DBusBusType bus = DBUS_BUS_SYSTEM;

};


class SnapperZyppCommitPlugin : public ZyppCommitPlugin
{
public:

    SnapperZyppCommitPlugin(const ProgramOptions& opts);

    Message plugin_begin(const Message& msg) override;
    Message plugin_end(const Message& msg) override;

    Message commit_begin(const Message& msg) override;
    Message commit_end(const Message& msg) override;

private:

    static const string cleanup_algorithm;

    const string snapper_cfg;

    DBus::Connection dbus_conn;
    unsigned int pre_snapshot_num;
    string snapshot_description;
    map<string, string> userdata;

    vector<SolvableMatcher> solvable_matchers;

    map<string, string> get_userdata(const Message& msg);

    enum class Phase { BEFORE, AFTER };

    std::set<string> get_solvables(const Message& msg, Phase phase) const;

    void match_solvables(const std::set<string>& solvables, bool& found, bool& important) const;

};


#endif
