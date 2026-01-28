/*
 * Copyright (c) 2026 SUSE LLC
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

#include "../misc.h"
#include "../utils/help.h"

#include "BackupConfig.h"
#include "GlobalOptions.h"
#include "TheBigThing.h"


namespace snapper
{
    using namespace std;

    namespace
    {

	enum class Mode
	{
	    SOURCE_TREE,
	    TARGET_TREE
	};

    } // namespace


    template <> struct EnumInfo<Mode>
    {
	static const vector<string> names;
    };

    const vector<string> EnumInfo<Mode>::names({
        "source-tree",
        "target-tree",
    });


    void
    help_visualize()
    {
	cout << "  " << _("Produce a specific graph in Graphviz DOT format:") << '\n'
	     << "\t" << _("snbk visualize <mode>") << '\n'
	     << "\n"
	     << "\t" << _("Supported modes:") << '\n'
	     << "\t" << _("- source-tree: Produce a tree diagram of the snapshots on the source.") << '\n'
	     << "\t" << _("- target-tree: Produce a tree diagram of the snapshots on the target.") << '\n'
	     << '\n'
	     << "    " << _("Options for the 'visualize' command:") << '\n';

	print_options({
	    { _("--rankdir, -r"), _("The 'rankdir' diagram attribute of Graphviz. Defaults to 'LR'.") },
	});
    }


    void
    command_visualize(const GlobalOptions& global_options, GetOpts& get_opts,
                           const BackupConfigs& backup_configs, ProxySnappers* snappers)
    {
	// Drawing a graph for multiple backup configs is not supported.
	if (backup_configs.size() != 1)
	{
	    SN_THROW(OptionsException(_("A backup-config must be specified to run this "
	                                "command.")));
	}

	// Check and parse arguments
	const vector<Option> options = {
	    Option("rankdir",	required_argument,	'r')
	};

	ParsedOpts opts = get_opts.parse("visualize", options);
	if (get_opts.num_args() != 1)
	{
	    SN_THROW(OptionsException(_("Command 'visualize' needs one argument.")));
	}

	Mode mode;
	const char* arg = get_opts.pop_arg();
	if (!toValue(arg, mode, false))
	{
	    string error = sformat(_("Unknown mode '%s'."), arg) + '\n' +
	                   possible_enum_values<Mode>();
	    SN_THROW(OptionsException(error));
	}

	TreeView::Rankdir rankdir = TreeView::Rankdir::LR;
	ParsedOpts::const_iterator opt;
	if ((opt = opts.find("rankdir")) != opts.end())
	{
	    if (!toValue(opt->second, rankdir, false))
	    {
		string error = sformat(_("Unknown rankdir '%s'."), opt->second.c_str()) +
		               '\n' + possible_enum_values<TreeView::Rankdir>();
		SN_THROW(OptionsException(error));
	    }
	}

	// Execute command
	const BackupConfig& backup_config = backup_configs.front();
	TheBigThings the_big_things(backup_config, snappers, false);

	switch (mode)
	{
	    case Mode::SOURCE_TREE:
		the_big_things.source_tree.print_graph_graphviz(rankdir);
		break;

	    case Mode::TARGET_TREE:
		the_big_things.target_tree.print_graph_graphviz(rankdir);
		break;
	};
    }

} // namespace snapper
