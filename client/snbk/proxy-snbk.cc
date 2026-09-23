/*
 * Copyright (c) 2026 SUSE LLC
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


#include "../utils/text.h"

#include "proxy-snbk.h"


namespace snapper
{


    ProxySnapshotsSnbk::ProxySnapshotsSnbk(const TheBigThings& the_big_things)
    {
	for (TheBigThings::const_iterator it = the_big_things.begin();
	     it != the_big_things.end(); ++it)
	{
	    // Only add valid and legacy snapshots to the list. The missing snapshots are
	    // managed by snapper, and the invalid snapshots should be fixed before
	    // cleanup.
	    if (it->target_state == TheBigThing::TargetState::VALID ||
	        it->target_state == TheBigThing::TargetState::LEGACY)
		proxy_snapshots.emplace_back(new ProxySnapshotSnbk(it));
	}
    }

    void SnbkCleanable::delete_snapshots(vector<ProxySnapshots::iterator> snapshots,
                                         bool verbose, Plugins::Report& report) const
    {
	for (ProxySnapshots::iterator proxy_it : snapshots)
	{
	    TheBigThings::iterator it = the_big_things.find(proxy_it->getNum());
	    if (it == the_big_things.end())
	    {
		SN_THROW(Exception(_("Cannot find the snapshot to delete.")));
	    }

	    it->remove(backup_config, verbose);
	}
    }


} // namespace snapper
