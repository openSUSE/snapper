/* SnapperAgent.h
 *
 * Snapper agent implementation
 *
 * Authors: Jiri Suchomel <jsuchome@suse.cz>
 *
 * $Id: SnapperAgent.h 63174 2011-01-13 10:50:42Z jsuchome $
 */

#ifndef _SnapperAgent_h
#define _SnapperAgent_h

#include <Y2.h>
#include <scr/SCRAgent.h>

#include <snapper/Snapper.h>
#include <snapper/Snapshot.h>
#include <snapper/Comparison.h>
#include <snapper/File.h>
#include <snapper/Logger.h>

/**
 * @short An interface class between YaST2 and Snapper Agent
 */
class SnapperAgent : public SCRAgent
{
private:
    /**
     * Agent private variables and methods
     */

    snapper::Snapper* sh;
    bool snapper_initialized;
    string snapper_error;

    /**
     * search the map for value of given key; both key and value have to be strings
     * when key is not present, default value is returned
     */
    string getValue (const YCPMap &map, const YCPString &key, const string &deflt);

    /**
     * Search the map for value of given key
     * @param map YCP Map to look in
     * @param key key we are looking for
     * @param deflt the default value to be returned if key is not found
     */
    int getIntValue ( const YCPMap &map, const YCPString &key, const int deflt);

    /**
     * Search the map for value of given key;
     * key is string and value is YCPList
     */
    YCPList getListValue (const YCPMap &map, const YCPString &key);

public:
    /**
     * Default constructor.
     */
    SnapperAgent();

    /**
     * Destructor.
     */
    virtual ~SnapperAgent();

    /**
     * Provides SCR Read ().
     * @param path Path that should be read.
     * @param arg Additional parameter.
     */
    virtual YCPValue Read ( const YCPPath &path,
			    const YCPValue& arg = YCPNull(),
			    const YCPValue& opt = YCPNull());

    /**
     * Provides SCR Write ().
     */
    virtual YCPBoolean Write(const YCPPath &path,
			   const YCPValue& arg,
			   const YCPValue& arg2 = YCPNull());

    /**
     * Provides SCR Execute ().
     */
    virtual YCPValue Execute(const YCPPath &path,
			     const YCPValue& arg = YCPNull(),
			     const YCPValue& arg2 = YCPNull());

    /**
     * Provides SCR Dir ().
     */
    virtual YCPList Dir(const YCPPath& path);

    /**
     * Used for mounting the agent.
     */
    virtual YCPValue otherCommand(const YCPTerm& term);
};

#endif /* _SnapperAgent_h */
