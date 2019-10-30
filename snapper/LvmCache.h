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

#ifndef SNAPPER_LVM_CACHE_H
#define SNAPPER_LVM_CACHE_H

#include <map>
#include <set>
#include <string>
#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/thread/shared_mutex.hpp>



namespace snapper
{
    using std::map;
    using std::set;
    using std::string;
    using std::vector;

    class LvmCapabilities;
    class VolumeGroup;

    typedef map<string, vector<string>> vg_content_raw;

    struct LvmCacheException : public std::exception
    {
	explicit LvmCacheException() throw() {}
	virtual const char* what() const throw() override { return "lvm cache exception"; }
    };

    struct LvAttrs
    {
	static bool extract_active(const string& raw);
	static bool extract_readonly(const string& raw);

	LvAttrs(const vector<string>& raw);
	LvAttrs(bool active, bool thin);

	bool active;
	bool thin;
    };


    class LogicalVolume : boost::noncopyable
    {
    public:
	friend class VolumeGroup;

	// default constructor
	LogicalVolume(const VolumeGroup* vg, const string& lv_name);
	LogicalVolume(const VolumeGroup* vg, const string& lv_name, const LvAttrs& attrs);

	void activate(); // upg -> excl. lock
	void deactivate(); // upg -> excl. lock

	void update(); // shared, unique_lock

	bool thin(); // shared

	friend std::ostream& operator<<(std::ostream& out, const LogicalVolume* cache);

    private:
	void debug(std::ostream& out) const;

	const VolumeGroup* vg;
	const string lv_name;
	const LvmCapabilities* caps;

	LvAttrs attrs;

	mutable boost::shared_mutex lv_mutex;
    };


    class VolumeGroup : boost::noncopyable
    {
    public:

	// store pointer: LvInfo can be modified
	typedef map<string, LogicalVolume*> vg_content_t;
	typedef vg_content_t::iterator iterator;
	typedef vg_content_t::const_iterator const_iterator;

	VolumeGroup(vg_content_raw& input, const string& vg_name, const string& add_lv_name);
	~VolumeGroup();

	string get_vg_name() const { return vg_name; }

	void activate(const string& lv_name); // shared lock
	void deactivate(const string& lv_name); // shared lock

	bool contains(const string& lv_name) const; // shared lock
	bool contains_thin(const string& lv_name) const; // shared lock

	void create_snapshot(const string& lv_origin_name, const string& lv_snapshot_name); // upg lock -> excl
	void add_or_update(const string& lv_name); // upg lock -> excl

	void remove_lv(const string& lv_name); // upg lock -> excl

	friend std::ostream& operator<<(std::ostream& out, const VolumeGroup* vg);

    private:
	void debug(std::ostream& out) const;

	const string vg_name;

	mutable boost::shared_mutex vg_mutex;

	vg_content_t lv_info_map;
    };


    class LvmCache : public boost::noncopyable
    {
    public:
	static LvmCache* get_lvm_cache();

	~LvmCache();

	// storing pointers in case we will need locking (mutex is noncopyable)
	typedef map<string, VolumeGroup*>::const_iterator const_iterator;
	typedef map<string, VolumeGroup*>::iterator iterator;

	void activate(const string& vg_name, const string& lv_name) const;
	void deactivate(const string& vg_name, const string& lv_name) const;

	bool contains(const string& vg_name, const string& lv_name) const;
	bool contains_thin(const string& vg_name, const string& lv_name) const;

	// create snapper owned snapshot
	void create_snapshot(const string& vg_name, const string&lv_origin_name, const string& lv_snapshot_name);
	// used to actualise info about origin volume
	void add_or_update(const string& vg_name, const string& lv_name);

	// remove snapshot owned by snapper
	void delete_snapshot(const string& vg_name, const string& lv_name) const;

	friend std::ostream& operator<<(std::ostream& out, const LvmCache* cache);
    private:
	LvmCache() {}

	// load all snapper's snapshots in vg_name VG and also add 'add_lv_name' LV
	void add_vg(const string& vg_name, const string& include_lv_name);

	map<string, VolumeGroup*> vgroups;
    };

    std::ostream& operator<<(std::ostream& out, const LvAttrs& a);
}
#endif // SNAPPER_LVM_CACHE_H
