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
string SnapperAgent::getValue (const YCPMap &map, const YCPString &key, const string &deflt)
{
    YCPValue val = map->value(key);
    if (!val.isNull() && val->isString())
	return val->asString()->value();
    else
	return deflt;
}

/**
 * Search the map for value of given key
 * @param map YCP Map to look in
 * @param key key we are looking for
 * @param deflt the default value to be returned if key is not found
 */
int SnapperAgent::getIntValue (const YCPMap &map, const YCPString &key, const int deflt)
{
    YCPValue val = map->value(key);

    if (!val.isNull() && val->isInteger()) {
	return val->asInteger()->value();
    }
    else if (!val.isNull() && val->isString()) {
	return YCPInteger (val->asString()->value().c_str())->value ();
    }
    return deflt;
}

/**
 * Search the map for value of given key;
 * key is string and value is YCPList
 */
YCPList SnapperAgent::getListValue (const YCPMap &map, const YCPString &key)
{
    YCPValue val = map->value(key);
    if (!val.isNull() && val->isList())
	return val->asList();
    else
	return YCPList();
}

/**
 * Search the map for value of given key;
 * key is string and value is YCPMap
 */
YCPMap SnapperAgent::getMapValue (const YCPMap &map, const YCPString &key)
{
    YCPValue val = map->value(key);
    if (!val.isNull() && val->isMap())
        return val->asMap();
    else
        return YCPMap();
}

YCPMap map2ycpmap (const map<string, string>& userdata)
{
    YCPMap m;  
    for (map<string, string>::const_iterator it = userdata.begin(); it != userdata.end(); ++it)
    {
        m->add (YCPString (it->first), YCPString (it->second));
    }
    return m;
}

map<string, string> ycpmap2stringmap (const YCPMap &ycp_map)
{
    map<string, string> m; 

    for (YCPMapIterator i = ycp_map->begin(); i != ycp_map->end(); i++) {
        string key = i->first->asString()->value();
        m[key]  = i->second->asString()->value();
    }
    return m;
}


void
log_do(LogLevel level, const string& component, const char* file, const int line, const char* func,
       const string& text)
{
    static const loglevel_t ln[4] = { LOG_DEBUG, LOG_MILESTONE, LOG_WARNING, LOG_ERROR };

    y2_logger_function(ln[level], component, file, line, func, "%s", text.c_str());
}


bool
log_query(LogLevel level, const string& component)
{
    static const loglevel_t ln[4] = { LOG_DEBUG, LOG_MILESTONE, LOG_WARNING, LOG_ERROR };

    return should_be_logged(ln[level], component);
}


/**
 * Constructor
 */
SnapperAgent::SnapperAgent() : SCRAgent()
{
    sh			= NULL;
    snapper_initialized	= false;
    snapper_error	= "";

    snapper::setLogDo(&log_do);
    snapper::setLogQuery(&log_query);
}

/**
 * Destructor
 */
SnapperAgent::~SnapperAgent()
{
    if (sh)
    {
	delete sh;
	sh = 0;
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

    if (!snapper_initialized && PC(0) != "error" && PC(0) != "configs") {
	y2error ("snapper not initialized: use Execute (.snapper) first!");
	snapper_error = "not_initialized";
	return YCPVoid();
    }
	
    if (path->length() == 1) {

	if (PC(0) == "configs") {
	    YCPList retlist;

	    try {
		list<ConfigInfo> configs = Snapper::getConfigs();
		for (list<ConfigInfo>::const_iterator it = configs.begin(); it != configs.end(); ++it)
		{
		    retlist->add (YCPString (it->config_name));
		}
	    }
	    catch (const ListConfigsFailedException& e)
	    {
		y2error ("sysconfig file not found.");
		snapper_error	= "sysconfig_not_found";
		return YCPVoid();
	    }
	    return retlist;
	}
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
	    unsigned int num    = getIntValue (argmap, YCPString ("num"), 0);
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
		s->add (YCPString ("description"), YCPString (it->getDescription()));

		if (it->getType() == PRE)
		{
		    s->add (YCPString ("post_num"), YCPInteger (snapshots.findPost (it)->getNum ()));
		}
		else if (it->getType() == POST)
		{
		    s->add (YCPString ("pre_num"), YCPInteger (it->getPreNum()));
		}
                s->add (YCPString ("userdata"), YCPMap (map2ycpmap (it->getUserdata())));
                s->add (YCPString ("cleanup"), YCPString (it->getCleanup ()));

		y2debug ("snapshot %s", s.toString().c_str());
		retlist->add (s);
	    }
	    return retlist;
	}

	unsigned int num1	= getIntValue (argmap, YCPString ("from"), 0);
	unsigned int num2	= getIntValue (argmap, YCPString ("to"), 0);

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
		YCPMap file_map;
		file_map->add (YCPString ("status"), YCPString (statusToString (it->getPreToPostStatus())));
		file_map->add (YCPString ("full_path"), YCPString (it->getAbsolutePath (LOC_SYSTEM)));
		retmap->add (YCPString (it->getName()), file_map);
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
    
	snapper_initialized	= false;
	if (sh)
	{
	    y2milestone ("deleting existing snapper object");
	    delete sh;
	    sh = 0;
	}
	string config_name = getValue (argmap, YCPString ("config"), "root");
	try
	{
	    sh = new Snapper(config_name);
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

	if (PC(0) == "create") {

            string description  = getValue (argmap, YCPString ("description"), "");
            string cleanup      = getValue (argmap, YCPString ("cleanup"), "");
            string type         = getValue (argmap, YCPString ("type"), "single");
            YCPMap userdata     = getMapValue (argmap, YCPString ("userdata"));

            const Snapshots& snapshots          = sh->getSnapshots();
            Snapshots::iterator snap;

            if (type == "single") {
                snap    = sh->createSingleSnapshot(description);
            }
            else if (type == "pre") {
                snap    = sh->createPreSnapshot(description);
            }
            else if (type == "post") {
                // check if pre was given!
                int pre = getIntValue (argmap, YCPString ("pre"), -1);
                if (pre == -1)
                {
                    snapper_error       = "pre_not_given";
                    return YCPBoolean (false);
                }
                else
                {
                    Snapshots::const_iterator snap1 = snapshots.find (pre);
                    if (snap1 == snapshots.end())
                    {
                        snapper_error   = "pre_not_found";
                        return YCPBoolean (false);
                    }
                    else
                    {
                        snap    = sh->createPostSnapshot(description, snap1);
                    }
                }
            }
            else {
                snapper_error   = "wrong_snapshot_type";
                return YCPBoolean (false);
            }

            snap->setCleanup (cleanup);
            snap->setUserdata (ycpmap2stringmap (userdata));
            snap->flushInfo();
            return ret;
        }
        else if (PC(0) == "modify") {

            int num     = getIntValue (argmap, YCPString ("num"), 0);

            Snapshots& snapshots = sh->getSnapshots();
            Snapshots::iterator snap = snapshots.find(num);
            if (snap == snapshots.end())
            {
                y2error ("snapshot '%d' not found", num);
                snapper_error   = "snapshot_not_found";
                return YCPBoolean (false);
            }

            if (argmap->hasKey(YCPString ("description")))
            {
                snap->setDescription (getValue (argmap, YCPString ("description"), ""));
            }
            if (argmap->hasKey(YCPString ("cleanup")))
            {
                snap->setCleanup (getValue (argmap, YCPString ("cleanup"), ""));
            }
            if (argmap->hasKey(YCPString ("userdata")))
            {
                snap->setUserdata (ycpmap2stringmap (getMapValue (argmap, YCPString ("userdata"))));
            }
            snap->flushInfo();
            return ret;
        }
	/**
	 * Rollback the list of given files from snapshot num1 to num2 (system by default)
	 */
        else if (PC(0) == "rollback") {

	    unsigned int num1	= getIntValue (argmap, YCPString ("from"), 0);
	    unsigned int num2	= getIntValue (argmap, YCPString ("to"), 0);
	    const Snapshots& snapshots = sh->getSnapshots();
	    Comparison comparison(sh, snapshots.find(num1), snapshots.find(num2));
	    Files& files = comparison.getFiles();

	    YCPList selected = getListValue (argmap, YCPString ("files"));
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
			it->setUndo(true);
			it->doUndo();
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
