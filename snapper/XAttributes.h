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

#ifndef SNAPPER_XATTRIBUTES_H
#define SNAPPER_XATTRIBUTES_H

#include <map>
#include <vector>
#include <string>
#include <iostream>

#include <stdint.h>

namespace snapper
{
	using std::map;
	using std::string;
	using std::pair;
	using std::ostream;
	using std::vector;

	typedef vector<uint8_t> xa_value_t;
	typedef map<string, xa_value_t> xa_map_t;
	typedef pair<string, xa_value_t> xa_pair_t;
        typedef pair<uint8_t, string> xa_cmp_pair_t;
        typedef pair<bool, xa_value_t> xa_find_pair_t;
        typedef vector<string> xa_name_vec_t;

        // this is ordered on purpose!
        // we can possibly avoid allocating new fs block if xattrs fits
        // into 100 bytes (ext2,3,4)
        // so, first remove/change and create later
        enum XaCompareFlags {
            XA_DELETE = 0,
            XA_REPLACE,
            XA_CREATE
        };
        // pair<mode, xa_name_vec_t>
        //i.e:
        // mode=create/delete/replace, names="acl, selinux"
        // create - whole new XA
        // delete - remove XA
        // replace - change in xa_value
        typedef map<uint8_t, xa_name_vec_t> xa_change_t;

        // iterators
	typedef xa_map_t::iterator xa_map_iter;
	typedef xa_map_t::const_iterator xa_map_citer;
        typedef xa_change_t::const_iterator xa_change_citer;
        typedef xa_name_vec_t::const_iterator xa_name_vec_citer;

	class XAttributes
	{
	private:
            xa_map_t *xamap;
            xa_change_t *xachmap;
	public:
		XAttributes();
		XAttributes(int);
		XAttributes(const XAttributes&);
		~XAttributes();

                xa_find_pair_t find(const string&) const;
                void insert(const xa_pair_t&);
                void generateXaComparison(const XAttributes&);
                bool serializeTo(int);

		XAttributes& operator=(const XAttributes&);
		bool operator==(const XAttributes&) const;

		friend ostream& operator<<(ostream&, const XAttributes&);
	};

        ostream& operator<<(ostream&, const xa_value_t&);
        ostream& operator<<(ostream&, const xa_change_t&);
}

#endif
