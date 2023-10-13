/*
 * Copyright (c) 2023 SUSE LLC
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


#include "snapper/SystemCmd.h"
#include "snapper/SnapperDefines.h"


namespace snapper
{

    void
    systemctl_enable_unit(bool enable, bool now, const string& name)
    {
	// When run in a chroot system the enable command works but the start command
	// fails (which is likely what we want).

	SystemCmd::Args cmd_args = { SYSTEMCTL_BIN, enable ? "enable" : "disable" };
	if (now)
	    cmd_args << "--now";
	cmd_args << name;

	SystemCmd cmd(cmd_args);
    }


    void
    systemctl_enable_timeline(bool enable, bool now)
    {
	systemctl_enable_unit(enable, now, "snapper-timeline.timer");
    }

}
