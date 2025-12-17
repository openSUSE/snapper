/*
 * Copyright (c) [2011-2015] Novell, Inc.
 * Copyright (c) [2016-2024] SUSE LLC
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


#include "config.h"

#include "../proxy/proxy.h"


namespace snapper
{

    void
    help_list_configs();

    void
    command_list_configs(const GlobalOptions& global_options, GetOpts& get_opts, BackupConfigs& backup_configs,
			 ProxySnappers* snappers);


    void
    help_list();

    void
    command_list(const GlobalOptions& global_options, GetOpts& get_opts, BackupConfigs& backup_configs,
		 ProxySnappers* snappers);


    void
    help_transfer();

    void
    command_transfer(const GlobalOptions& global_options, GetOpts& get_opts, BackupConfigs& backup_configs,
		     ProxySnappers* snappers);


    void
    help_restore();

    void
    command_restore(const GlobalOptions& global_options, GetOpts& get_opts, BackupConfigs& backup_configs,
		     ProxySnappers* snappers);


    void
    help_delete();

    void
    command_delete(const GlobalOptions& global_options, GetOpts& get_opts, BackupConfigs& backup_configs,
		   ProxySnappers* snappers);


    void
    help_transfer_and_delete();

    void
    command_transfer_and_delete(const GlobalOptions& global_options, GetOpts& get_opts, BackupConfigs& backup_configs,
				ProxySnappers* snappers);

}
