/*
 * Copyright (c) [2004-2014] Novell, Inc.
 * Copyright (c) [2016-2021] SUSE LLC
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
#include <vector>
#include <sstream>
#include <stdexcept>
#include <boost/algorithm/string.hpp>

#include "HumanString.h"
#include "text.h"

#include <snapper/Exception.h>


namespace snapper
{
    using namespace std;


    /*
     * These are simplified versions of the functions in libstorage-ng,
     * https://github.com/openSUSE/libstorage-ng/blob/master/storage/Utils/HumanString.h.
     */


    int
    num_suffixes()
    {
	return 7;
    }


    vector<string>
    get_all_suffixes(int i, bool all = true, bool classic = false)
    {
	auto _ = [classic](const char* msg) { return classic ? msg : snapper::_(msg); };

	vector<string> ret;

	switch (i)
	{
	    case 0:
		// TRANSLATORS: symbol for "bytes" (best keep untranslated)
		ret.push_back(_("B"));
		if (all)
		{
		    ret.push_back("");
		}
		break;

	    case 1:
		// TRANSLATORS: symbol for "kibi bytes" (best keep untranslated)
		ret.push_back(_("KiB"));
		if (all)
		{
		    // TRANSLATORS: symbol for "kilo bytes" (best keep untranslated)
		    ret.push_back(_("kB"));
		    // TRANSLATORS: symbol for "kilo" (best keep untranslated)
		    ret.push_back(_("k"));
		}
		break;

	    case 2:
		// TRANSLATORS: symbol for "mebi bytes" (best keep untranslated)
		ret.push_back(_("MiB"));
		if (all)
		{
		    // TRANSLATORS: symbol for "mega bytes" (best keep untranslated)
		    ret.push_back(_("MB"));
		    // TRANSLATORS: symbol for "mega" (best keep untranslated)
		    ret.push_back(_("M"));
		}
		break;

	    case 3:
		// TRANSLATORS: symbol for "gibi bytes" (best keep untranslated)
		ret.push_back(_("GiB"));
		if (all)
		{
		    // TRANSLATORS: symbol for "giga bytes" (best keep untranslated)
		    ret.push_back(_("GB"));
		    // TRANSLATORS: symbol for "giga" (best keep untranslated)
		    ret.push_back(_("G"));
		}
		break;

	    case 4:
		// TRANSLATORS: symbol for "tebi bytes" (best keep untranslated)
		ret.push_back(_("TiB"));
		if (all)
		{
		    // TRANSLATORS: symbol for "tera bytes" (best keep untranslated)
		    ret.push_back(_("TB"));
		    // TRANSLATORS: symbol for "tera" (best keep untranslated)
		    ret.push_back(_("T"));
		}
		break;

	    case 5:
		// TRANSLATORS: symbol for "pebi bytes" (best keep untranslated)
		ret.push_back(_("PiB"));
		if (all)
		{
		    // TRANSLATORS: symbol for "peta bytes" (best keep untranslated)
		    ret.push_back(_("PB"));
		    // TRANSLATORS: symbol for "peta" (best keep untranslated)
		    ret.push_back(_("P"));
		}
		break;

	    case 6:
		// TRANSLATORS: symbol for "exbi bytes" (best keep untranslated)
		ret.push_back(_("EiB"));
		if (all)
		{
		    // TRANSLATORS: symbol for "exa bytes" (best keep untranslated)
		    ret.push_back(_("EB"));
		    // TRANSLATORS: symbol for "exa" (best keep untranslated)
		    ret.push_back(_("E"));
		}
		break;
	}

	return ret;
    }


    string
    get_suffix(int i, bool classic)
    {
	return get_all_suffixes(i, false, classic).front();
    }


    namespace
    {

	int
	clz(unsigned long long i)
	{
	    return __builtin_clzll(i);
	}


	// Helper functions to parse a number as int or float, multiply
	// according to the suffix. Do all required error checks.

	pair<bool, unsigned long long>
	parse_i(const string& str, int i, const locale& loc)
	{
	    istringstream s(str);
	    s.imbue(loc);

	    unsigned long long v;
	    s >> v;

	    if (!s.eof())
		return make_pair(false, 0);

	    if (s.fail())
	    {
		if (v == std::numeric_limits<unsigned long long>::max())
		    SN_THROW(Exception("overflow"));

		return make_pair(false, 0);
	    }

	    if (v != 0 && str[0] == '-')
		SN_THROW(Exception("overflow"));

	    if (v > 0 && clz(v) < 10 * i)
		SN_THROW(Exception("overflow"));

	    v <<= 10 * i;

	    return make_pair(true, v);
	}


	pair<bool, unsigned long long>
	parse_f(const string& str, int i, const locale& loc)
	{
	    istringstream s(str);
	    s.imbue(loc);

	    long double v;
	    s >> v;

	    if (!s.eof())
		return make_pair(false, 0);

	    if (s.fail())
	    {
		if (v == std::numeric_limits<long double>::max())
		    SN_THROW(Exception("overflow"));

		return make_pair(false, 0);
	    }

	    if (v < 0.0)
		SN_THROW(Exception("overflow"));

	    v = std::round(std::ldexp(v, 10 * i));

	    if (v > std::numeric_limits<unsigned long long>::max())
		SN_THROW(Exception("overflow"));

	    return make_pair(true, v);
	}

    }


    // byte_to_humanstring uses integer arithmetic as long as possible
    // to provide exact results for those cases.


    string
    byte_to_humanstring(unsigned long long size, bool classic, int precision)
    {
	// Calculate the index of the suffix to use. The index increases by 1
	// when the number of leading zeros decreases by 10.

	static_assert(sizeof(size) == 8, "unsigned long long not 64 bit");

	const locale loc = classic ? locale::classic() : locale();

	int i = size > 0 ? (64 - (clz(size) + 1)) / 10 : 0;

	if (i == 0)
	{
	    unsigned long long v = size >> 10 * i;

	    std::ostringstream s;
	    s.imbue(loc);
	    s << v << ' ' << get_suffix(i, classic);
	    return s.str();
	}
	else
	{
	    long double v = std::ldexp((long double)(size), - 10 * i);

	    std::ostringstream s;
	    s.imbue(loc);
	    s.setf(std::ios::fixed);
	    s.precision(precision);
	    s << v << ' ' << get_suffix(i, classic);
	    return s.str();
	}
    }


    unsigned long long
    humanstring_to_byte(const string& str, bool classic)
    {
	const locale loc = classic ? locale::classic() : locale();

	const string str_trimmed = boost::trim_copy(str, loc);

	for (int i = 0; i < num_suffixes(); ++i)
	{
	    vector<string> suffixes = get_all_suffixes(i, true, classic);

	    for (const string& suffix : suffixes)
	    {
		if (boost::iends_with(str_trimmed, suffix, loc))
		{
		    string number = boost::trim_copy(str_trimmed.substr(0, str_trimmed.size() - suffix.size()), loc);

		    pair<bool, unsigned long long> v;

		    v = parse_i(number, i, loc);
		    if (v.first)
			return v.second;

		    v = parse_f(number, i, loc);
		    if (v.first)
			return v.second;
		}
	    }
	}

	SN_THROW(Exception("failed to parse size"));
	__builtin_unreachable();
    }

}
