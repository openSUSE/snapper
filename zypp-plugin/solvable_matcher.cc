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

#include "solvable_matcher.h"

#include <iostream>
#include <vector>
#include <string>
using namespace std;

// fnmatch
#include <fnmatch.h>
#include "snapper/Regex.h"
#include "snapper/XmlFile.h"
using namespace snapper;

std::ostream& SolvableMatcher::log = std::cerr;

bool SolvableMatcher::match(const string& solvable) const {
    log << "DEBUG:"
	<< "match? " << solvable
	<< " by " << ((kind == Kind::GLOB)? "GLOB '": "REGEX '")
	<< pattern << '\'' << endl;
    bool res;
    if (kind == Kind::GLOB) {
	static const int flags = 0;
	res = fnmatch(pattern.c_str(), solvable.c_str(), flags) == 0;
    }
    else {
	// POSIX Extended Regular Expression Syntax
	// The original Python implementation allows "foo" to match "foo-devel"
	snapper::Regex rx_pattern("^" + pattern, REG_EXTENDED | REG_NOSUB);
	res = rx_pattern.match(solvable);
    }
    log << "DEBUG:" << "-> " << res << endl;
    return res;
}

vector<SolvableMatcher> SolvableMatcher::load_config(const string& cfg_filename) {
    vector<SolvableMatcher> result;

    log << "DEBUG:" << "parsing " << cfg_filename << endl;
    XmlFile config(cfg_filename);
    const xmlNode* root = config.getRootElement();
    const xmlNode* solvables_n = getChildNode(root, "solvables");
    const list<const xmlNode*> solvables_l = getChildNodes(solvables_n, "solvable");
    for (auto node: solvables_l) {
	string pattern;
	Kind kind;
	bool important = false;

	getAttributeValue(node, "important", important);
	string kind_s;
	getAttributeValue(node, "match", kind_s);
	getValue(node, pattern);
	if (kind_s == "w") { // w for wildcard
	    kind = Kind::GLOB;
	}
	else if (kind_s == "re") { // Regular Expression
	    kind = Kind::REGEX;
	}
	else {
	    log << "ERROR:" << "Unknown match attribute '" << kind_s << "', disregarding pattern '"<< pattern << "'" << endl;
	    continue;
	}

	result.emplace_back(SolvableMatcher(pattern, kind, important));
    }
    return result;
}
