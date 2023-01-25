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


#include "zypp-commit-plugin.h"


ZyppPlugin::Message
ZyppCommitPlugin::dispatch(const Message& msg)
{
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
