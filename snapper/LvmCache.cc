/*
 * Copyright (c) [2013] Red Hat, Inc.
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
 *
 */

#include "config.h"

#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/thread/lock_options.hpp>
#include <boost/thread/shared_lock_guard.hpp>

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
	return (raw.size() > 4 && raw[4] == 'a');
    }


    bool
    LvAttrs::extract_readonly(const string& raw)
    {
	return (raw.size() > 1 && raw[1] == 'r');
    }


    LvAttrs::LvAttrs(const vector<string>& raw)
	: active(raw.size() > 0 && extract_active(raw.front())),
	readonly(raw.size() > 0 && extract_readonly(raw.front())),
	thin(raw.size() > 1 && raw[1] == "thin"),
	pool(raw.size() > 2 ? raw[2] : "")
    {
    }


    LvAttrs::LvAttrs(bool active, bool readonly, bool thin, string pool)
	: active(active), readonly(readonly), thin(thin), pool(pool)
    {
    }


    LogicalVolume::LogicalVolume(const VolumeGroup* vg, const string& lv_name)
	: vg(vg), lv_name(lv_name), caps(LvmCapabilities::get_lvm_capabilities()),
	attrs(caps->get_ignoreactivationskip().empty(), true, true, "")
    {
    }


    LogicalVolume::LogicalVolume(const VolumeGroup* vg, const string& lv_name, const LvAttrs& attrs)
	: vg(vg), lv_name(lv_name), caps(LvmCapabilities::get_lvm_capabilities()), attrs(attrs)
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

	boost::upgrade_lock<boost::upgrade_mutex> upg_lock(lv_mutex);

	if (!attrs.active)
	{
	    boost::upgrade_to_unique_lock<boost::upgrade_mutex> unique_lock(upg_lock);

	    SystemCmd cmd(LVCHANGEBIN + caps->get_ignoreactivationskip() + " -ay " + quote(vg->get_vg_name() + "/" + lv_name));
	    if (cmd.retcode() != 0)
	    {
		y2err("Couldn't activate snapshot " << vg->get_vg_name() << "/" << lv_name);
		throw LvmActivationException();
	    }

	    attrs.active = true;
	}
    }


    void
    LogicalVolume::deactivate()
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

	boost::upgrade_lock<boost::upgrade_mutex> upg_lock(lv_mutex);

	if (attrs.active)
	{
	    boost::upgrade_to_unique_lock<boost::upgrade_mutex> unique_lock(upg_lock);

	    SystemCmd cmd(LVCHANGEBIN " -an " + quote(vg->get_vg_name() + "/" + lv_name));
	    if (cmd.retcode() != 0)
	    {
		y2err("Couldn't activate snapshot " << vg->get_vg_name() << "/" << lv_name);
		throw LvmDeactivatationException();
	    }

	    attrs.active = false;
	}
    }

    // vg shared lock
    void
    LogicalVolume::update()
    {
	boost::shared_lock<boost::upgrade_mutex> shared_lock(lv_mutex);
	SystemCmd cmd(LVSBIN " --noheadings -o lv_attr,segtype,pool_lv " + quote(vg->get_vg_name() + "/" + lv_name));
	shared_lock.unlock();

	if (cmd.retcode() != 0 || cmd.numLines() < 1)
	{
	    y2err("lvm cache failed to get info about " << vg->get_vg_name() << "/" << lv_name);
	    throw LvmCacheException();
	}


	vector<string> args;
	const string tmp = boost::trim_copy(cmd.getLine(0));
	boost::split(args, tmp, boost::is_any_of(" \t\n"), boost::token_compress_on);
	if (args.size() < 1)
	    throw LvmCacheException();

	LvAttrs new_attrs(args);

	boost::unique_lock<boost::upgrade_mutex> unique_lock(lv_mutex);

	attrs = new_attrs;
    }


    bool
    LogicalVolume::readonly()
    {
	boost::shared_lock<boost::upgrade_mutex> shared_lock(lv_mutex);

	return attrs.readonly;
    }


    bool
    LogicalVolume::thin()
    {
	boost::shared_lock<boost::upgrade_mutex> shared_lock(lv_mutex);

	return attrs.thin;
    }


    void
    LogicalVolume::debug(std::ostream& out) const
    {
	out << attrs;
    }


    VolumeGroup::VolumeGroup(vg_content_raw& input, const string& vg_name, const string& add_lv_name)
	: vg_name(vg_name)
    {
	for (vg_content_raw::const_iterator cit = input.begin(); cit != input.end(); cit++)
	    if (cit->first == add_lv_name || cit->first.find("-snapshot") != string::npos)
		lv_info_map.insert(make_pair(cit->first, new LogicalVolume(this, cit->first, LvAttrs(cit->second))));
    }


    VolumeGroup::~VolumeGroup()
    {
	for (const_iterator cit = lv_info_map.begin(); cit != lv_info_map.end(); cit++)
	    delete cit->second;
    }


    void
    VolumeGroup::activate(const string& lv_name)
    {
	boost::shared_lock<boost::upgrade_mutex> shared_lock(vg_mutex);

	iterator it = lv_info_map.find(lv_name);
	if (it == lv_info_map.end())
	{
	    y2err(vg_name << "/" << lv_name << " is not in cache!");
	    throw LvmCacheException();
	}

	it->second->activate();
    }


    void
    VolumeGroup::deactivate(const string& lv_name)
    {
	boost::shared_lock<boost::upgrade_mutex> shared_lock(vg_mutex);

	iterator it = lv_info_map.find(lv_name);
	if (it == lv_info_map.end())
	{
	    y2err(vg_name << "/" << lv_name << " is not in cache!");
	    throw LvmCacheException();
	}

	it->second->deactivate();
    }


    bool
    VolumeGroup::contains(const std::string& lv_name) const
    {
	boost::shared_lock<boost::upgrade_mutex> shared_lock(vg_mutex);

	return lv_info_map.find(lv_name) != lv_info_map.end();
    }


    bool
    VolumeGroup::contains_thin(const string& lv_name) const
    {
	boost::shared_lock<boost::upgrade_mutex> shared_lock(vg_mutex);

	const_iterator cit = lv_info_map.find(lv_name);

	return cit != lv_info_map.end() && cit->second->thin();
    }


    bool
    VolumeGroup::read_only(const string& lv_name) const
    {
	boost::shared_lock<boost::upgrade_mutex> shared_lock(vg_mutex);

	const_iterator cit = lv_info_map.find(lv_name);

	return cit != lv_info_map.end() && cit->second->readonly();
    }


    void
    VolumeGroup::add(const string& lv_name)
    {
	boost::unique_lock<boost::upgrade_mutex> lock(vg_mutex);

	if (!lv_info_map.insert(make_pair(lv_name, new LogicalVolume(this, lv_name))).second)
	    throw LvmCacheException();
    }


    void
    VolumeGroup::add_or_update(const string& lv_name)
    {
	boost::upgrade_lock<boost::upgrade_mutex> upg_lock(vg_mutex);

	iterator it = lv_info_map.find(lv_name);
	if (it != lv_info_map.end())
	{
	    // FIXME: Is there a better way to downgrade upgrade lock?
	    upg_lock.release()->unlock_upgrade_and_lock_shared();
	    boost::shared_lock_guard<boost::upgrade_mutex> shared_guard(vg_mutex, boost::adopt_lock_t());

	    it->second->update();
	}
	else
	{
	    SystemCmd cmd(LVSBIN " --noheadings -o lv_attr,segtype,pool_lv " + quote(vg_name + "/" + lv_name));
	    if (cmd.retcode() != 0 || cmd.numLines() < 1)
	    {
		y2err("lvm cache failed to get info about " << vg_name << "/" << lv_name);
		throw LvmCacheException();
	    }

	    vector<string> args;
	    const string tmp = boost::trim_copy(cmd.getLine(0));
	    boost::split(args, tmp, boost::is_any_of(" \t\n"), boost::token_compress_on);
	    if (args.size() < 1)
		throw LvmCacheException();

	    LogicalVolume* p_lv = new LogicalVolume(this, lv_name, LvAttrs(args));

	    boost::upgrade_to_unique_lock<boost::upgrade_mutex> unique_lock(upg_lock);
	    lv_info_map.insert(make_pair(lv_name, p_lv));
	}
    }


    void
    VolumeGroup::remove(const string& lv_name)
    {
	boost::unique_lock<boost::upgrade_mutex> unique_lock(vg_mutex);

	const_iterator cit = lv_info_map.find(lv_name);

	if (cit == lv_info_map.end())
	    throw LvmCacheException();

	delete cit->second;
	lv_info_map.erase(cit);
    }


    void
    VolumeGroup::rename(const string& old_name, const string& new_name)
    {
	boost::upgrade_lock<boost::upgrade_mutex> upg_lock(vg_mutex);

	const_iterator cit = lv_info_map.find(old_name);

	if (cit == lv_info_map.end() || lv_info_map.find(new_name) != lv_info_map.end())
	    throw LvmCacheException();

	boost::upgrade_to_unique_lock<boost::upgrade_mutex> unique_lock(upg_lock);

	lv_info_map.insert(make_pair(new_name, new LogicalVolume(this, new_name, cit->second->attrs)));

	delete cit->second;
	lv_info_map.erase(cit);
    }


    void
    VolumeGroup::debug(std::ostream& out) const
    {
	// do not allow any modifications in a whole VG
	boost::unique_lock<boost::upgrade_mutex> unique_lock(vg_mutex);

	for (const_iterator cit = lv_info_map.begin(); cit != lv_info_map.end(); cit++)
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
	for (const_iterator cit = vgroups.begin(); cit != vgroups.end(); cit++)
	   delete cit->second;
    }


    void
    LvmCache::activate(const string& vg_name, const string& lv_name) const
    {
	const_iterator cit = vgroups.find(vg_name);

	if (cit == vgroups.end())
	{
	    y2err("VG " << vg_name << " is not in cache!");
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
	    y2err("VG " << vg_name << " is not in cache!");
	    throw LvmCacheException();
	}

	cit->second->deactivate(lv_name);
    }


    bool
    LvmCache::read_only(const string& vg_name, const string& lv_name) const
    {
	const_iterator cit = vgroups.find(vg_name);

	return cit != vgroups.end() && cit->second->read_only(lv_name);
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
    LvmCache::add(const string& vg_name, const string& lv_name) const
    {
	const_iterator cit = vgroups.find(vg_name);
	if (cit == vgroups.end())
	{
	    y2err("VG " << vg_name << " is not in cache!");
	    throw LvmCacheException();
	}

	try
	{
	    cit->second->add(lv_name);
	}
	catch(const LvmCacheException& e)
	{
	    y2err(vg_name << "/" << lv_name << " already in cache!");
	    throw;
	}

	y2deb("lvm cache: added new lv: " << lv_name << " in vg: " << vg_name);
    }


    void
    LvmCache::add_vg(const string& vg_name, const string& include_lv_name)
    {
	SystemCmd cmd(LVSBIN " --noheadings -o lv_name,lv_attr,segtype,pool_lv " + quote(vg_name));
	if (cmd.retcode() != 0)
	{
	    y2err("lvm cache failed to get info about VG: " << vg_name);
	    throw LvmCacheException();
	}

	vg_content_raw new_content;

	for (vector<string>::const_iterator cit = cmd.stdout().begin(); cit != cmd.stdout().end(); cit++)
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
    LvmCache::remove(const string& vg_name, const string& lv_name) const
    {
	const_iterator cit = vgroups.find(vg_name);

	if (cit != vgroups.end())
	    cit->second->remove(lv_name);

	y2deb("lvm cache: removed lv " << lv_name << " from vg " << vg_name);
    }


    void
    LvmCache::rename(const string& vg_name, const string& old_name, const string& new_name) const
    {
	const_iterator cit = vgroups.find(vg_name);

	if (cit == vgroups.end())
	{
	    y2err("VG " << vg_name << " is not in cache!");
	    throw LvmCacheException();
	}

	try
	{
	    cit->second->rename(old_name, new_name);
	}
	catch (const LvmCacheException& e)
	{
	    y2err("lvm cache failed to rename " << vg_name << "/" << old_name << " ->  " << vg_name << "/" << new_name);
	    throw;
	}

	y2deb("lvm cache: in vg " << vg_name << " " << old_name << " was renamed to " << new_name);
    }


    std::ostream&
    operator<<(std::ostream& out, const LvmCache* cache)
    {
	out << "LvmCache:" << std::endl;
	for (LvmCache::const_iterator cit = cache->vgroups.begin(); cit != cache->vgroups.end(); cit++)
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
    operator<<(std::ostream& out, const LvAttrs& a)
    {
	out << "active='" << (a.active ? "true" : "false") << "',readonly='"
	    << (a.readonly ? "true" : "false") << "',thin='"
	    << (a.thin ? "true" : "false") << ",pool='" << a.pool << "'"
	    << std::endl;

	return out;
    }
}
