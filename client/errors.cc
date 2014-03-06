/*
 * Copyright (c) [2011-2014] Novell, Inc.
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
 * with this program; if not, contact Novell, Inc.
 *
 * To contact Novell about this file by physical or electronic mail, you may
 * find current contact information at www.novell.com.
 */


#include <snapper/AppUtil.h>

#include "utils/text.h"

#include "errors.h"


using namespace std;
using namespace snapper;


string
error_description(const DBus::ErrorException& e)
{
    string name = e.name();

    if (name == "error.unknown_config")
	return _("Unknown config.");

    if (name == "error.no_permissions")
	return _("No permissions.");

    if (name == "error.invalid_userdata")
	return _("Invalid userdata.");

    if (name == "error.invalid_configdata")
	return _("Invalid configdata.");

    if (name == "error.illegal_snapshot")
	return _("Illegal Snapshot.");

    if (name == "error.config_locked")
	return _("Config is locked.");

    if (name == "error.config_in_use")
	return _("Config is in use.");

    if (name == "error.snapshot_in_use")
	return _("Snapshot is in use.");

    if (name == "error.unknown_file")
	return _("Unknown file.");

    if (name == "error.io_error")
	return _("IO Error.");

    if (name == "error.create_config_failed")
	return sformat(_("Creating config failed (%s)."), e.message());

    if (name == "error.delete_config_failed")
	return sformat(_("Deleting config failed (%s)."), e.message());

    if (name == "error.create_snapshot_failed")
	return _("Creating snapshot failed.");

    if (name == "error.delete_snapshot_failed")
	return _("Deleting snapshot failed.");

    if (name == "error.invalid_user")
	return _("Invalid user.");

    if (name == "error.invalid_group")
	return _("Invalid group.");

    if (name == "error.acl_error")
	return _("ACL error.");

    return sformat(_("Failure (%s)."), name.c_str());
}
