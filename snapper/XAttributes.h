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
#include <functional>

#include <stdint.h>
#include <sys/types.h>

namespace snapper
{
	using std::map;
	using std::string;
	using std::pair;
	using std::ostream;
	using std::vector;

        class XAModification;
        class XAttributes;

	typedef vector<uint8_t> xa_value_t;
	typedef map<string, xa_value_t> xa_map_t;
	typedef pair<string, xa_value_t> xa_pair_t;
        typedef vector<xa_pair_t> xa_mod_vec_t;

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
        typedef map<uint8_t, xa_mod_vec_t> xa_modification_t;

        // iterators
	typedef xa_map_t::const_iterator xa_map_citer;
        typedef xa_modification_t::const_iterator xa_mod_citer;
        typedef xa_mod_vec_t::const_iterator xa_mod_vec_citer;

        typedef std::function<int(const string&, const xa_value_t&, int)> xattr_set_cb_t;
        typedef std::function<int(const string&)> xattr_rm_cb_t;

	class XAttributes
	{
	private:
            xa_map_t xamap;
            mode_t type;
        public:
            /* constructor for all but link types */
            XAttributes(int);
            /* constructor for links */
            XAttributes(const string&);
            XAttributes(const XAttributes&);

            xa_map_citer cbegin() const { return xamap.begin(); }
            xa_map_citer cend() const { return xamap.end(); }
            mode_t getType() const { return type; }

            XAttributes& operator=(const XAttributes&);
            bool operator==(const XAttributes&) const;
	};

        class XAModification
        {
        private:
            xa_modification_t xamodmap;
            const xa_mod_vec_t& operator[](const uint8_t) const;

            struct LinkSetHelper;
            struct LinkRmHelper;
            struct FileSetHelper;
            struct FileRmHelper;

            bool serializeTo(xattr_set_cb_t, xattr_rm_cb_t) const;
        public:
            XAModification();
            XAModification(const XAttributes&, const XAttributes&);

            bool isEmpty() const;
            bool serializeToFd(int) const;
            bool serializeToLink(const string&) const;
            xa_mod_citer cbegin() const { return xamodmap.begin(); };
            xa_mod_citer cend() const { return xamodmap.end(); };

            friend ostream& operator<<(ostream&, const XAModification&);
        };

        ostream& operator<<(ostream&, const XAttributes&);
        ostream& operator<<(ostream&, const xa_value_t&);
        ostream& operator<<(ostream&, const xa_modification_t&);
}
#endif
