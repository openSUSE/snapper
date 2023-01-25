/*
 * Copyright (c) 2019 SUSE LLC
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
 * with this program; if not, contact SUSE LLC.
 *
 * To contact SUSE about this file by physical or electronic mail, you may
 * find current contact information at www.suse.com.
 */

#ifndef SOLVABLE_MATCHER_H
#define SOLVABLE_MATCHER_H


#include <vector>
#include <string>


class SolvableMatcher
{
public:

    enum class Kind { GLOB, REGEX };

    SolvableMatcher(const std::string& pattern, Kind kind, bool important)
	: pattern(pattern), kind(kind), important(important)
    {}

    bool is_important() const { return important; }

    bool match(const std::string& solvable) const;

    static std::vector<SolvableMatcher> load_config(const std::string& cfg_filename);

private:

    const std::string pattern;
    const Kind kind;
    const bool important;

};

#endif
