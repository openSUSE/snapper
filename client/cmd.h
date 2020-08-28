/*
 * Copyright (c) [2011-2015] Novell, Inc.
 * Copyright (c) [2016-2020] SUSE LLC
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

#include "proxy.h"
#include "GlobalOptions.h"


namespace snapper
{

    void
    help_list_configs();

    void
    command_list_configs(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers* snappers, ProxySnapper*);


    void
    help_create_config();

    void
    command_create_config(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers* snappers, ProxySnapper*);


    void
    help_delete_config();

    void
    command_delete_config(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers* snappers, ProxySnapper*);


    void
    help_get_config();

    void
    command_get_config(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers* snappers, ProxySnapper*);


    void
    help_set_config();

    void
    command_set_config(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers* snappers, ProxySnapper*);


    void
    help_list();

    void
    command_list(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers* snappers, ProxySnapper*);


    void
    help_create();

    void
    command_create(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers* snappers, ProxySnapper* snapper);


    void
    help_modify();

    void
    command_modify(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers* snappers, ProxySnapper* snapper);


    void
    help_delete();

    void
    command_delete(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers* snappers, ProxySnapper* snapper);


    void
    help_mount();

    void
    command_mount(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers* snappers, ProxySnapper* snapper);


    void
    help_umount();

    void
    command_umount(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers* snappers, ProxySnapper* snapper);


    void
    help_status();

    void
    command_status(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers* snappers, ProxySnapper* snapper);


    void
    help_diff();

    void
    command_diff(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers* snappers, ProxySnapper* snapper);


#ifdef ENABLE_XATTRS

    void
    help_xadiff();

    void
    command_xadiff(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers* snappers, ProxySnapper* snapper);

#endif


    void
    help_undochange();

    void
    command_undochange(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers* snappers, ProxySnapper* snapper);


#ifdef ENABLE_ROLLBACK

    void
    help_rollback();

    void
    command_rollback(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers* snappers, ProxySnapper* snapper);

#endif


    void
    help_setup_quota();

    void
    command_setup_quota(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers* snappers, ProxySnapper* snapper);


    void
    help_cleanup();

    void
    command_cleanup(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers* snappers, ProxySnapper* snapper);


    void
    help_debug();

    void
    command_debug(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers* snappers, ProxySnapper* snapper);

}
