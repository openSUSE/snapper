/*
 * Copyright (c) [2010-2011] Novell, Inc.
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


#ifndef SNAPPER_ENUM_H
#define SNAPPER_ENUM_H


#include <string>
#include <vector>
#include <algorithm>
#include <exception>

#include "snapper/Log.h"
#include "snapper/Snapshot.h"


namespace snapper
{
    using std::string;
    using std::vector;


    struct OutOfRangeException : public std::exception
    {
	explicit OutOfRangeException() throw() {}
	virtual const char* what() const throw() { return "out of range"; }
    };


    template <typename EnumType> struct EnumInfo {};

    template <> struct EnumInfo<SnapshotType> { static const vector<string> names; };


    template <typename EnumType>
    const string& toString(EnumType value)
    {
	static_assert(std::is_enum<EnumType>::value, "not enum");

	const vector<string>& names = EnumInfo<EnumType>::names;

	// Comparisons must not be done with type of enum since the enum may
	// define comparison operators.
	if ((size_t)(value) >= names.size())
	    throw OutOfRangeException();

	return names[value];
    }


    template <typename EnumType>
    bool toValue(const string& str, EnumType& value, bool log_error = true)
    {
	static_assert(std::is_enum<EnumType>::value, "not enum");

	const vector<string>& names = EnumInfo<EnumType>::names;

	vector<string>::const_iterator it = find(names.begin(), names.end(), str);

	if (it == names.end())
	{
	    if (log_error)
		y2err("converting '" << str << "' to enum failed");
	    return false;
	}

	value = EnumType(it - names.begin());
	return true;
    }


    template <typename EnumType>
    EnumType toValueWithFallback(const string& str, EnumType fallback, bool log_error = true)
    {
	EnumType value;

	if (toValue(str, value, log_error))
	    return value;

	return fallback;
    }

}


#endif
