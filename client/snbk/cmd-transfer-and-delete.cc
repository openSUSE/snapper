/*
 * Copyright (c) [2024-2026] SUSE LLC
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


#include <iostream>

#include "../utils/text.h"

#include "GlobalOptions.h"
#include "cmd.h"


namespace snapper
{

    using namespace std;


    void
    help_transfer_and_delete()
    {
	cout << "  " << _("Transfer and delete:") << '\n'
	     << "\t" << _("snbk transfer-and-delete") << '\n'
	     << '\n'
	     << "\t" << _("A backward-compatibility alias for 'transfer-and-cleanup'.")
	     << '\n'
	     << '\n';
    }


    void
    command_transfer_and_delete(const GlobalOptions& global_options, GetOpts& get_opts,
				const BackupConfigs& backup_configs, ProxySnappers* snappers)
    {
	const char* warning_msg =
	    _("The 'transfer-and-delete' command has been renamed to "
	      "'transfer-and-cleanup' and may be removed in the future. Please use the "
	      "'transfer-and-cleanup' command instead.");

	cout << _("WARNING: ") << warning_msg << '\n' << '\n';
	y2war(warning_msg);

	command_transfer_and_cleanup(global_options, get_opts, backup_configs, snappers);
    }

}
