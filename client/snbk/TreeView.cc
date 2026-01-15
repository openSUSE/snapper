/*
 * Copyright (c) [2024-2026] SUSE LLC
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

#include <iostream>
#include <memory>
#include <queue>
#include <set>
#include <unordered_set>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>

#include <snapper/AppUtil.h>

#include "../utils/text.h"

#include "TreeView.h"


using namespace std;
using namespace snapper;


namespace snapper
{

    namespace
    {
	// Comparator for shared pointers to nodes.
	struct NodePtrComparator
	{
	    bool operator()(const shared_ptr<TreeView::ProxyNode>& lhs,
			    const shared_ptr<TreeView::ProxyNode>& rhs) const
	    {
		return (*lhs) < (*rhs);
	    }
	};

	string get_node_name(const TreeView::ProxyNode* node)
	{
	    if (node->is_virtual())
	    {
		// Use the object's address as the node name to avoid name collisions.
		stringstream ss;
		ss.imbue(locale::classic());
		ss << node;
		return ss.str();
	    }
	    else
	    {
		// Use the snapshot number as the node name.
		return to_string(node->get_number());
	    }
	}

	string get_node_id(const TreeView::ProxyNode* node)
	{
	    // Graphviz node IDs cannot start with a digit.
	    // Prefix the name with “n” to guarantee a valid identifier.
	    return "n" + get_node_name(node);
	}

	string get_node_declaration(const TreeView::ProxyNode* node)
	{
	    // Get the node's properties.
	    vector<string> properties;
	    if (node->is_virtual())
	    {
		properties.push_back("virtual");
	    }

	    if (node->is_valid())
	    {
		properties.push_back("valid");
	    }

	    // Compose the node declaration.
	    string label = get_node_name(node);
	    if (!properties.empty())
	    {
		label = label + "\\n" + boost::join(properties, ", ");
	    }

	    return sformat(_("%s [ label=\"%s\", style=\"%s\", shape=\"box\"]"),
	                   get_node_id(node).c_str(), label.c_str(),
	                   node->is_virtual() ? "dashed" : "");
	}

	void print_graph_graphviz_recursive(const TreeView::ProxyNode* node)
	{
	    cout << "    " << get_node_declaration(node) << '\n';

	    for (const TreeView::ProxyNode* child : node->children)
	    {
		const char* link_style = nullptr;
		switch (child->parent_type)
		{
		    case TreeView::ParentType::DIRECT_PARENT:
			link_style = "";
			break;

		    case TreeView::ParentType::IMPLICIT_PARENT:
			link_style = "dashed";
			break;

		    case TreeView::ParentType::NONE:
			SN_THROW(Exception("Invalid parent type."));
		}

		cout << sformat(_("    %s -> %s [style=\"%s\"]"),
		                get_node_id(node).c_str(), get_node_id(child).c_str(),
		                link_style)
		     << '\n';

		print_graph_graphviz_recursive(child);
	    }
	}

    }


    bool TreeView::ProxyNode::operator < (const ProxyNode& other) const
    {
	// A node with a greater number (more recent) has a higher priority.
	return get_number() > other.get_number();
    }


    TreeView::VirtualNode::VirtualNode(const string& uuid) : uuid(uuid), parent_uuid("")
    {
    }

    unsigned int TreeView::VirtualNode::get_number() const
    {
	// A constant number used for virtual nodes.
	// Assume that there won't be so many snapshots.
	return numeric_limits<unsigned int>::max();
    }

    const string& TreeView::VirtualNode::get_uuid() const { return uuid; }
    const string& TreeView::VirtualNode::get_parent_uuid() const { return parent_uuid; }
    bool TreeView::VirtualNode::is_virtual() const { return true; }
    bool TreeView::VirtualNode::is_valid() const { return false; }


    TreeView::TreeView() : virtual_root(make_shared<VirtualNode>("virtual_root")) {}

    TreeView::TreeView(const vector<shared_ptr<ProxyNode>>& nodes) : TreeView()
    {
	// Construct a sorted container of source nodes in descending order.
	set<shared_ptr<ProxyNode>, NodePtrComparator> sorted_nodes;
	for (const shared_ptr<ProxyNode>& node : nodes)
	{
	    if (!node->get_uuid().empty())
	    {
		sorted_nodes.insert(node);
	    }
	}

	// Insert and take ownership of external nodes into the pool.
	for (const shared_ptr<ProxyNode>& node : sorted_nodes)
	{
	    pool[node->get_uuid()] = node;
	}

	// Construct virtual nodes for unmanaged parent Btrfs subvolumes.
	for (const shared_ptr<ProxyNode>& node : sorted_nodes)
	{
	    string uuid = node->get_parent_uuid();
	    if (!uuid.empty())
	    {
		if (pool.find(uuid) == pool.end())
		{
		    // Create a virtual node and insert it into the pool.
		    shared_ptr<ProxyNode> tmp_node = make_shared<VirtualNode>(uuid);
		    pool[uuid] = tmp_node;

		    // Make it an implicit child of the virtual root.
		    set_parent(tmp_node.get(), virtual_root.get(),
		               ParentType::IMPLICIT_PARENT);

		    y2deb("Added virtual node for unmanaged Btrfs subvolume: " << uuid);
		}
	    }
	}

	// Construct the tree.
	for (const shared_ptr<ProxyNode>& node : sorted_nodes)
	{
	    // Find its parent.
	    string parent_uuid = node->get_parent_uuid();
	    if (parent_uuid.empty())
	    {
		// If the parent UUID is missing, add the node as a child of the virtual
		// root to prevent orphaned nodes.
		set_parent(node.get(), virtual_root.get(), ParentType::IMPLICIT_PARENT);
	    }
	    else
	    {
		auto pair = pool.find(parent_uuid);
		if (pair == pool.end())
		{
		    string error =
			sformat(_("Parent node %s not found."), parent_uuid.c_str());
		    SN_THROW(Exception(error));
		}
		else
		{
		    set_parent(node.get(), pair->second.get(), ParentType::DIRECT_PARENT);
		}
	    }
	}
    }

    boost::optional<TreeView::SearchResult>
    TreeView::find_nearest_valid_node(const string& start_uuid) const
    {
	auto pair = pool.find(start_uuid);
	if (pair != pool.end())
	{
	    return find_nearest_valid_node(pair->second.get());
	}

	SN_THROW(Exception(
	    sformat(_("Cannot find the node of UUID %s"), start_uuid.c_str())));
	__builtin_unreachable();
    }

    boost::optional<TreeView::SearchResult>
    TreeView::find_nearest_valid_node(const ProxyNode* start_node) const
    {
	queue<SearchResult> nodes_to_visit;
	unordered_set<string> visited;

	nodes_to_visit.push(SearchResult(start_node, 0));
	visited.insert(start_node->get_uuid());

	while (!nodes_to_visit.empty())
	{
	    SearchResult current = nodes_to_visit.front();
	    nodes_to_visit.pop();

	    // Return the current search result if the distance is > 0
	    // (i.e., not the start node) and it is valid.
	    if (current.distance > 0 && current.node->is_valid())
	    {
		return current;
	    }

	    // Append the parent and child nodes to the search queue.
	    vector<const ProxyNode*> new_candidates = current.node->children;
	    if (const ProxyNode* tmp_node = current.node->parent)
	    {
		new_candidates.insert(new_candidates.begin(), tmp_node);
	    }

	    for (const ProxyNode* node : new_candidates)
	    {
		if (!visited.count(node->get_uuid()))
		{
		    visited.insert(node->get_uuid());
		    nodes_to_visit.push(SearchResult(node, current.distance + 1));
		}
	    }
	}

	return boost::none;
    }

    void TreeView::set_parent(ProxyNode* node, ProxyNode* parent, ParentType parent_type)
    {
	parent->children.push_back(node);
	node->parent = parent;
	node->parent_type = parent_type;
    }

    void TreeView::print_graph_graphviz(const ProxyNode* node, const Rankdir& rankdir)
    {
	cout << "digraph {" << '\n';
	cout << sformat(_("    rankdir=\"%s\"\n"), toString(rankdir).c_str());
	print_graph_graphviz_recursive(node);
	cout << "}" << '\n';
    }

    void TreeView::print_graph_graphviz(const Rankdir& rankdir) const
    {
	TreeView::print_graph_graphviz(virtual_root.get(), rankdir);
    }


    const vector<string> EnumInfo<TreeView::ParentType>::names({
	"none", "direct-parent", "implicit-parent"
    });

    const vector<string> EnumInfo<TreeView::Rankdir>::names({
        "TB",
        "LR",
        "BT",
        "RL",
    });

}
