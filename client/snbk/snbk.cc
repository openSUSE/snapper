/*
 * Copyright (c) [2024-2025] SUSE LLC
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
#include <memory>

#include "snapper/Logger.h"
#include <snapper/SnapperTmpl.h>
#include <snapper/Enum.h>
#include "snapper/SnapperDefines.h"

#include "../utils/text.h"
#include "../utils/GetOpts.h"

#include "GlobalOptions.h"
#include "BackupConfig.h"
#include "cmd.h"


using namespace std;
using namespace snapper;


struct Cmd
{
    typedef void (*cmd_func_t)(const GlobalOptions& global_options, GetOpts& get_opts,
			       BackupConfigs& backup_configs, ProxySnappers* snappers);

    typedef void (*help_func_t)();

    Cmd(const string& name, cmd_func_t cmd_func, help_func_t help_func, bool needs_snapper)
	: name(name), cmd_func(cmd_func), help_func(help_func), needs_snapper(needs_snapper)
    {}

    Cmd(const string& name, const vector<string>& aliases, cmd_func_t cmd_func, help_func_t help_func,
	bool needs_snapper)
	: name(name), aliases(aliases), cmd_func(cmd_func), help_func(help_func),
	  needs_snapper(needs_snapper)
    {}

    const string name;
    const vector<string> aliases;
    const cmd_func_t cmd_func;
    const help_func_t help_func;
    const bool needs_snapper;
};


void help() __attribute__ ((__noreturn__));


void
help(const vector<Cmd>& cmds, GetOpts& get_opts)
{
    get_opts.parse("help", GetOpts::no_options);
    if (get_opts.has_args())
    {
	cerr << _("Command 'help' does not take arguments.") << endl;
	exit(EXIT_FAILURE);
    }

    cout << _("usage: snbk [--global-options] <command> [command-arguments]") << '\n'
	 << endl;

    GlobalOptions::help_global_options();

    for (const Cmd& cmd : cmds)
	(*cmd.help_func)();

    exit(EXIT_SUCCESS);
}


vector<string>
get_backup_configs(const GlobalOptions& global_options, const Cmd* cmd)
{
    if (cmd->name == "list-configs")
	return read_backup_config_names();

    if (global_options.backup_config())
	return { global_options.backup_config().value() };

    const vector<string> names = read_backup_config_names();
    if (names.empty())
	SN_THROW(Exception(_("No backup configs found.")));

    return names;
}


int
main(int argc, char** argv)
{
    try
    {
	locale::global(locale(""));
    }
    catch (const runtime_error& e)
    {
	cerr << _("Failed to set locale.") << endl;
    }

    set_logger(get_stdout_logger());

    const vector<Cmd> cmds = {
	Cmd("list-configs", command_list_configs, help_list_configs, false),
	Cmd("list", { "ls" }, command_list, help_list, true),
	Cmd("transfer", command_transfer, help_transfer, true),
	Cmd("delete", { "remove", "rm" }, command_delete, help_delete, true),
	Cmd("transfer-and-delete", command_transfer_and_delete, help_transfer_and_delete, true),
    };

    try
    {
	GetOpts get_opts(argc, argv);

	GlobalOptions global_options(get_opts);

	if (global_options.debug())
	    set_logger_tresshold(LogLevel::DEBUG);

	if (global_options.version())
	{
	    cout << "snbk " << Snapper::compileVersion() << endl;
	    exit(EXIT_SUCCESS);
	}

	if (global_options.help())
	{
	    help(cmds, get_opts);
	}

	if (!get_opts.has_args())
	{
	    cerr << _("No command provided.") << endl
		 << _("Try 'snbk --help' for more information.") << endl;
	    exit(EXIT_FAILURE);
	}

	const char* command = get_opts.pop_arg();

	vector<Cmd>::const_iterator cmd = cmds.begin();
	while (cmd != cmds.end() && (cmd->name != command && !contains(cmd->aliases, command)))
	    ++cmd;

	if (cmd == cmds.end())
	{
	    cerr << sformat(_("Unknown command '%s'."), command) << endl
		 << _("Try 'snbk --help' for more information.") << endl;
	    exit(EXIT_FAILURE);
	}

	try
	{
	    const vector<string> names = get_backup_configs(global_options, &*cmd);

	    BackupConfigs backup_configs;

	    for (const string& name : names)
	    {
		BackupConfig backup_config(name);

		if (global_options.target_mode() &&
		    backup_config.target_mode != global_options.target_mode().value())
		    continue;

		if (global_options.automatic() && !backup_config.automatic)
		    continue;

		backup_configs.push_back(backup_config);
	    }

	    unique_ptr<ProxySnappers> snappers;

	    if (cmd->needs_snapper)
	    {
		snappers = make_unique<ProxySnappers>(global_options.no_dbus() ? ProxySnappers::createLib("/") :
						      ProxySnappers::createDbus());
	    }

	    (*cmd->cmd_func)(global_options, get_opts, backup_configs, snappers.get());
	}
	catch (const Exception& e)
	{
	    SN_CAUGHT(e);

	    cerr << e.what() << endl;

	    exit(EXIT_FAILURE);
	}
    }
    catch (const OptionsException& e)
    {
	SN_CAUGHT(e);

	cerr << e.what() << '\n'
	     << _("Try 'snbk --help' for more information.") << endl;

	exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
