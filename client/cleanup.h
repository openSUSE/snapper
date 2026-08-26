/*
 * Copyright (c) [2011-2012] Novell, Inc.
 * Copyright (c) [2016-2026] SUSE LLC
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


#include <functional>

#include <snapper/Enum.h>

#include "proxy/locker.h"
#include "proxy/proxy.h"


namespace snapper
{

    enum class CleanupAlgorithm
    {
	ALL,
	NUMBER,
	TIMELINE,
	EMPTY_PRE_POST
    };

    template <> struct EnumInfo<CleanupAlgorithm>
    {
	static const vector<string> names;
    };


    /**
     * Base class providing the interfaces required by cleanup algorithms (classes derived
     * from `Cleaner`)
     */
    class ProxyCleanable
    {
    public:

	virtual ~ProxyCleanable() {}

	virtual ProxySnapshots& get_snapshots() = 0;
	virtual void delete_snapshots(vector<ProxySnapshots::iterator> snapshots,
	                              bool verbose, Plugins::Report& report) const = 0;
	virtual void prepare_quota() const = 0;
	virtual QuotaData query_quota_data() const = 0;
	virtual FreeSpaceData query_free_space_data() const = 0;
	virtual ProxyComparison create_comparison(const ProxySnapshot& lhs,
	                                          const ProxySnapshot& rhs,
	                                          bool mount) const = 0;
    };


    /**
     * Pre-defined cleanup operations.
     */
    class CleanupOperation
    {
    public:

	virtual ~CleanupOperation() {}

	/*
	 * The following three functions do the cleanup based on the conditionals defined
	 * in the config, that are hard limit, quota and free space.
	 */
	void do_cleanup_number(bool verbose, Plugins::Report& report);
	void do_cleanup_timeline(bool verbose, Plugins::Report& report);
	void do_cleanup_empty_pre_post(bool verbose, Plugins::Report& report);


	/*
	 * The following three functions do the cleanup only based on the provided
	 * conditional. The lower range and min-age defined in the config are respected.
	 */
	void do_cleanup_number(bool verbose, std::function<bool()> condition,
	                       Plugins::Report& report);
	void do_cleanup_timeline(bool verbose, std::function<bool()> condition,
	                         Plugins::Report& report);
	void do_cleanup_empty_pre_post(bool verbose, std::function<bool()> condition,
	                               Plugins::Report& report);

	/**
	 * This member function should return a config containing the snapshot retention
	 * policy. The retention policy settings should be accessible from the root level.
	 */
	virtual const ProxyConfig& get_config() const = 0;

    protected:

	/**
	 * This member function should return a cleanable object.
	 */
	virtual ProxyCleanable& get_cleanable() = 0;
    };


    class SnapperCleanable : public ProxyCleanable
    {
    public:

	SnapperCleanable(ProxySnapper* snapper) : snapper(snapper), locker(snapper) {}

	virtual ProxySnapshots& get_snapshots() override
	{
	    return snapper->getSnapshots();
	}

	virtual void delete_snapshots(vector<ProxySnapshots::iterator> snapshots,
	                              bool verbose,
	                              Plugins::Report& report) const override
	{
	    snapper->deleteSnapshots(snapshots, verbose, report);
	}

	virtual void prepare_quota() const override { snapper->prepareQuota(); }

	virtual QuotaData query_quota_data() const override
	{
	    return snapper->queryQuotaData();
	}

	virtual FreeSpaceData query_free_space_data() const override
	{
	    return snapper->queryFreeSpaceData();
	}

	virtual ProxyComparison create_comparison(const ProxySnapshot& lhs,
	                                          const ProxySnapshot& rhs,
	                                          bool mount) const override
	{
	    return snapper->createComparison(lhs, rhs, mount);
	}


    private:

	ProxySnapper* snapper;
	Locker locker;
    };


    class SnapperCleanup : public CleanupOperation
    {
    public:

	SnapperCleanup(ProxySnapper* snapper)
	    : config(ProxyConfig(snapper->getConfig())),
	      cleanable(SnapperCleanable(snapper))
	{
	}

	virtual const ProxyConfig& get_config() const override { return config; }

    protected:

	virtual ProxyCleanable& get_cleanable() override { return cleanable; }

    private:

	ProxyConfig config;
	SnapperCleanable cleanable;
    };


} // namespace snapper
