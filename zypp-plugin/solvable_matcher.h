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

#include <iostream>
#include <vector>
#include <string>

struct SolvableMatcher {
    enum class Kind {
		     GLOB,
		     REGEX
    };
    std::string pattern;
    Kind kind;
    bool important;

    static std::ostream& log;
    SolvableMatcher(const std::string& apattern, Kind akind, bool aimportant)
	: pattern(apattern)
	, kind(akind)
	, important(aimportant)
    {}

    bool match(const std::string& solvable) const;
    static std::vector<SolvableMatcher> load_config(const std::string& cfg_filename);
};

#endif //SOLVABLE_MATCHER_H
