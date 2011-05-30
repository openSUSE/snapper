/* SnapperAgent.cc
 *
 * An agent for accessing snapper library
 *
 * Authors: Jiri Suchomel <jsuchome@suse.cz>
 *
 * $Id: SnapperAgent.cc 63174 2011-01-13 10:50:42Z jsuchome $
 */

#include "SnapperAgent.h"
#include <ctype.h>
#include <boost/algorithm/string.hpp>

#define PC(n)       (path->component_str(n))

using namespace snapper;

/*
 * search the map for value of given key; both key and value have to be strings
 */
string SnapperAgent::getValue (const YCPMap map, const string key)
{
    if (!map->value(YCPString(key)).isNull()
	&& map->value(YCPString(key))->isString())
	return map->value(YCPString(key))->asString()->value();
    else
	return "";
}

/**
 * Search the map for value of given key
 * @param map YCP Map to look in
 * @param key key we are looking for
 * @param deflt the default value to be returned if key is not found
 */
int SnapperAgent::getIntValue (const YCPMap map, const string key, int deflt)
{
    if (!map->value(YCPString(key)).isNull() && map->value(YCPString(key))->isInteger()) {
	return map->value(YCPString(key))->asInteger()->value(); 
    }
    else if (!map->value(YCPString(key)).isNull() &&
	     map->value(YCPString(key))->isString()) {
	YCPInteger i (map->value(YCPString(key))->asString()->value().c_str());
	return i->value();
    }
    return deflt;
}

/**
 * Search the map for value of given key;
 * key is string and value is YCPList
 */
YCPList SnapperAgent::getListValue (const YCPMap map, const string key)
{
    if (!map->value(YCPString(key)).isNull() && map->value(YCPString(key))->isList())
	return map->value(YCPString(key))->asList();
    else
	return YCPList();
}


string statusToString(unsigned int status)
{
	string ret;

	if (status & CREATED)
	    ret += "+";
	else if (status & DELETED)
	    ret += "-";
	else if (status & TYPE)
	    ret += "t";
	else if (status & CONTENT)
	    ret += "c";
	else
	    ret += ".";

	ret += status & PERMISSIONS ? "p" : ".";
	ret += status & USER ? "u" : ".";
	ret += status & GROUP ? "g" : ".";

	return ret;
}

/**
 * Constructor
 */
SnapperAgent::SnapperAgent() : SCRAgent()
{
    sh			= NULL;
    snapper_initialized	= false;
    snapper_error	= "";
}

/**
 * Destructor
 */
SnapperAgent::~SnapperAgent()
{
    if (sh)
    {
	deleteSnapper(sh);
    }
}


/**
 * Dir
 */
YCPList SnapperAgent::Dir(const YCPPath& path)
{
    y2error("Wrong path '%s' in Read().", path->toString().c_str());
    return YCPNull();
}


struct Tree { map<string, Tree> trees; };

static YCPMap
make_ycpmap(const Tree& tree)
{
    YCPMap ret;

    for (map<string, Tree>::const_iterator it = tree.trees.begin(); it != tree.trees.end(); ++it)
	ret->add(YCPString(it->first), make_ycpmap(it->second));

    return ret;
}


/**
 * Read
 */
YCPValue SnapperAgent::Read(const YCPPath &path, const YCPValue& arg, const YCPValue& opt) {

    y2debug ("path in Read: '%s'.", path->toString().c_str());
    YCPValue ret = YCPVoid();

    YCPMap argmap;
    if (!arg.isNull() && arg->isMap())
    	argmap = arg->asMap();

    if (!snapper_initialized && PC(0) != "error") {
	y2error ("snapper not initialized: use Execute (.snapper) first!");
	snapper_error = "not_initialized";
	return YCPVoid();
    }
	
    if (path->length() == 1) {

	/**
	 * Read (.snapper.error) -> returns last error message
	 */
	if (PC(0) == "error") {
	    YCPMap retmap;
	    retmap->add (YCPString ("type"), YCPString (snapper_error));
	    return retmap;
	}
	/**
	 * Read (.snapper.path, $[ "num" : num]) -> returns the path to directory with given snapshot
	 */
	if (PC(0) == "path") {
	    unsigned int num    = getIntValue (argmap, "num", 0);
	    const Snapshots& snapshots = sh->getSnapshots();
	    Snapshots::const_iterator snap = snapshots.find(num);
	    if (snap == snapshots.end())
	    {
		y2error ("snapshot '%d' not found", num);
		return ret;
	    }
	    return YCPString (snap->snapshotDir());
	}

	/**
	 * Read(.snapper.snapshots) -> return list of snapshot description maps
	 */
	if (PC(0) == "snapshots") {
	    YCPList retlist;
	    const Snapshots& snapshots = sh->getSnapshots();
	    for (Snapshots::const_iterator it = snapshots.begin(); it != snapshots.end(); ++it)
	    {
		YCPMap s;

		switch (it->getType())
		{
		    case SINGLE: s->add (YCPString ("type"), YCPSymbol ("SINGLE")); break; 
		    case PRE: s->add (YCPString ("type"), YCPSymbol ("PRE")); break; 
		    case POST: s->add (YCPString ("type"), YCPSymbol ("POST")); break;
		}

		s->add (YCPString ("num"), YCPInteger (it->getNum()));
		s->add (YCPString ("date"), YCPInteger (it->getDate()));

		if (it->getType() == SINGLE || it->getType() == PRE)
		{
		    s->add (YCPString ("description"), YCPString (it->getDescription()));
		    if (it->getType() == PRE)
			s->add (YCPString ("post_num"), YCPInteger (snapshots.findPost (it)->getNum ()));
		}
		else if (it->getType() == POST)
		{
		    s->add (YCPString ("pre_num"), YCPInteger (it->getPreNum()));
		}

		y2debug ("snapshot %s", s.toString().c_str());
		retlist->add (s);
	    }
	    return retlist;
	}

	unsigned int num1	= getIntValue (argmap, "from", 0);
	unsigned int num2	= getIntValue (argmap, "to", 0);

	/**
	 * Read(.snapper.diff_list) -> show difference between snapnots num1 and num2 as list.
	 */
	if (PC(0) == "diff_list") {
	    YCPList retlist;

	    const Snapshots& snapshots = sh->getSnapshots();
	    const Comparison comparison(sh, snapshots.find(num1), snapshots.find(num2));
	    const Files& files = comparison.getFiles();

	    for (Files::const_iterator it = files.begin(); it != files.end(); ++it)
	    {
		YCPMap filemap;
		filemap->add (YCPString ("name"), YCPString (it->getName()));
		filemap->add (YCPString ("changes"), YCPString (statusToString (it->getPreToPostStatus())));
		retlist->add (filemap);
	    }
	    return retlist;
	}
	/**
	 * Read(.snapper.diff_index) -> show difference between snapnots num1 and num2 as one-level map:
	 * (mapping each file to its changes)
	 */
	if (PC(0) == "diff_index") {
	    YCPMap retmap;

	    const Snapshots& snapshots = sh->getSnapshots();
	    const Comparison comparison(sh, snapshots.find(num1), snapshots.find(num2));
	    const Files& files = comparison.getFiles();

	    for (Files::const_iterator it = files.begin(); it != files.end(); ++it)
	    {
		retmap->add (YCPString (it->getName()), YCPString (statusToString (it->getPreToPostStatus())));
	    }
	    return retmap;
	}
	/**
	 * Read(.snapper.diff_tree) -> show difference between snapnots num1 and num2 as tree.
	 */
	else if (PC(0) == "diff_tree")
	{
	    Tree ret1;

	    const Snapshots& snapshots = sh->getSnapshots();
	    const Comparison comparison(sh, snapshots.find(num1), snapshots.find(num2));
	    const Files& files = comparison.getFiles();
	    for (Files::const_iterator it = files.begin(); it != files.end(); ++it)
	    {
		deque<string> parts;
		boost::split(parts, it->getName(), boost::is_any_of("/"));
		parts.pop_front();

		Tree* tmp = &ret1;
		for (deque<string>::const_iterator it = parts.begin(); it != parts.end(); ++it)
		{
		    map<string, Tree>::iterator pos = tmp->trees.find(*it);
		    if (pos == tmp->trees.end())
			pos = tmp->trees.insert(tmp->trees.begin(), make_pair(*it, Tree()));
		    tmp = &pos->second;
		}
	    }

	    return make_ycpmap(ret1);
	}
	else {
	    y2error("Wrong path '%s' in Read().", path->toString().c_str());
	}
    }
    else if (path->length() == 2) {

	    y2error("Wrong path '%s' in Read().", path->toString().c_str());
    }
    else {
	y2error("Wrong path '%s' in Read().", path->toString().c_str());
    }
    return YCPVoid();
}

/**
 * Write
 */
YCPBoolean SnapperAgent::Write(const YCPPath &path, const YCPValue& arg,
       const YCPValue& arg2)
{
    y2debug ("path in Write: '%s'.", path->toString().c_str());

    YCPBoolean ret = YCPBoolean(true);
    return ret;
}

/**
 * Execute
 */
YCPValue SnapperAgent::Execute(const YCPPath &path, const YCPValue& arg,
	const YCPValue& arg2)
{
    y2debug ("path in Execute: '%s'.", path->toString().c_str());
    YCPValue ret = YCPBoolean (true);

    YCPMap argmap;
    if (!arg.isNull() && arg->isMap())
    	argmap = arg->asMap();

    /**
     * Execute (.snapper) call: Initialize snapper object
     */
    if (path->length() == 0) {

	try {
	    sh = createSnapper();
	}
	catch (const ConfigNotFoundException& e)
	{
	    y2error ("Config not found.");
	    snapper_error	= "config_not_found";
	    sh			= NULL;
	    return YCPBoolean (false);
	}
	catch (const InvalidConfigException& e)
	{
	    y2error ("Config is invalid.");
	    snapper_error	= "config_invalid";
	    sh			= NULL;
	    return YCPBoolean (false);
	}
	snapper_initialized	= true;
	return ret;
    }

    if (!snapper_initialized) {
	y2error ("snapper not initialized: use Execute (.snapper) first!");
	snapper_error = "not_initialized";
	return YCPVoid();
    }

    if (path->length() == 1) {

	/**
	 * Rollback the list of given files from snapshot num1 to num2 (system by default)
	 */
	if (PC(0) == "rollback") {

	    unsigned int num1	= getIntValue (argmap, "from", 0);
	    unsigned int num2	= getIntValue (argmap, "to", 0);
	    const Snapshots& snapshots = sh->getSnapshots();
	    Comparison comparison(sh, snapshots.find(num1), snapshots.find(num2));
	    Files& files = comparison.getFiles();

	    YCPList selected = getListValue (argmap, "files");
	    for (int i=0; i < selected->size(); i++) {
		if (selected.value(i)->isString())
		{
		    string name = selected->value(i)->asString()->value();
		    y2debug ("file to rollback: %s", name.c_str());
		    Files::iterator it = files.find(name);
		    if (it == files.end())
		    {
			y2error ("file %s not found in diff", name.c_str());
		    }
		    else
		    {
			it->setRollback(true);
			it->doRollback ();
		    }
		}
	    }
	    return ret;
	}

    }
    else {
	y2error("Wrong path '%s' in Execute().", path->toString().c_str());
    }

    return YCPVoid ();
}

/**
 * otherCommand
 */
YCPValue SnapperAgent::otherCommand(const YCPTerm& term)
{
    string sym = term->name();

    if (sym == "SnapperAgent") {
        /* Your initialization */
        return YCPVoid();
    }

    return YCPNull();
}
