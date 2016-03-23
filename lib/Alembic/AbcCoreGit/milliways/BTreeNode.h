/*
 * milliways - B+ trees and key-value store C++ library
 *
 * Author: Marco Pantaleoni <marco.pantaleoni@gmail.com>
 * Copyright (C) 2016 Marco Pantaleoni. All rights reserved.
 *
 * Distributed under the Apache License, Version 2.0
 * See the NOTICE file distributed with this work for
 * additional information regarding copyright ownership.
 * The author licenses this file to you under the Apache
 * License, Version 2.0 (the "License"); you may not use
 * this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef MILLIWAYS_BTREENODE_H
#define MILLIWAYS_BTREENODE_H

#include <iostream>
#include <fstream>
#include <string>
#include <functional>
#include <array>

#include <stdint.h>
#include <assert.h>

#include "BlockStorage.h"
#include "BTreeCommon.h"

namespace milliways {

template < int B_, typename KeyTraits, typename TTraits, class Compare = std::less<typename KeyTraits::type> >
class BTreeLookup
{
public:
	static const int B = B_;

	typedef KeyTraits key_traits_type;
	typedef TTraits mapped_traits_type;
	typedef typename KeyTraits::type key_type;
	typedef typename TTraits::type mapped_type;
	typedef std::pair<key_type, mapped_type> value_type;
	typedef Compare key_compare;
	typedef value_type& reference;
	typedef value_type* pointer;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;

	typedef BTree<B_, KeyTraits, TTraits, Compare> tree_type;
	typedef BTreeNode<B_, KeyTraits, TTraits, Compare> node_type;

	BTreeLookup() :
		m_node_ptr(NULL), m_found(false), m_pos(-1) {}
	BTreeLookup(node_type* node, bool found_, int pos_, const key_type& key_) :
		m_node_ptr(node), m_found(found_), m_pos(pos_), m_key(key_) {}
	BTreeLookup(const BTreeLookup& other) :
			m_node_ptr(other.m_node_ptr), m_found(other.m_found), m_pos(other.m_pos), m_key(other.m_key) {}
	BTreeLookup& operator= (const BTreeLookup& other) { m_node_ptr = other.m_node_ptr; m_found = other.m_found; m_pos = other.m_pos; m_key = other.m_key; return *this; }

	bool operator== (const BTreeLookup& rhs) { return (m_node_ptr == rhs.m_node_ptr) && (m_found == rhs.m_found) && (m_pos == rhs.m_pos); }
	bool operator!= (const BTreeLookup& rhs) { return (m_node_ptr != rhs.m_node_ptr) || (m_found != rhs.m_found) || (m_pos != rhs.m_pos); }
	operator bool() const { return m_node_ptr && (node_id_valid(m_node_ptr->id())) && m_found && (m_pos >= 0) && (m_pos < m_node_ptr->n()); }

	bool found() const { return m_found; }
	BTreeLookup& found(bool value) { m_found = value; return *this; }

	const key_type& key() const { return m_key; }
	BTreeLookup& key(const key_type& key_) { m_key = key_; return *this; }

	node_type* node() const { return m_node_ptr; }
	BTreeLookup& node(node_type* node_) { m_node_ptr = node_; return *this; }

	int pos() const { return m_pos; }
	BTreeLookup& pos(int pos_) { m_pos = pos_; return *this; }

	node_id_t nodeId() const { return m_node_ptr ? m_node_ptr->id() : NODE_ID_INVALID; }

private:
	node_type* m_node_ptr;
	bool m_found;
	int m_pos;
	key_type m_key;
};

template < int B_, typename KeyTraits, typename TTraits, class Compare = std::less<typename KeyTraits::type> >
inline std::ostream& operator<< ( std::ostream& out, const BTreeLookup<B_, KeyTraits, TTraits, Compare>& value )
{
	node_id_t node_id = value.nodeId();
	out << "<Lookup " << (value.found() ? "found" : "NOT-found") << " key:" << value.key() << " node:" << (int)(node_id_valid(node_id) ? node_id : -1) << " pos:" << value.pos() << ">";
	return out;
}

template < int B_, typename KeyTraits, typename TTraits, class Compare = std::less<typename KeyTraits::type> >
class BTreeNode
{
public:
	static const int B = B_;

	typedef KeyTraits key_traits_type;
	typedef TTraits mapped_traits_type;
	typedef typename KeyTraits::type key_type;
	typedef typename TTraits::type mapped_type;
	typedef std::pair<key_type, mapped_type> value_type;
	typedef Compare key_compare;
	typedef value_type& reference;
	typedef value_type* pointer;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;

	typedef BTree<B_, KeyTraits, TTraits, Compare> tree_type;
	typedef BTreeNode<B_, KeyTraits, TTraits, Compare> node_type;
	typedef BTreeLookup<B_, KeyTraits, TTraits, Compare> lookup_type;

	typedef std::array<key_type, 2*B - 1> keys_array_type;
	typedef std::array<mapped_type, 2*B - 1> values_array_type;
	typedef std::array<node_id_t, 2*B> children_array_type;

	BTreeNode(tree_type* tree, node_id_t node_id, node_id_t parent_id = NODE_ID_INVALID);
	BTreeNode(const BTreeNode& other);
	~BTreeNode() {}

	BTreeNode& operator= (const BTreeNode& other);

	bool valid() const { return hasId(); }

	bool dirty() const { return m_dirty; }
	bool dirty(bool value) { bool old = m_dirty; m_dirty = value; return old; }

	bool hasId() const { return m_id != NODE_ID_INVALID; }
	node_id_t id() const { return m_id; }
	node_id_t id(node_id_t value) { node_id_t old = m_id; m_id = value; return old; }

	bool hasParent() const { return m_parent_id != NODE_ID_INVALID; }
	node_id_t parentId() const { return m_parent_id; }
	node_id_t parentId(node_id_t value) { node_id_t old = m_parent_id; m_parent_id = value; return old; }
	BTreeNode* parent() { if (hasParent()) return m_tree->node_get(m_parent_id); else return NULL; }
	void parent(node_type* node) { assert(node); assert(node->id() != NODE_ID_INVALID); m_parent_id = node->id(); }

	bool hasLeft() const { return m_left_id != NODE_ID_INVALID; }
	node_id_t leftId() const { return m_left_id; }
	node_id_t leftId(node_id_t value) { node_id_t old = m_left_id; m_left_id = value; return old; }
	BTreeNode* left() { if (hasLeft()) return m_tree->node_get(m_left_id); else return NULL; }
	void left(node_type* node) { assert(node); assert(node->id() != NODE_ID_INVALID); m_left_id = node->id(); }

	bool hasRight() const { return m_right_id != NODE_ID_INVALID; }
	node_id_t rightId() const { return m_right_id; }
	node_id_t rightId(node_id_t value) { node_id_t old = m_right_id; m_right_id = value; return old; }
	BTreeNode* right() { if (hasRight()) return m_tree->node_get(m_right_id); else return NULL; }
	void right(node_type* node) { assert(node); assert(node->id() != NODE_ID_INVALID); m_right_id = node->id(); }

	bool leaf() const { return m_leaf; }
	bool leaf(bool value) { bool old = m_leaf; m_leaf = value; return old; }

	int n() const { return m_n; }
	int n(int value) { int old = m_n; m_n = value; return old; }

	int rank() const { return m_rank; }
	int rank(int value) { int old = m_rank; m_rank = value; return old; }

	bool empty() const { return (m_n == 0); }
	bool nonEmpty() const { return !empty(); }
	bool full() const { return (m_n == (2 * B - 1)); }
	bool nonFull() const { return !full(); }

	keys_array_type& keys() { return m_keys; }
	const keys_array_type& keys() const { return m_keys; }
	values_array_type& values() { return m_values; }
	const values_array_type& values() const { return m_values; }
	children_array_type& children() { return m_children; }
	const children_array_type& children() const { return m_children; }

	key_type& key(int i) { return m_keys[i]; }
	const key_type& key(int i) const { return m_keys[i]; }

	mapped_type& value(int i) { return m_values[i]; }
	const mapped_type& value(int i) const { return m_values[i]; }

	node_id_t& child(int i) { return m_children[i]; }
	const node_id_t& child(int i) const { return m_children[i]; }
	bool hasChild(int i) const { return ((! leaf()) && (i >= 0) && (i <= n()) && node_id_valid(m_children[i])); }

	node_type* child_node(int i) { node_id_t node_id = child(i); return (node_id != NODE_ID_INVALID) ? node_get(node_id) : NULL; }
	const node_type* child_node(int i) const { node_id_t node_id = child(i); return (node_id != NODE_ID_INVALID) ? node_get(node_id) : NULL; }
	void child_node(int i, node_type* node) { assert(node); assert(node->id() != NODE_ID_INVALID); m_children[i] = node->id(); }

	lookup_type* create_lookup(bool found, int pos, const key_type& key) { return new lookup_type(this, found, pos, key); }

	bool search(lookup_type& res, const key_type& key_);
	void truncate(int num);
	bool bsearch(lookup_type& res, const key_type& key_);
	void split_child(int i);
	node_type* insert_non_full(const key_type& key_, const mapped_type& value_);
	bool remove(lookup_type& res, const key_type& key_);

	/* -- Node I/O ------------------------------------------------- */

	node_type* node_alloc() { assert(m_tree); return m_tree->node_alloc(); }
	node_type* node_child_alloc(node_type* parent = NULL) { assert(m_tree); return m_tree->node_child_alloc(parent); }
	void node_dispose(node_type* node) { assert(m_tree); return m_tree->node_dispose(node); }
	node_type* node_get(node_id_t node_id) { assert(m_tree); return m_tree->node_get(node_id); }
	node_type* node_get(node_type* node) { assert(m_tree); return m_tree->node_get(node); }
	node_type* node_put(node_type* node) { assert(m_tree); return m_tree->node_put(node); }

	BTreeNode* child_alloc() { return node_child_alloc(this); }

	/* -- Output --------------------------------------------------- */

	std::ostream& dotGraph(std::ostream& out);
	void dotGraph(const std::string& basename, bool display = false);
	std::ostream& dotGraph(std::ostream& out, std::map<node_id_t, bool>& visitedMap, int level = 0, int indent_ = 1);

private:
	tree_type* m_tree;
	node_id_t m_id;
	node_id_t m_parent_id;
	node_id_t m_left_id;
	node_id_t m_right_id;
	bool m_leaf;
	int m_n;
	int m_rank;
	bool m_dirty;

	keys_array_type m_keys;
	values_array_type m_values;
	children_array_type m_children;
};

} /* end of namespace milliways */

#include "BTreeNode.impl.hpp"

#endif /* MILLIWAYS_BTREENODE_H */
