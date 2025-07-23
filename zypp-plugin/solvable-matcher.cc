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

#include <fnmatch.h>
#include <regex>
#include "snapper/XmlFile.h"
#include "snapper/LoggerImpl.h"

using namespace std;
using namespace snapper;


bool
SolvableMatcher::match(const string& solvable) const
{
    y2deb("match '" << solvable << "' by " << (kind == Kind::GLOB ? "GLOB '": "REGEX '") << pattern << '\'');

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
		y2err("Regex compilation error:" << e.what() << + " pattern:'" << pattern << "'");
	    }
	}
	break;
    }

    y2deb("-> " << res);
    return res;
}


vector<SolvableMatcher>
SolvableMatcher::load_config(const string& cfg_filename)
{
    y2deb("parsing " << cfg_filename);

    vector<SolvableMatcher> result;

    try
    {
	XmlFile config(cfg_filename);

	const xmlNode* root = config.getRootElement();
	const xmlNode* solvables_n = getChildNode(root, "solvables");
	const vector<const xmlNode*> solvables_l = getChildNodes(solvables_n, "solvable");
	for (const xmlNode* node : solvables_l)
	{
	    string pattern;
	    getValue(node, pattern);

	    Kind kind;
	    string kind_s;
	    getAttributeValue(node, "match", kind_s);
	    if (kind_s == "w")		// w = Wildcard
		kind = Kind::GLOB;
	    else if (kind_s == "re")	// re = Regular Expression
		kind = Kind::REGEX;
	    else
	    {
		y2err("Unknown match attribute '" << kind_s << "', disregarding pattern '" <<
		      pattern << "'");
		continue;
	    }

	    bool important = false;
	    getAttributeValue(node, "important", important);

	    result.emplace_back(pattern, kind, important);
	}
    }
    catch (const exception& e)
    {
	y2err("Loading XML failed");
    }

    return result;
}
