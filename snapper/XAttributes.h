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


#include <stdint.h>

#include <map>
#include <vector>
#include <string>
#include <ostream>


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
	typedef vector<string> xa_del_vec_t;

        // iterators
	typedef xa_map_t::const_iterator xa_map_citer;
        typedef xa_mod_vec_t::const_iterator xa_mod_vec_citer;
	typedef xa_del_vec_t::const_iterator xa_del_vec_citer;

	class XAttributes
	{
	private:
            xa_map_t xamap;
        public:
            XAttributes(const string&);
            XAttributes(const XAttributes&);

            xa_map_citer cbegin() const { return xamap.begin(); }
            xa_map_citer cend() const { return xamap.end(); }

            XAttributes& operator=(const XAttributes&);
            bool operator==(const XAttributes&) const;
	};

        class XAModification
        {
        private:
	    xa_mod_vec_t create_vec;
	    xa_del_vec_t delete_vec;
	    xa_mod_vec_t replace_vec;

	    void printTo(ostream&, bool) const;
        public:
            XAModification();
            XAModification(const XAttributes&, const XAttributes&);

            bool isEmpty() const;
            bool serializeTo(const string&) const;

            unsigned int getXaCreateNum() const;
            unsigned int getXaDeleteNum() const;
            unsigned int getXaReplaceNum() const;

	    // this will generate diff report
	    void dumpDiffReport(ostream&) const;

	    // this will print out the class content
            friend ostream& operator<<(ostream&, const XAModification&);
        };

        ostream& operator<<(ostream&, const XAttributes&);
        ostream& operator<<(ostream&, const xa_value_t&);
}
#endif
