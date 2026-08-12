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

#ifndef SNAPPER_PROXY_SNBK_H
#define SNAPPER_PROXY_SNBK_H


#include <snapper/Exception.h>

#include <../proxy/proxy.h>

#include "TheBigThing.h"


namespace snapper
{


    /**
     * This class is designed to run cleanup algorithms. Only the subset of interfaces
     * required by cleanup is implemented.
     */
    class ProxySnapshotSnbk : public ProxySnapshot::Impl
    {
    public:

	ProxySnapshotSnbk(const TheBigThings::const_iterator& it) : it(it) {}

	virtual SnapshotType getType() const override { return it->meta.type; }
	virtual unsigned int getNum() const override { return it->num; };
	virtual time_t getDate() const override { return it->meta.date; };
	virtual uid_t getUid() const override
	{
	    SN_THROW(UnsupportedException());
	    __builtin_unreachable();
	};

	virtual bool isReadOnly() const override
	{
	    // The transferred snapshots are always read-only.
	    // (snbk only transfers read-only snapshots.)
	    return true;
	};

	virtual void setReadOnly(bool read_only, Plugins::Report& report) override
	{
	    SN_THROW(UnsupportedException());
	    __builtin_unreachable();
	}

	virtual unsigned int getPreNum() const override { return it->meta.pre_num; };
	virtual const string& getDescription() const override
	{
	    // Even though implementing this method is possible, the snapshot description
	    // is not required by cleanup algorithms.
	    SN_THROW(UnsupportedException());
	    __builtin_unreachable();
	}

	virtual const string& getCleanup() const override { return it->meta.cleanup; };
	virtual const map<string, string>& getUserdata() const override
	{
	    return it->meta.userdata;
	};

	virtual bool isCurrent() const override { return false; };

	virtual uint64_t getUsedSpace() const override
	{
	    SN_THROW(UnsupportedException());
	    __builtin_unreachable();
	};

	virtual string mountFilesystemSnapshot(bool user_request) const override
	{
	    SN_THROW(UnsupportedException());
	    __builtin_unreachable();
	}

	virtual void umountFilesystemSnapshot(bool user_request) const override
	{
	    SN_THROW(UnsupportedException());
	    __builtin_unreachable();
	}

	const TheBigThings::const_iterator it;
    };


    class ProxySnapshotsSnbk : public ProxySnapshots
    {
    public:

	ProxySnapshotsSnbk(const TheBigThings& the_big_things);

	/** snbk doesn't have default snapshots. */
	virtual iterator getDefault() override { return end(); };
	virtual const_iterator getDefault() const override { return end(); }

	/** snbk doesn't have active snapshots. */
	virtual iterator getActive() override { return end(); };
	virtual const_iterator getActive() const override { return end(); };
    };


} // namespace snapper


#endif
