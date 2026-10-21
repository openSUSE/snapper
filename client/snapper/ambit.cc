/*
 * Copyright (c) [2026] SUSE LLC
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

#include "ambit.h"

#include <cctype>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

#include <snapper/AppUtil.h>
#include <snapper/MntTable.h>
#include <snapper/Exception.h>

#include "../utils/text.h"



namespace snapper
{

#ifdef ENABLE_ROLLBACK

    namespace
    {

	vector<string>
	split_options(const string& options)
	{
	    vector<string> ret;
	    std::istringstream stream(options);
	    for (string option; std::getline(stream, option, ',');)
		if (!option.empty())
		    ret.push_back(option);
	    return ret;
	}


	// without the slashes around it, "/" for the top-level, "" if none
	string
	subvol_option(const vector<string>& options)
	{
	    string ret;
	    for (const string& option : options)
	    {
		if (option.compare(0, 7, "subvol=") == 0)
		{
		    const string::size_type first = option.find_first_not_of('/', 7);
		    ret = first == string::npos ? "/" :
			option.substr(first, option.find_last_not_of('/') - first + 1);
		}
	    }
	    return ret;
	}


	bool
	has_subvolid_option(const vector<string>& options)
	{
	    for (const string& option : options)
		if (option.compare(0, 9, "subvolid=") == 0)
		    return true;
	    return false;
	}

    }


    vector<string>
    rootflags_from_cmdline(const string& cmdline)
    {
	vector<string> ret;

	// double quotes group words; several rootflags= are joined as by
	// systemd's fstab-generator (the kernel itself takes only the last one)
	string word;
	bool quoted = false, in_word = false;
	auto end_word = [&]() {
	    if (in_word && word.compare(0, 10, "rootflags=") == 0)
	    {
		const vector<string> options = split_options(word.substr(10));
		ret.insert(ret.end(), options.begin(), options.end());
	    }
	    word.clear();
	    in_word = false;
	};

	for (char c : cmdline)
	{
	    if (c == '"')
	    {
		quoted = !quoted;
		in_word = true;
	    }
	    else if (!quoted && isspace(static_cast<unsigned char>(c)))
	    {
		end_word();
	    }
	    else
	    {
		word += c;
		in_word = true;
	    }
	}
	end_word();

	return ret;
    }


    namespace
    {

	// only such a name can be swapped (as in Btrfs::rollbackSubvolRename)
	bool
	is_top_level(const string& subvol)
	{
	    return !subvol.empty() && subvol.find('/') == string::npos;
	}

    }


    BootRoot
    boot_root_from(const std::optional<vector<string>>& rootflags,
		   const vector<string>& fstab_options, const string& running_subvol)
    {
	BootRoot boot;

	const string fstab_subvol = subvol_option(fstab_options);
	const bool fstab_subvolid = has_subvolid_option(fstab_options);

	// fstab only describes the normal boot for a one-off boot of a snapshot
	// or an unknown command line; otherwise the kernel mounts what the
	// command line says, the btrfs default without subvol= or subvolid=
	bool use_fstab = !rootflags;

	const string top_level_root = _("The root filesystem is mounted from the top-level "
					"subvolume (subvol=/), which a rollback cannot replace.");

	if (rootflags)
	{
	    const string cmdline_subvol = subvol_option(*rootflags);
	    const bool cmdline_subvolid = has_subvolid_option(*rootflags);

	    if (cmdline_subvol == "/")
	    {
		boot.unsupported = top_level_root;
	    }
	    else if (is_top_level(cmdline_subvol))
	    {
		// The kernel refuses a subvolid= that does not match subvol=, and
		// after the rename it names the old root. Another name in fstab
		// means a backup booted by hand, the next boot is unknown.
		if (cmdline_subvolid)
		    boot.unsupported = sformat(_("The kernel command line mounts the root "
						 "filesystem with subvol=%s and subvolid=, which "
						 "would not match after the rollback and the system "
						 "would not boot. Remove subvolid= from the kernel "
						 "command line."), cmdline_subvol.c_str());
		else if (is_top_level(fstab_subvol) && fstab_subvol != cmdline_subvol)
		    boot.unsupported = sformat(_("The system was booted with subvol=%s but fstab "
						 "mounts subvol=%s. Boot the system normally "
						 "before a rollback."), cmdline_subvol.c_str(),
					       fstab_subvol.c_str());
		else
		    boot.subvol = cmdline_subvol;
	    }
	    else if (!cmdline_subvol.empty())
	    {
		// the nested name of a booted snapshot
		use_fstab = true;
	    }
	    else if (cmdline_subvolid)
	    {
		// a one-off boot of a snapshot, or the normal root pinned by id;
		// without a name in fstab a default subvolume layout
		if (is_top_level(fstab_subvol) && running_subvol == fstab_subvol)
		    boot.unsupported = _("The kernel command line mounts the root filesystem "
					 "with subvolid=, so a rollback cannot change which "
					 "subvolume boots.");
		else
		    use_fstab = !fstab_subvol.empty() || fstab_subvolid;
	    }
	}

	if (use_fstab)
	{
	    if (fstab_subvol == "/")
		boot.unsupported = top_level_root;
	    else if (is_top_level(fstab_subvol))
		boot.subvol = fstab_subvol;
	    else if (!fstab_subvol.empty())
		boot.unsupported = sformat(_("fstab mounts the root filesystem with the nested "
					     "subvol=%s, which a rollback cannot replace."),
					   fstab_subvol.c_str());
	    else if (fstab_subvolid)
		boot.unsupported = _("The root filesystem is mounted with subvolid= in fstab, "
				     "so a rollback cannot change which subvolume boots.");
	}

	boot.stale_subvolid = !boot.subvol.empty() && fstab_subvolid;

	return boot;
    }


    namespace
    {

	// as systemd's running_in_chroot(): / is not the root of pid 1
	bool
	in_chroot()
	{
	    struct stat root, init_root;
	    return stat("/", &root) == 0 && stat("/proc/1/root", &init_root) == 0 &&
		(root.st_dev != init_root.st_dev || root.st_ino != init_root.st_ino);
	}

    }


    BootRoot
    get_boot_root(const string& root_prefix)
    {
	// both belong to the running system, not to another root prefix or chroot
	std::optional<vector<string>> rootflags;
	string running_subvol;
	if ((root_prefix.empty() || root_prefix == "/") && !in_chroot())
	{
	    std::ifstream proc_cmdline("/proc/cmdline");
	    rootflags = rootflags_from_cmdline(string(std::istreambuf_iterator<char>(proc_cmdline), {}));

	    bool found = false;
	    MtabData mtab_data;
	    if (getMtabData("/", found, mtab_data) && found)
		running_subvol = subvol_option(mtab_data.options);
	}

	return boot_root_from(rootflags, fstab_options(root_prefix, "/"), running_subvol);
    }


    Ambit
    determine_ambit(Ambit cli_ambit, const string& subvol_name, SubvolumeMode mode)
    {
	if (cli_ambit == Ambit::SUBVOL_RENAME)
	{
	    if (!is_top_level(subvol_name))
		SN_THROW(Exception(_("Ambit is 'subvol-rename' but the system does not "
				     "boot a top-level named root subvolume.")));
	    return cli_ambit;
	}

	if (cli_ambit != Ambit::AUTO)
	    return cli_ambit;

	// the btrfs default subvolume id is ignored when booting by name
	if (is_top_level(subvol_name))
	    return Ambit::SUBVOL_RENAME;

	switch (mode)
	{
	    case SubvolumeMode::READ_ONLY:  return Ambit::TRANSACTIONAL;
	    case SubvolumeMode::READ_WRITE: return Ambit::CLASSIC;
	    case SubvolumeMode::UNKNOWN:    return Ambit::AUTO;
	}

	return Ambit::AUTO;
    }


    bool
    is_set_default_ineffective(Ambit ambit, const string& subvol_name)
    {
	// a nested name was already refused in boot_root_from
	return (ambit == Ambit::CLASSIC || ambit == Ambit::TRANSACTIONAL) &&
	    is_top_level(subvol_name);
    }

#endif

}
