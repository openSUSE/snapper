/*
 * Copyright (c) [2011-2015] Novell, Inc.
 * Copyright (c) [2016-2023] SUSE LLC
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

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <boost/algorithm/string.hpp>

#include <snapper/Snapper.h>
#include <snapper/SnapperTmpl.h>
#include <snapper/Enum.h>
#include <snapper/Version.h>

#include "../utils/text.h"
#include "../utils/Table.h"
#include "../utils/GetOpts.h"
#include "../proxy/errors.h"
#include "../proxy/proxy.h"

#include "GlobalOptions.h"
#include "cmd.h"


using namespace snapper;
using namespace std;


struct Cmd
{
    typedef void (*cmd_func_t)(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers* snappers,
			       ProxySnapper* snapper, Plugins::Report& report);

    typedef void (*help_func_t)();

    Cmd(const string& name, cmd_func_t cmd_func, help_func_t help_func, bool needs_snapper)
	: name(name), cmd_func(cmd_func), help_func(help_func), needs_snapper(needs_snapper)
    {}

    Cmd(const string& name, const vector<string>& aliases, cmd_func_t cmd_func,
	help_func_t help_func, bool needs_snapper)
	: name(name), aliases(aliases), cmd_func(cmd_func), help_func(help_func),
	  needs_snapper(needs_snapper)
    {}

    const string name;
    const vector<string> aliases;
    const cmd_func_t cmd_func;
    const help_func_t help_func;
    const bool needs_snapper;
};


static bool log_debug = false;


void
log_do(LogLevel level, const string& component, const char* file, const int line, const char* func,
       const string& text)
{
    cerr << text << endl;
}


bool
log_query(LogLevel level, const string& component)
{
    return log_debug || level == ERROR;
}


void usage() __attribute__ ((__noreturn__));

void
usage()
{
    cerr << "Try 'snapper --help' for more information." << endl;
    exit(EXIT_FAILURE);
}


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

    cout << _("usage: snapper [--global-options] <command> [--command-options] [command-arguments]") << '\n'
	 << endl;

    GlobalOptions::help_global_options();

    for (const Cmd& cmd : cmds)
	(*cmd.help_func)();

    exit(EXIT_SUCCESS);
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

    setLogDo(&log_do);
    setLogQuery(&log_query);

    const vector<Cmd> cmds = {
	Cmd("list-configs", command_list_configs, help_list_configs, false),
	Cmd("create-config", command_create_config, help_create_config, false),
	Cmd("delete-config", command_delete_config, help_delete_config, false),
	Cmd("get-config", command_get_config, help_get_config, true),
	Cmd("set-config", command_set_config, help_set_config, true),
	Cmd("list", { "ls" }, command_list, help_list, false),
	Cmd("create", command_create, help_create, true),
	Cmd("modify", command_modify, help_modify, true),
	Cmd("delete", { "remove", "rm" }, command_delete, help_delete, true),
	Cmd("mount", command_mount, help_mount, true),
	Cmd("umount", command_umount, help_umount, true),
	Cmd("status", command_status, help_status, true),
	Cmd("diff", command_diff, help_diff, true),
#ifdef ENABLE_XATTRS
	Cmd("xadiff", command_xadiff, help_xadiff, true),
#endif
	Cmd("undochange", command_undochange, help_undochange, true),
#ifdef ENABLE_ROLLBACK
	Cmd("rollback", command_rollback, help_rollback, true),
#endif
	Cmd("setup-quota", command_setup_quota, help_setup_quota, true),
	Cmd("cleanup", command_cleanup, help_cleanup, false),
	Cmd("debug", command_debug, help_debug, false)
    };

    int exit_status = EXIT_SUCCESS;

    try
    {
	GetOpts get_opts(argc, argv);

	GlobalOptions global_options(get_opts);

	if (global_options.debug())
	{
	    log_debug = true;
	}

	if (global_options.version())
	{
	    cout << "snapper " << Snapper::compileVersion() << endl;
	    cout << "libsnapper " << LIBSNAPPER_VERSION_STRING;
	    if (strcmp(LIBSNAPPER_VERSION_STRING, get_libversion_string()) != 0)
		cout << " (" << get_libversion_string() << ")";
	    cout << '\n';
	    cout << "flags " << Snapper::compileFlags() << endl;
	    exit(EXIT_SUCCESS);
	}

	if (global_options.help())
	{
	    help(cmds, get_opts);
	}

	if (!get_opts.has_args())
	{
	    cerr << _("No command provided.") << endl
		 << _("Try 'snapper --help' for more information.") << endl;
	    exit(EXIT_FAILURE);
	}

	const char* command = get_opts.pop_arg();

	vector<Cmd>::const_iterator cmd = cmds.begin();
	while (cmd != cmds.end() && (cmd->name != command && !contains(cmd->aliases, command)))
	    ++cmd;

	if (cmd == cmds.end())
	{
	    cerr << sformat(_("Unknown command '%s'."), command) << endl
		 << _("Try 'snapper --help' for more information.") << endl;
	    exit(EXIT_FAILURE);
	}

	try
	{
	    y2mil("constructing ProxySnapper object");

	    ProxySnappers snappers(global_options.no_dbus() ? ProxySnappers::createLib(global_options.root()) :
				   ProxySnappers::createDbus());

	    y2mil("executing command");

	    Plugins::Report client_report;

	    if (cmd->needs_snapper)
		(*cmd->cmd_func)(global_options, get_opts, &snappers, snappers.getSnapper(global_options.config()),
				 client_report);
	    else
		(*cmd->cmd_func)(global_options, get_opts, &snappers, nullptr, client_report);

	    Plugins::Report server_report = snappers.get_plugins_report();

	    // Plugins can be started from the client or server. So we have to check both
	    // reports.

	    for (const Plugins::Report::Entry& entry : client_report.entries)
	    {
		y2deb("client: " << entry.name << " " << boost::join(entry.args, " ") << " exit-status: " <<
		      entry.exit_status);

		if (entry.exit_status != 0)
		{
		    cerr << sformat(_("Client-side plugin '%s' failed."), entry.name.c_str()) << '\n';
		    exit_status = EXIT_FAILURE;
		}
	    }

	    for (const Plugins::Report::Entry& entry : server_report.entries)
	    {
		y2deb("server: " << entry.name << " " << boost::join(entry.args, " ") << " exit-status: " <<
		      entry.exit_status);

		if (entry.exit_status != 0)
		{
		    cerr << sformat(_("Server-side plugin '%s' failed."), entry.name.c_str()) << '\n';
		    exit_status = EXIT_FAILURE;
		}
	    }
	}
	catch (const DBus::ErrorException& e)
	{
	    SN_CAUGHT(e);

	    if (strcmp(e.name(), "error.unknown_config") == 0 && global_options.config() == "root")
	    {
		cerr << _("The config 'root' does not exist. Likely snapper is not configured.") << endl
		     << _("See 'man snapper' for further instructions.") << endl;
		exit(EXIT_FAILURE);
	    }

	    cerr << error_description(e) << endl;
	    exit(EXIT_FAILURE);
	}
	catch (const DBus::FatalException& e)
	{
	    SN_CAUGHT(e);
	    cerr << _("Failure") << " (" << e.what() << ")." << endl;
	    exit(EXIT_FAILURE);
	}
	catch (const IllegalSnapshotException& e)
	{
	    SN_CAUGHT(e);
	    cerr << _("Illegal snapshot.") << endl;
	    exit(EXIT_FAILURE);
	}
	catch (const ConfigNotFoundException& e)
	{
	    SN_CAUGHT(e);
	    cerr << sformat(_("Config '%s' not found."), global_options.config().c_str()) << endl;
	    exit(EXIT_FAILURE);
	}
	catch (const InvalidConfigException& e)
	{
	    SN_CAUGHT(e);
	    cerr << sformat(_("Config '%s' is invalid."), global_options.config().c_str()) << endl;
	    exit(EXIT_FAILURE);
	}
	catch (const ListConfigsFailedException& e)
	{
	    SN_CAUGHT(e);
	    cerr << sformat(_("Listing configs failed (%s)."), e.what()) << endl;
	    exit(EXIT_FAILURE);
	}
	catch (const CreateConfigFailedException& e)
	{
	    SN_CAUGHT(e);
	    cerr << sformat(_("Creating config failed (%s)."), e.what()) << endl;
	    exit(EXIT_FAILURE);
	}
	catch (const DeleteConfigFailedException& e)
	{
	    SN_CAUGHT(e);
	    cerr << sformat(_("Deleting config failed (%s)."), e.what()) << endl;
	    exit(EXIT_FAILURE);
	}
	catch (const InvalidConfigdataException& e)
	{
	    SN_CAUGHT(e);
	    cerr << _("Invalid configdata.") << endl;
	    exit(EXIT_FAILURE);
	}
	catch (const AclException& e)
	{
	    SN_CAUGHT(e);
	    cerr << _("ACL error.") << endl;
	    exit(EXIT_FAILURE);
	}
	catch (const IOErrorException& e)
	{
	    SN_CAUGHT(e);
	    cerr << sformat(_("IO error (%s)."), e.what()) << endl;
	    exit(EXIT_FAILURE);
	}
	catch (const InvalidUserException& e)
	{
	    SN_CAUGHT(e);
	    cerr << sformat(_("Invalid user (%s)."), e.what()) << endl;
	    exit(EXIT_FAILURE);
	}
	catch (const InvalidGroupException& e)
	{
	    SN_CAUGHT(e);
	    cerr << sformat(_("Invalid group (%s)."), e.what()) << endl;
	    exit(EXIT_FAILURE);
	}
	catch (const QuotaException& e)
	{
	    SN_CAUGHT(e);
	    cerr << sformat(_("Quota error (%s)."), e.what()) << endl;
	    exit(EXIT_FAILURE);
	}
	catch (const FreeSpaceException& e)
	{
	    SN_CAUGHT(e);
	    cerr << sformat(_("Free space error (%s)."), e.what()) << endl;
	    exit(EXIT_FAILURE);
	}
	catch (const OptionsException& e)
	{
	    SN_CAUGHT(e);
	    cerr << e.what() << endl
		 << _("Try 'snapper --help' for more information.") << endl;
	    exit(EXIT_FAILURE);
	}
	catch (const CleanupException& e)
	{
	    SN_CAUGHT(e);
	    cerr << e.what() << endl;
	    exit(EXIT_FAILURE);
	}
	catch (const Exception& e)
	{
	    SN_CAUGHT(e);
	    cerr << sformat(_("Error (%s)."), e.what()) << endl;
	    exit(EXIT_FAILURE);
	}
    }
    catch (const OptionsException& e)
    {
	SN_CAUGHT(e);
	cerr << e.what() << endl
	     << _("Try 'snapper --help' for more information.") << endl;
	exit(EXIT_FAILURE);
    }

    y2deb("exit-status: " << exit_status);
    exit(exit_status);
}
