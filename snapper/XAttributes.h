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

	typedef xa_map_t::iterator xa_map_iter;
	typedef xa_map_t::const_iterator xa_map_citer;

	class XAttributes
	{
	private:
		xa_map_t *xamap;
	public:
		XAttributes();
		XAttributes(int);
		XAttributes(const XAttributes&);
		~XAttributes();

		XAttributes& operator=(const XAttributes&);
		bool operator==(const XAttributes&);

		void insert(const xa_pair_t&);

		friend ostream& operator<<(ostream&, const XAttributes&);
	};

	ostream& operator<<(ostream&, const xa_value_t&);
}

#endif
