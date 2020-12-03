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

#include <iostream>
#include <boost/algorithm/string.hpp>

#include <snapper/AppUtil.h>
#ifdef ENABLE_BTRFS
#include <snapper/BtrfsUtils.h>
#endif
#include <snapper/FileUtils.h>

#include "utils/HumanString.h"
#include "utils/text.h"
#include "GlobalOptions.h"
#include "proxy.h"
#include "cleanup.h"
#include "misc.h"
#include "cmd.h"


namespace snapper
{

    using namespace std;


    void
    help_cleanup()
    {
	cout << _("  Cleanup snapshots:") << '\n'
	     << _("\tsnapper cleanup <cleanup-algorithm>") << '\n'
	     << '\n'
	     << _("    Options for 'cleanup' command:") << '\n'
	     << _("\t--path <path>\t\t\tCleanup all configs affecting path.") << '\n'
	     << _("\t--free-space <space>\t\tTry to make space available.") << '\n'
	     << endl;
    }


    namespace
    {

	enum class CleanupAlgorithm { ALL, NUMBER, TIMELINE, EMPTY_PRE_POST };

	class FreeSpaceCondition
	{
	public:

	    FreeSpaceCondition(const string& path, unsigned long long free_space)
		: sdir(path), free_space(free_space)
	    {
	    }

	    bool
	    is_satisfied() const
	    {
		FreeSpaceData free_space_data;
		std::tie(free_space_data.size, free_space_data.free) = sdir.statvfs();

		bool satisfied = free_space_data.free >= free_space;

#ifdef VERBOSE_LOGGING
		cout << byte_to_humanstring(free_space_data.size, 2) << ", "
		     << byte_to_humanstring(free_space_data.free, 2) << ", "
		     << byte_to_humanstring(free_space, 2) << ", " << satisfied << '\n';
#endif

		return satisfied;
	    }

	    bool
	    is_satisfied(const ProxySnapper* snapper) const
	    {
		snapper->syncFilesystem();

		return is_satisfied();
	    }

	private:

	    SDir sdir;

	    unsigned long long free_space;

	};


	void
	run_cleanup(ProxySnapper* snapper, CleanupAlgorithm cleanup_algorithm, bool verbose)
	{
	    switch (cleanup_algorithm)
	    {
		case CleanupAlgorithm::NUMBER:
		    do_cleanup_number(snapper, verbose);
		    break;

		case CleanupAlgorithm::TIMELINE:
		    do_cleanup_timeline(snapper, verbose);
		    break;

		case CleanupAlgorithm::EMPTY_PRE_POST:
		    do_cleanup_empty_pre_post(snapper, verbose);
		    break;

		case CleanupAlgorithm::ALL:
		    do_cleanup_number(snapper, verbose);
		    do_cleanup_timeline(snapper, verbose);
		    do_cleanup_empty_pre_post(snapper, verbose);
		    break;
	    }
	}


	void
	run_cleanup(ProxySnapper* snapper, CleanupAlgorithm cleanup_algorithm, bool verbose,
		    const FreeSpaceCondition& free_space_condition)
	{
	    std::function<bool()> condition = [snapper, &free_space_condition]() {
		return free_space_condition.is_satisfied(snapper);
	    };

	    switch (cleanup_algorithm)
	    {
		case CleanupAlgorithm::NUMBER:
		    do_cleanup_number(snapper, verbose, condition);
		    break;

		case CleanupAlgorithm::TIMELINE:
		    do_cleanup_timeline(snapper, verbose, condition);
		    break;

		case CleanupAlgorithm::EMPTY_PRE_POST:
		    do_cleanup_empty_pre_post(snapper, verbose, condition);
		    break;

		case CleanupAlgorithm::ALL:
		    do_cleanup_number(snapper, verbose, condition);
		    do_cleanup_timeline(snapper, verbose, condition);
		    do_cleanup_empty_pre_post(snapper, verbose, condition);
		    break;
	    }
	}


	void
	run_cleanup(const vector<ProxySnapper*>& snappers, CleanupAlgorithm cleanup_algorithm, bool verbose)
	{
	    for (ProxySnapper* snapper : snappers)
	    {
#ifdef VERBOSE_LOGGING
		cout << "config " << snapper->configName() << '\n';
#endif

		run_cleanup(snapper, cleanup_algorithm, verbose);
	    }
	}


	void
	run_cleanup(const vector<ProxySnapper*>& snappers, CleanupAlgorithm cleanup_algorithm, bool verbose,
		    const FreeSpaceCondition& free_space_condition)
	{
	    for (ProxySnapper* snapper : snappers)
	    {
#ifdef VERBOSE_LOGGING
		cout << "config " << snapper->configName() << '\n';
#endif

		if (free_space_condition.is_satisfied())
		    break;

		try
		{
		    run_cleanup(snapper, cleanup_algorithm, verbose, free_space_condition);
		}
		catch (...)
		{
#ifdef VERBOSE_LOGGING
		    cout << "failed for " << snapper->configName() << '\n';
#endif
		}
	    }
	}

    }


    void
    command_cleanup(GlobalOptions& global_options, GetOpts& get_opts, ProxySnappers* snappers, ProxySnapper*)
    {
	const vector<Option> options = {
	    Option("path",		required_argument),
	    Option("free-space",	required_argument)
	};

	ParsedOpts opts = get_opts.parse("cleanup", options);

	CleanupAlgorithm cleanup_algorithm;
	string path;
	unsigned long long free_space = 0;

	ParsedOpts::const_iterator opt;

	if ((opt = opts.find("path")) != opts.end())
	{
	    if (!boost::starts_with(opt->second, "/"))
	    {
		string error = sformat(_("Invalid path '%s'."), opt->second.c_str());
		SN_THROW(OptionsException(error));
	    }

	    path = opt->second;
	}

	if ((opt = opts.find("free-space")) != opts.end())
	{
	    try
	    {
		free_space = humanstring_to_byte(opt->second);
	    }
	    catch (const Exception& e)
	    {
		SN_CAUGHT(e);

		string error = sformat(_("Failed to parse '%s'."), opt->second.c_str());
		SN_THROW(OptionsException(error));
	    }

	    if (free_space == 0)
	    {
		SN_THROW(OptionsException(_("Invalid free-space value.")));
	    }
	}

	if (get_opts.num_args() != 1)
	{
	    SN_THROW(OptionsException(_("Command 'cleanup' needs one arguments.")));
	}

	if (!toValue(get_opts.pop_arg(), cleanup_algorithm, false))
	{
	    string error = sformat(_("Unknown cleanup algorithm '%s'."), opt->second.c_str()) + '\n' +
		possible_enum_values<CleanupAlgorithm>();
	    SN_THROW(OptionsException(error));
	}

	if (path.empty())
	{
	    ProxySnapper* snapper = snappers->getSnapper(global_options.config());

	    if (free_space == 0)
	    {
		run_cleanup(snapper, cleanup_algorithm, global_options.verbose());
	    }
	    else
	    {
		boost::optional<FreeSpaceCondition> free_space_condition;

		try
		{
		    free_space_condition = FreeSpaceCondition(snapper->getConfig().getSubvolume(),
							      free_space);
		}
		catch (...)
		{
		    SN_THROW(CleanupException( _("Failed to query free space.")));
		}

		if (!free_space_condition->is_satisfied())
		{
		    run_cleanup(snapper, cleanup_algorithm, global_options.verbose(), *free_space_condition);

		    if (!free_space_condition->is_satisfied())
		    {
			SN_THROW(CleanupException(_("Could not make enough free space available.")));
		    }
		}
	    }
	}
	else
	{
	    vector<ProxySnapper*> affected_snappers;

#ifdef ENABLE_BTRFS

	    try
	    {
		Uuid uuid = BtrfsUtils::get_uuid(path);

		for (const map<string, ProxyConfig>::value_type& it : snappers->getConfigs())
		{
		    try
		    {
			if (BtrfsUtils::get_uuid(it.second.getSubvolume()) == uuid)
			    affected_snappers.push_back(snappers->getSnapper(it.first));
		    }
		    catch (...)
		    {
			// The config is likely not btrfs so just ignore it.
		    }
		}
	    }
	    catch (...)
	    {
		// The provided path is likely not btrfs -> simply nothing will be
		// affected.  Anyway, the exit code of snapper still tells whether enough
		// free space is available.
	    }

#endif

	    if (global_options.verbose())
	    {
		cout << "affected configs:";
		for (const ProxySnapper* snapper : affected_snappers)
		    cout << " " << snapper->configName();
		cout << '\n';
	    }

	    if (free_space == 0)
	    {
		run_cleanup(affected_snappers, cleanup_algorithm, global_options.verbose());
	    }
	    else
	    {
		boost::optional<FreeSpaceCondition> free_space_condition;

		try
		{
		    free_space_condition = FreeSpaceCondition(path, free_space);
		}
		catch (...)
		{
		    string error = sformat(_("Failed to query free space for path '%s'."), path.c_str());
		    SN_THROW(CleanupException(error));
		}

		if (!free_space_condition->is_satisfied())
		{
		    run_cleanup(affected_snappers, cleanup_algorithm, global_options.verbose(),
				*free_space_condition);

		    if (!free_space_condition->is_satisfied())
		    {
			string error = sformat(_("Could not make enough free space available for path '%s'."),
					       path.c_str());
			SN_THROW(CleanupException(error));
		    }
		}
	    }
	}
    }


    template <> struct EnumInfo<CleanupAlgorithm> { static const vector<string> names; };

    const vector<string> EnumInfo<CleanupAlgorithm>::names({ "all", "number", "timeline", "empty-pre-post" });

}
