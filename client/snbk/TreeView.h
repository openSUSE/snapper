/*
 * Copyright (c) [2024-2025] SUSE LLC
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

#ifndef SNAPPER_SNBK_TREE_VIEW_H
#define SNAPPER_SNBK_TREE_VIEW_H


#include <map>
#include <memory>
#include <string>
#include <vector>

#include <boost/optional.hpp>

#include <snapper/Enum.h>


namespace snapper
{
    using std::shared_ptr;
    using std::string;
    using std::vector;


    class TreeView
    {
    public:

	/** The type of the parent node. */
	enum class ParentType
	{
	    NONE,           /** Not yet specified. */
	    DIRECT_PARENT,  /** An explicit parent Btrfs subvolume is specified. */
	    IMPLICIT_PARENT /** Manually assigned. */
	};

	/** Proxy class to the nodes (snapshots). */
	class ProxyNode
	{
	public:

	    virtual ~ProxyNode() = default;

	    /** Get the node number (snapshot number). */
	    virtual unsigned int get_number() const = 0;

	    /** Get the UUID on the sender side. */
	    virtual const string& get_uuid() const = 0;

	    /** Get the parent UUID on the sender side. */
	    virtual const string& get_parent_uuid() const = 0;

	    /** Determine whether the node can be used as a Btrfs send parent. */
	    virtual bool is_valid() const = 0;

	    /**
	     * Determine whether the node is a virtual node. If a Btrfs subvolume is not
	     * managed by Snapper, the corresponding node is considered virtual.
	     */
	    virtual bool is_virtual() const = 0;

	    /** Provided for ordering. This affects the priority of sibling nodes. */
	    virtual bool operator<(const ProxyNode& other) const;

	    ParentType parent_type = ParentType::NONE;
	    const ProxyNode* parent = nullptr;
	    vector<const ProxyNode*> children;
	};

	struct SearchResult
	{
	    const ProxyNode* node;
	    unsigned int distance;

	    SearchResult(const ProxyNode* node, unsigned int distance)
	        : node(node), distance(distance)
	    {
	    }
	};

	TreeView();
	TreeView(const vector<shared_ptr<ProxyNode>>& nodes);

	/** Find the nearest valid node to use as a Btrfs‑send parent. */
	boost::optional<SearchResult>
	find_nearest_valid_node(const string& start_uuid) const;

	/** Print the tree graph in Graphviz DOT Language. */
	void print_graph_graphviz(const string& rankdir = "LR") const;

    private:

	/** Virtual node for Btrfs subvolumes that are not managed by snapper. */
	class VirtualNode : public ProxyNode
	{
	public:

	    VirtualNode(const string& uuid);

	    unsigned int get_number() const override;
	    const string& get_uuid() const override;
	    const string& get_parent_uuid() const override;
	    bool is_virtual() const override;
	    bool is_valid() const override;

	private:

	    const string uuid;
	    const string parent_uuid;
	};

	/**
	 * Find the nearest valid node to use as a Btrfs‑send parent, starting from the
	 * given node.
	 */
	boost::optional<SearchResult>
	find_nearest_valid_node(const ProxyNode* start_node) const;

	/**
	 * Print the tree graph in Graphviz DOT Language, starting from the given node.
	 */
	static void print_graph_graphviz(const ProxyNode* node,
	                                 const string& rankdir = "LR");

	/**
	 * A static function that sets the parent relationship for the given two nodes.
	 */
	static void set_parent(ProxyNode* node, ProxyNode* parent,
	                       ParentType parent_type);

	/**
	 * A pool that stores and owns all nodes, including both real and virtual nodes.
	 */
	std::map<string, shared_ptr<ProxyNode>> pool;

	shared_ptr<ProxyNode> virtual_root;
    };

    template <> struct EnumInfo<TreeView::ParentType>
    {
	static const vector<string> names;
    };

} // namespace snapper

#endif
