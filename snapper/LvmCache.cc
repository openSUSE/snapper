/*
 * Copyright (c) [2013] Red Hat, Inc.
 * Copyright (c) [2020-2023] SUSE LLC
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <vector>
#include <boost/algorithm/string.hpp>

#include "snapper/Log.h"
#include "snapper/LvmCache.h"
#include "snapper/Lvm.h"
#include "snapper/SystemCmd.h"


namespace snapper
{
    using std::make_pair;


    bool
    LvAttrs::extract_active(const string& raw)
    {
	return raw.size() > 4 && raw[4] == 'a';
    }


    bool
    LvAttrs::extract_read_only(const string& raw)
    {
	return raw.size() > 2 && (raw[1] == 'r' || raw[1] == 'R');
    }


    LvAttrs::LvAttrs(const vector<string>& raw)
	: active(raw.size() > 0 && extract_active(raw.front())),
	  read_only(raw.size() > 0 && extract_read_only(raw.front())),
	  thin(raw.size() > 1 && raw[1] == "thin")
    {
    }


    LvAttrs::LvAttrs(bool active, bool read_only, bool thin)
	: active(active), read_only(read_only), thin(thin)
    {
    }


    LogicalVolume::LogicalVolume(const VolumeGroup* vg, const string& lv_name, const LvAttrs& attrs)
	: vg(vg), lv_name(lv_name), attrs(attrs)
    {
    }


    void
    LogicalVolume::activate()
    {
	/*
	 * FIXME: There is bug in LVM causing lvs and lvchange commands
	 *	 may fail in certain situations.
	 *	 Concurrent lvs only commands are fine:
	 *	 https://bugzilla.redhat.com/show_bug.cgi?id=922568
	 *
	 *	 Upgrade lock is used to protect concurrent lvs/lvchange
	 *	 in scope of logical volume.
	 */

	if (attrs.active)
	    return;

	const LvmCapabilities* caps = LvmCapabilities::get_lvm_capabilities();

	boost::upgrade_lock<boost::shared_mutex> upg_lock(lv_mutex);

	{
	    boost::upgrade_to_unique_lock<boost::shared_mutex> unique_lock(upg_lock);

	    SystemCmd::Args cmd_args = { LVCHANGE_BIN };
	    if (!caps->get_ignoreactivationskip().empty())
		cmd_args << caps->get_ignoreactivationskip();
	    cmd_args << "--activate" << "y" << full_name();

	    SystemCmd cmd(cmd_args);
	    if (cmd.retcode() != 0)
	    {
		y2err("lvm cache: " << full_name() << " activation failed!");
		throw LvmCacheException();
	    }

	    attrs.active = true;
	}

	y2deb("lvm cache: " << full_name() << " activated");
    }


    void
    LogicalVolume::deactivate()
    {
	/*
	 * FIXME: See activate() above.
	 */

	if (!attrs.active)
	    return;

	boost::upgrade_lock<boost::shared_mutex> upg_lock(lv_mutex);

	{
	    boost::upgrade_to_unique_lock<boost::shared_mutex> unique_lock(upg_lock);

	    SystemCmd cmd({ LVCHANGE_BIN, "--activate", "n", full_name() });
	    if (cmd.retcode() != 0)
	    {
		y2err("lvm cache: " << full_name() << " deactivation failed!");
		throw LvmCacheException();
	    }

	    attrs.active = false;
	}

	y2deb("lvm cache: " << full_name() << " deactivated");
    }


    void
    LogicalVolume::update()
    {
	boost::unique_lock<boost::shared_mutex> unique_lock(lv_mutex);

	SystemCmd cmd({ LVS_BIN, "--noheadings", "--options", "lv_attr,segtype", full_name() });
	if (cmd.retcode() != 0 || cmd.get_stdout().empty())
	{
	    y2err("lvm cache: failed to get info about " << full_name());
	    throw LvmCacheException();
	}

	vector<string> args;
	const string tmp = boost::trim_copy(cmd.get_stdout().front());
	boost::split(args, tmp, boost::is_any_of(" \t\n"), boost::token_compress_on);
	if (args.size() < 1)
	    throw LvmCacheException();

	LvAttrs new_attrs(args);
	attrs = new_attrs;
    }


    bool
    LogicalVolume::is_read_only() const
    {
	boost::shared_lock<boost::shared_mutex> shared_lock(lv_mutex);

	return attrs.read_only;
    }


    void
    LogicalVolume::set_read_only(bool read_only)
    {
	/*
	 * FIXME: See activate() above.
	 */

	if (attrs.read_only == read_only)
	    return;

	boost::upgrade_lock<boost::shared_mutex> upg_lock(lv_mutex);

	{
	    boost::upgrade_to_unique_lock<boost::shared_mutex> unique_lock(upg_lock);

	    SystemCmd cmd({ LVCHANGE_BIN, "--permission", read_only ? "r" : "rw", full_name() });
	    if (cmd.retcode() != 0)
	    {
		y2err("lvm cache: " << full_name() << " setting permission failed!");
		throw LvmCacheException();
	    }

	    attrs.read_only = read_only;
	}

	y2deb("lvm cache: " << full_name() << " permission set");
    }


    bool
    LogicalVolume::thin() const
    {
	boost::shared_lock<boost::shared_mutex> shared_lock(lv_mutex);

	return attrs.thin;
    }


    string
    LogicalVolume::full_name() const
    {
	return vg->get_vg_name() + "/" + lv_name;
    }


    void
    LogicalVolume::debug(std::ostream& out) const
    {
	out << attrs;
    }


    VolumeGroup::VolumeGroup(vg_content_raw& input, const string& vg_name, const string& add_lv_name)
	: vg_name(vg_name)
    {
	for (vg_content_raw::const_iterator cit = input.begin(); cit != input.end(); ++cit)
	    if (cit->first == add_lv_name || cit->first.find("-snapshot") != string::npos)
		lv_info_map.insert(make_pair(cit->first, new LogicalVolume(this, cit->first, LvAttrs(cit->second))));
    }


    VolumeGroup::~VolumeGroup()
    {
	for (const_iterator cit = lv_info_map.begin(); cit != lv_info_map.end(); ++cit)
	    delete cit->second;
    }


    void
    VolumeGroup::activate(const string& lv_name)
    {
	boost::shared_lock<boost::shared_mutex> shared_lock(vg_mutex);

	iterator it = lv_info_map.find(lv_name);
	if (it == lv_info_map.end())
	{
	    y2err("lvm cache: " << full_name(lv_name) << " is not in cache!");
	    throw LvmCacheException();
	}

	it->second->activate();
    }


    void
    VolumeGroup::deactivate(const string& lv_name)
    {
	boost::shared_lock<boost::shared_mutex> shared_lock(vg_mutex);

	iterator it = lv_info_map.find(lv_name);
	if (it == lv_info_map.end())
	{
	    y2err("lvm cache: " << full_name(lv_name) << " is not in cache!");
	    throw LvmCacheException();
	}

	it->second->deactivate();
    }


    bool
    VolumeGroup::is_read_only(const string& lv_name)
    {
	boost::shared_lock<boost::shared_mutex> shared_lock(vg_mutex);

	iterator it = lv_info_map.find(lv_name);
	if (it == lv_info_map.end())
	{
	    y2err("lvm cache: " << full_name(lv_name) << " is not in cache!");
	    throw LvmCacheException();
	}

	return it->second->is_read_only();
    }


    void
    VolumeGroup::set_read_only(const string& lv_name, bool read_only)
    {
	boost::shared_lock<boost::shared_mutex> shared_lock(vg_mutex);

	iterator it = lv_info_map.find(lv_name);
	if (it == lv_info_map.end())
	{
	    y2err("lvm cache: " << full_name(lv_name) << " is not in cache!");
	    throw LvmCacheException();
	}

	it->second->set_read_only(read_only);
    }


    bool
    VolumeGroup::contains(const std::string& lv_name) const
    {
	boost::shared_lock<boost::shared_mutex> shared_lock(vg_mutex);

	return lv_info_map.find(lv_name) != lv_info_map.end();
    }


    bool
    VolumeGroup::contains_thin(const string& lv_name) const
    {
	boost::shared_lock<boost::shared_mutex> shared_lock(vg_mutex);

	const_iterator cit = lv_info_map.find(lv_name);

	return cit != lv_info_map.end() && cit->second->thin();
    }


    void
    VolumeGroup::create_snapshot(const string& lv_origin_name, const string& lv_snapshot_name,
				 bool read_only)
    {
	const LvmCapabilities* caps = LvmCapabilities::get_lvm_capabilities();

	boost::upgrade_lock<boost::shared_mutex> upg_lock(vg_mutex);

	if (lv_info_map.find(lv_snapshot_name) != lv_info_map.end())
	{
	    y2err("lvm cache: " << full_name(lv_snapshot_name) << " already in cache!");
	    throw LvmCacheException();
	}

	boost::upgrade_to_unique_lock<boost::shared_mutex> unique_lock(upg_lock);

	SystemCmd cmd({ LVCREATE_BIN, "--permission", read_only ? "r" : "rw", "--snapshot",
		"--name", lv_snapshot_name, full_name(lv_origin_name) });

	if (cmd.retcode() != 0)
	    throw LvmCacheException();

	LvAttrs attrs(caps->get_ignoreactivationskip().empty(), read_only, true);
	lv_info_map.insert(make_pair(lv_snapshot_name, new LogicalVolume(this, lv_snapshot_name, attrs)));
    }


    void
    VolumeGroup::add_or_update(const string& lv_name)
    {
	boost::upgrade_lock<boost::shared_mutex> upg_lock(vg_mutex);

	iterator it = lv_info_map.find(lv_name);
	if (it != lv_info_map.end())
	{
	    // FIXME: upgrade lock is too strict here. Should be shared_lock only
	    it->second->update();
	}
	else
	{
	    SystemCmd cmd({ LVS_BIN, "--noheadings", "--options", "lv_attr,segtype", full_name(lv_name) });
	    if (cmd.retcode() != 0 || cmd.get_stdout().empty())
	    {
		y2err("lvm cache: failed to get info about " << full_name(lv_name));
		throw LvmCacheException();
	    }

	    vector<string> args;
	    const string tmp = boost::trim_copy(cmd.get_stdout().front());
	    boost::split(args, tmp, boost::is_any_of(" \t\n"), boost::token_compress_on);
	    if (args.size() < 1)
		throw LvmCacheException();

	    LogicalVolume* p_lv = new LogicalVolume(this, lv_name, LvAttrs(args));

	    boost::upgrade_to_unique_lock<boost::shared_mutex> unique_lock(upg_lock);
	    lv_info_map.insert(make_pair(lv_name, p_lv));
	}
    }


    void
    VolumeGroup::remove_lv(const string& lv_name)
    {
	boost::upgrade_lock<boost::shared_mutex> upg_lock(vg_mutex);

	iterator cit = lv_info_map.find(lv_name);
	if (cit == lv_info_map.end())
	{
	    y2err("lvm cache: " << full_name(lv_name) << " is not in cache!");
	    throw LvmCacheException();
	}

	// wait for all individual lv cache operations under shared vg lock to finish
	boost::upgrade_to_unique_lock<boost::shared_mutex> unique_lock(upg_lock);

	SystemCmd cmd({ LVREMOVE_BIN, "--force", full_name(lv_name) });
	if (cmd.retcode() != 0)
	    throw LvmCacheException();

	delete cit->second;
	lv_info_map.erase(cit);
    }


    string
    VolumeGroup::full_name(const string& lv_name) const
    {
	return vg_name + "/" + lv_name;
    }


    void
    VolumeGroup::debug(std::ostream& out) const
    {
	// do not allow any modifications in a whole VG
	boost::unique_lock<boost::shared_mutex> unique_lock(vg_mutex);

	for (const_iterator cit = lv_info_map.begin(); cit != lv_info_map.end(); ++cit)
	    out << "\tLV:'" << cit->first << "':" << std::endl << "\t\t" << cit->second;
    }


    LvmCache*
    LvmCache::get_lvm_cache()
    {
	static LvmCache cache;
	return &cache;
    }


    LvmCache::~LvmCache()
    {
	for (const_iterator cit = vgroups.begin(); cit != vgroups.end(); ++cit)
	    delete cit->second;
    }


    void
    LvmCache::activate(const string& vg_name, const string& lv_name) const
    {
	const_iterator cit = vgroups.find(vg_name);
	if (cit == vgroups.end())
	{
	    y2err("lvm cache: VG " << vg_name << " is not in cache!");
	    throw LvmCacheException();
	}

	cit->second->activate(lv_name);
    }


    void
    LvmCache::deactivate(const string& vg_name, const string& lv_name) const
    {
	const_iterator cit = vgroups.find(vg_name);
	if (cit == vgroups.end())
	{
	    y2err("lvm cache: VG " << vg_name << " is not in cache!");
	    throw LvmCacheException();
	}

	cit->second->deactivate(lv_name);
    }


    bool
    LvmCache::is_read_only(const string& vg_name, const string& lv_name) const
    {
	const_iterator cit = vgroups.find(vg_name);
	if (cit == vgroups.end())
	{
	    y2err("lvm cache: VG " << vg_name << " is not in cache!");
	    throw LvmCacheException();
	}

	return cit->second->is_read_only(lv_name);
    }


    void
    LvmCache::set_read_only(const string& vg_name, const string& lv_name, bool read_only)
    {
	const_iterator cit = vgroups.find(vg_name);
	if (cit == vgroups.end())
	{
	    y2err("lvm cache: VG " << vg_name << " is not in cache!");
	    throw LvmCacheException();
	}

	cit->second->set_read_only(lv_name, read_only);
    }


    bool
    LvmCache::contains(const string& vg_name, const string& lv_name) const
    {
	const_iterator cit = vgroups.find(vg_name);

	return cit != vgroups.end() && cit->second->contains(lv_name);
    }


    bool
    LvmCache::contains_thin(const string& vg_name, const string& lv_name) const
    {
	const_iterator cit = vgroups.find(vg_name);

	return cit != vgroups.end() && cit->second->contains_thin(lv_name);
    }


    void
    LvmCache::add_or_update(const string& vg_name, const string& lv_name)
    {
	const_iterator cit = vgroups.find(vg_name);
	if (cit == vgroups.end())
	{
	    add_vg(vg_name, lv_name);
	    y2deb("lvm cache: added new vg: " << vg_name << ", including lv: " << lv_name);
	}
	else
	{
	    cit->second->add_or_update(lv_name);
	    y2deb("lvm cache: updated lv details for " << lv_name);
	}
    }


    void
    LvmCache::create_snapshot(const string& vg_name, const string& lv_origin_name,
			      const string& lv_snapshot_name, bool read_only)
    {
	const_iterator cit = vgroups.find(vg_name);
	if (cit == vgroups.end())
	{
	    y2err("VG " << vg_name << " is not in cache!");
	    throw LvmCacheException();
	}

	cit->second->create_snapshot(lv_origin_name, lv_snapshot_name, read_only);

	y2deb("lvm cache: created new snapshot: " << lv_snapshot_name << " in vg: " << vg_name);
    }


    void
    LvmCache::add_vg(const string& vg_name, const string& include_lv_name)
    {
	SystemCmd cmd({ LVS_BIN, "--noheadings", "--options", "lv_name,lv_attr,segtype", vg_name });
	if (cmd.retcode() != 0)
	{
	    y2err("lvm cache: failed to get info about VG " << vg_name);
	    throw LvmCacheException();
	}

	vg_content_raw new_content;

	for (vector<string>::const_iterator cit = cmd.get_stdout().begin(); cit != cmd.get_stdout().end(); ++cit)
	{
	    vector<string> args;

	    const string tmp = boost::trim_copy(*cit);
	    boost::split(args, tmp, boost::is_any_of(" \t\n"), boost::token_compress_on);
	    if (args.size() < 1)
		throw LvmCacheException();

	    new_content.insert(make_pair(args.front(), vector<string>(args.begin() + 1, args.end())));
	}

	VolumeGroup *p_vg = new VolumeGroup(new_content, vg_name, include_lv_name);

	vgroups.insert(std::make_pair(vg_name, p_vg));
    }


    void
    LvmCache::delete_snapshot(const string& vg_name, const string& lv_name) const
    {
	const_iterator cit = vgroups.find(vg_name);
	if (cit == vgroups.end())
	{
	    y2err("lvm cache: VG " << vg_name << " not in cache!");
	    throw LvmCacheException();
	}

	cit->second->remove_lv(lv_name);

	y2deb("lvm cache: removed " << vg_name << "/" << lv_name);
    }


    std::ostream&
    operator<<(std::ostream& out, const LvmCache* cache)
    {
	out << "LvmCache:" << std::endl;

	for (LvmCache::const_iterator cit = cache->vgroups.begin(); cit != cache->vgroups.end(); ++cit)
	    out << "Volume Group:'" << cit->first << "':" << std::endl << cit->second;

	return out;
    }


    std::ostream&
    operator<<(std::ostream& out, const VolumeGroup* vg)
    {
	vg->debug(out);

	return out;
    }


    std::ostream&
    operator<<(std::ostream& out, const LogicalVolume* lv)
    {
	lv->debug(out);

	return out;
    }


    std::ostream&
    operator<<(std::ostream& out, const LvAttrs& lv_attrs)
    {
	out << "active:" << (lv_attrs.active ? "true" : "false")
	    << ", read-only:" << (lv_attrs.read_only ? "true" : "false")
	    << ", thin:" << (lv_attrs.thin ? "true" : "false") << '\n';

	return out;
    }

}
