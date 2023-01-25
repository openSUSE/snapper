/*
 * Copyright (c) [2019-2023] SUSE LLC
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

#include "solvable-matcher.h"

#include <iostream>
#include <vector>
#include <string>
#include <regex>
#include <fnmatch.h>
#include "snapper/XmlFile.h"

using namespace std;
using namespace snapper;


std::ostream& SolvableMatcher::log = std::cerr;


bool
SolvableMatcher::match(const string& solvable) const
{
    log << "DEBUG:"
	<< "match? " << solvable
	<< " by " << ((kind == Kind::GLOB)? "GLOB '": "REGEX '")
	<< pattern << '\'' << endl;

    bool res = false;

    switch (kind)
    {
	case Kind::GLOB:
	{
	    static const int flags = 0;
	    res = fnmatch(pattern.c_str(), solvable.c_str(), flags) == 0;
	}
	break;

	case Kind::REGEX:
	{
	    try
	    {
		// POSIX Extended Regular Expression Syntax
		// The original Python implementation allows "foo" to match "foo-devel"
		const regex rx_pattern("^" + pattern, regex::extended);
		res = regex_search(solvable, rx_pattern);
	    }
	    catch (const regex_error& e)
	    {
		log << "ERROR:" << "Regex compilation error:" << e.what() << + " pattern:'" << pattern
		    << "'" << endl;
	    }
	}
	break;
    }

    log << "DEBUG:" << "-> " << res << endl;
    return res;
}


vector<SolvableMatcher>
SolvableMatcher::load_config(const string& cfg_filename)
{
    log << "DEBUG:" << "parsing " << cfg_filename << endl;

    vector<SolvableMatcher> result;

    try
    {
	XmlFile config(cfg_filename);

	const xmlNode* root = config.getRootElement();
	const xmlNode* solvables_n = getChildNode(root, "solvables");
	const list<const xmlNode*> solvables_l = getChildNodes(solvables_n, "solvable");
	for (const xmlNode* node : solvables_l)
	{
	    string pattern;
	    getValue(node, pattern);

	    Kind kind;
	    string kind_s;
	    getAttributeValue(node, "match", kind_s);
	    getValue(node, pattern);
	    if (kind_s == "w")		// w = wildcard
		kind = Kind::GLOB;
	    else if (kind_s == "re")	// re = Regular Expression
		kind = Kind::REGEX;
	    else
	    {
		log << "ERROR:" << "Unknown match attribute '" << kind_s << "', disregarding pattern '"
		    << pattern << "'" << endl;
		continue;
	    }

	    bool important = false;
	    getAttributeValue(node, "important", important);

	    result.emplace_back(pattern, kind, important);
	}
    }
    catch (const exception& e)
    {
	log << "ERROR:" << "Loading XML failed" << endl;
    }

    return result;
}
