/*
 * Copyright (c) [2004-2014] Novell, Inc.
 * Copyright (c) [2016-2018] SUSE LLC
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


#include <locale>
#include <cmath>
#include <sstream>
#include <stdexcept>

#include "HumanString.h"
#include "text.h"


namespace snapper
{
    using std::string;


    /*
     * These are simplified versions of the functions in libstorage-ng.
     */


    int
    num_suffixes()
    {
	return 7;
    }


    string
    get_suffix(int i)
    {
	switch (i)
	{
	    case 0:
		// TRANSLATORS: symbol for "bytes" (best keep untranslated)
		return _("B");

	    case 1:
		// TRANSLATORS: symbol for "kibi bytes" (best keep untranslated)
		return _("KiB");

	    case 2:
		// TRANSLATORS: symbol for "mebi bytes" (best keep untranslated)
		return _("MiB");

	    case 3:
		// TRANSLATORS: symbol for "gibi bytes" (best keep untranslated)
		return _("GiB");

	    case 4:
		// TRANSLATORS: symbol for "tebi bytes" (best keep untranslated)
		return _("TiB");

	    case 5:
		// TRANSLATORS: symbol for "pebi bytes" (best keep untranslated)
		return _("PiB");

	    case 6:
		// TRANSLATORS: symbol for "exbi bytes" (best keep untranslated)
		return _("EiB");

	    default:
		throw std::logic_error("invalid prefix");
	}
    }


    bool
    is_multiple_of(unsigned long long i, unsigned long long j)
    {
	return i % j == 0;
    }


    int
    clz(unsigned long long i)
    {
	return __builtin_clzll(i);
    }


    // byte_to_humanstring uses integer arithmetic as long as possible
    // to provide exact results for those cases.


    string
    byte_to_humanstring(unsigned long long size, int precision)
    {
	const std::locale loc = std::locale();

	// Calculate the index of the suffix to use. The index increases by 1
	// when the number of leading zeros decreases by 10.

	int i = size > 0 ? (64 - (clz(size) + 1)) / 10 : 0;

	if (i == 0)
	{
	    unsigned long long v = size >> 10 * i;

	    std::ostringstream s;
	    s.imbue(loc);
	    s << v << ' ' << get_suffix(i);
	    return s.str();
	}
	else
	{
	    long double v = std::ldexp((long double)(size), - 10 * i);

	    std::ostringstream s;
	    s.imbue(loc);
	    s.setf(std::ios::fixed);
	    s.precision(precision);
	    s << v << ' ' << get_suffix(i);
	    return s.str();
	}
    }

}
