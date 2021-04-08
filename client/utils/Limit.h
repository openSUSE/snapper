/*
 * Copyright (c) 2021 SUSE LLC
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


#ifndef SNAPPER_LIMIT_H
#define SNAPPER_LIMIT_H


#include <string>
#include <ostream>


namespace snapper
{
    using namespace std;


    class Limit
    {
    public:

	Limit(double fraction) : type(FRACTION), fraction(fraction) {}

	/**
	 * Uses the classic locale since the class is intended for config data.
	 */
	void parse(const string& s);

	friend ostream& operator<<(ostream& str, const Limit& limit);

    protected:

	enum Type { FRACTION, ABSOLUTE };

	Type type;

	union
	{
	    double fraction;
	    unsigned long long absolute;
	};

    };


    class MaxUsedLimit : public Limit
    {
    public:

	MaxUsedLimit(double fraction) : Limit(fraction) {}

	bool is_enabled() const;

	bool is_satisfied(unsigned long long size, unsigned long long used) const;

    };


    class MinFreeLimit : public Limit
    {
    public:

	MinFreeLimit(double fraction) : Limit(fraction) {}

	bool is_enabled() const;

	bool is_satisfied(unsigned long long size, unsigned long long free) const;

    };

}

#endif
