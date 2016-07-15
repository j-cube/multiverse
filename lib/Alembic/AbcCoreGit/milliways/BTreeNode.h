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

#include "config.h"

#include <iostream>
#include <fstream>
#include <string>
#include <functional>
#if defined(USE_STD_ARRAY)
#include <array>
#elif defined(USE_TR1_ARRAY)
#include <tr1/array>
#elif defined(USE_BOOST_ARRAY)
#include <boost/array.hpp>
#endif
#include <map>
#if defined(USE_STD_UNORDERED_MAP)
#include <unordered_map>
#elif defined(USE_TR1_UNORDERED_MAP)
#include <tr1/unordered_map>
#elif defined(USE_BOOST_UNORDERED_MAP)
#include <boost/unordered_map.hpp>
#endif
#if defined(USE_STD_MEMORY)
#include <memory>
#elif defined(USE_TR1_MEMORY)
#include <tr1/memory>
#elif defined(USE_BOOST_MEMORY)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

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
	typedef ITYPENAME KeyTraits::type key_type;
	typedef ITYPENAME TTraits::type mapped_type;
	typedef std::pair<key_type, mapped_type> value_type;
	typedef Compare key_compare;
	typedef value_type& reference;
	typedef value_type* pointer;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;

	typedef BTree<B_, KeyTraits, TTraits, Compare> tree_type;
	typedef BTreeNode<B_, KeyTraits, TTraits, Compare> node_type;

	BTreeLookup() :
		m_node_ptr(), m_found(false), m_pos(-1), m_key() {}
	BTreeLookup(MW_SHPTR<node_type>& node_, bool found_, int pos_, const key_type& key_) :
		m_node_ptr(node_), m_found(found_), m_pos(pos_), m_key(key_) {}
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

	MW_SHPTR<node_type> node() const { return m_node_ptr; }
	BTreeLookup& node(const MW_SHPTR<node_type>& node_) { m_node_ptr = node_; return *this; }
	BTreeLookup& nodeReset() { m_node_ptr.reset(); return *this; }

	int pos() const { return m_pos; }
	BTreeLookup& pos(int pos_) { m_pos = pos_; return *this; }

	node_id_t nodeId() const { return m_node_ptr ? m_node_ptr->id() : NODE_ID_INVALID; }

private:
	MW_SHPTR<node_type> m_node_ptr;
	bool m_found;
	int m_pos;
	key_type m_key;
};

template < int B_, typename KeyTraits, typename TTraits, class Compare >
inline std::ostream& operator<< ( std::ostream& out, const BTreeLookup<B_, KeyTraits, TTraits, Compare>& value )
{
	node_id_t node_id = value.nodeId();
	out << "<Lookup " << (value.found() ? "found" : "NOT-found") << " key:" << value.key() << " node:" << (int)(node_id_valid(node_id) ? (int)node_id : -1) << " pos:" << value.pos() << ">";
	return out;
}

template < int B_, typename KeyTraits, typename TTraits, class Compare = std::less<typename KeyTraits::type> >
class BTreeNode
{
public:
	static const int B = B_;

	typedef KeyTraits key_traits_type;
	typedef TTraits mapped_traits_type;
	typedef ITYPENAME KeyTraits::type key_type;
	typedef ITYPENAME TTraits::type mapped_type;
	typedef std::pair<key_type, mapped_type> value_type;
	typedef Compare key_compare;
	typedef value_type& reference;
	typedef value_type* pointer;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;

	typedef BTree<B_, KeyTraits, TTraits, Compare> tree_type;
	typedef BTreeNode<B_, KeyTraits, TTraits, Compare> node_type;
	typedef BTreeLookup<B_, KeyTraits, TTraits, Compare> lookup_type;

	typedef cxx_a::array<key_type, 2*B - 1> keys_array_type;
	typedef cxx_a::array<mapped_type, 2*B - 1> values_array_type;
	typedef cxx_a::array<node_id_t, 2*B> children_array_type;

	BTreeNode& operator= (const BTreeNode& other);

	bool valid() const { return hasId(); }

	tree_type* tree() { return m_tree; }
	const tree_type* tree() const { return m_tree; }

	bool dirty() const { return m_dirty; }
	bool dirty(bool value_) { bool old = m_dirty; m_dirty = value_; return old; }

	bool hasId() const { return m_id != NODE_ID_INVALID; }
	node_id_t id() const { return m_id; }
	node_id_t id(node_id_t value_) { node_id_t old = m_id; m_id = value_; return old; }

	bool hasParent() const { return m_parent_id != NODE_ID_INVALID; }
	node_id_t parentId() const { return m_parent_id; }
	node_id_t parentId(node_id_t value_) { node_id_t old = m_parent_id; m_parent_id = value_; return old; }
	MW_SHPTR<BTreeNode> parent() { if (hasParent()) return m_tree->node_read(m_parent_id); else return MW_SHPTR<BTreeNode>(); }
	void parent(const MW_SHPTR<node_type>& node) { assert(node); assert(node->id() != NODE_ID_INVALID); m_parent_id = node->id(); }

	bool hasLeft() const { return m_left_id != NODE_ID_INVALID; }
	node_id_t leftId() const { return m_left_id; }
	node_id_t leftId(node_id_t value_) { node_id_t old = m_left_id; m_left_id = value_; return old; }
	MW_SHPTR<BTreeNode> left() { if (hasLeft()) return m_tree->node_read(m_left_id); else return MW_SHPTR<BTreeNode>(); }
	void left(const MW_SHPTR<node_type>& node) { assert(node); assert(node->id() != NODE_ID_INVALID); m_left_id = node->id(); }

	bool hasRight() const { return m_right_id != NODE_ID_INVALID; }
	node_id_t rightId() const { return m_right_id; }
	node_id_t rightId(node_id_t value_) { node_id_t old = m_right_id; m_right_id = value_; return old; }
	MW_SHPTR<BTreeNode> right() { if (hasRight()) return m_tree->node_read(m_right_id); else return MW_SHPTR<BTreeNode>(); }
	void right(const MW_SHPTR<node_type>& node) { assert(node); assert(node->id() != NODE_ID_INVALID); m_right_id = node->id(); }

	bool leaf() const { return m_leaf; }
	bool leaf(bool value_) { bool old = m_leaf; m_leaf = value_; return old; }

	int n() const { return m_n; }
	int n(int value_) { int old = m_n; m_n = value_; return old; }

	int rank() const { return m_rank; }
	int rank(int value_) { int old = m_rank; m_rank = value_; return old; }

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

	key_type& key(int i) { return m_keys[static_cast<size_type>(i)]; }
	const key_type& key(int i) const { return m_keys[static_cast<size_type>(i)]; }

	mapped_type& value(int i) { return m_values[static_cast<size_type>(i)]; }
	const mapped_type& value(int i) const { return m_values[static_cast<size_type>(i)]; }

	node_id_t& child(int i) { return m_children[static_cast<size_type>(i)]; }
	const node_id_t& child(int i) const { return m_children[static_cast<size_type>(i)]; }
	bool hasChild(int i) const { return ((! leaf()) && (i >= 0) && (i <= n()) && node_id_valid(m_children[static_cast<size_type>(i)])); }

	MW_SHPTR<node_type> child_node(int i) const { node_id_t node_id = child(i); return (node_id != NODE_ID_INVALID) ? node_read(node_id) : MW_SHPTR<node_type>(); }
	void child_node(int i, const MW_SHPTR<node_type>& node) { assert(node); assert(node->id() != NODE_ID_INVALID); m_children[i] = node->id(); }

	lookup_type* create_lookup(bool found, int pos, const key_type& key_) { return new lookup_type(self_read(), found, pos, key_); }

	bool search(lookup_type& res, const key_type& key_);
	void truncate(int num);
	bool bsearch(lookup_type& res, const key_type& key_);
	void split_child(int i);
	MW_SHPTR<node_type> insert_non_full(const key_type& key_, const mapped_type& value_);
	bool remove(lookup_type& res, const key_type& key_);

	/* -- Node I/O ------------------------------------------------- */

	MW_SHPTR<node_type> self() { assert(m_tree); assert(node_id_valid(id())); return m_tree->node(id()); }
	MW_SHPTR<node_type> self_read() const { assert(m_tree); assert(node_id_valid(id())); return m_tree->node_read(id()); }

	MW_SHPTR<node_type> node_alloc() { assert(m_tree); return m_tree->node_alloc(); }
	MW_SHPTR<node_type> node_child_alloc(MW_SHPTR<node_type> parent_) { assert(m_tree); return m_tree->node_child_alloc(parent_); }
	void node_dispose(MW_SHPTR<node_type>& node) { assert(m_tree); return m_tree->node_dispose(node); }
	MW_SHPTR<node_type> node_read(node_id_t node_id) const { assert(m_tree); return m_tree->node_read(node_id); }
	MW_SHPTR<node_type> node_read(MW_SHPTR<node_type>& node) const { assert(m_tree); return m_tree->node_read(node); }
	MW_SHPTR<node_type> node_write(MW_SHPTR<node_type>& node) const { assert(m_tree); return m_tree->node_write(node); }

	MW_SHPTR<BTreeNode> child_alloc() { return node_child_alloc(self()); }

	/* -- Output --------------------------------------------------- */

	std::ostream& dotGraph(std::ostream& out);
	void dotGraph(const std::string& basename, bool display = false);
	std::ostream& dotGraph(std::ostream& out, std::map<node_id_t, bool>& visitedMap, int level = 0, int indent_ = 1);

#if defined(COMPILER_SUPPORTS_CXX11)
	/* lifetime managed by BTreeStorage (and we are on C++11) */
private:
	BTreeNode() : m_tree(NULL), m_id(NODE_ID_INVALID), m_parent_id(NODE_ID_INVALID), m_left_id(NODE_ID_INVALID), m_right_id(NODE_ID_INVALID), m_leaf(true), m_n(0), m_rank(0), m_dirty(false), m_keys(), m_values(), m_children() {}
	~BTreeNode() {}
#else
	/* lifetime managed by BTreeStorage but we are on C++03... */
public:
	BTreeNode() : m_tree(NULL), m_id(NODE_ID_INVALID), m_parent_id(NODE_ID_INVALID), m_left_id(NODE_ID_INVALID), m_right_id(NODE_ID_INVALID), m_leaf(true), m_n(0), m_rank(0), m_dirty(false), m_keys(), m_values(), m_children() {}
	~BTreeNode() {}
#endif

private:
	/* lifetime managed by BTreeStorage */
	// BTreeNode() : m_tree(NULL), m_id(NODE_ID_INVALID), m_parent_id(NODE_ID_INVALID), m_left_id(NODE_ID_INVALID), m_right_id(NODE_ID_INVALID), m_leaf(true), m_n(0), m_rank(0), m_dirty(false), m_keys(), m_values(), m_children() {}
	BTreeNode(tree_type* tree, node_id_t node_id, node_id_t parent_id = NODE_ID_INVALID);
	BTreeNode(const BTreeNode& other);
	// ~BTreeNode() {}

	friend class BTreeNodeManager<B_, KeyTraits, TTraits, Compare>;
	friend class BTreeNodeManager<B_, KeyTraits, TTraits, Compare>::node_deleter;

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


/* ----------------------------------------------------------------- *
 *   BTreeNodeManager                                                *
 * ----------------------------------------------------------------- */

/*
 * Based on http://stackoverflow.com/a/15708286
 *   http://stackoverflow.com/questions/15707991/good-design-pattern-for-manager-handler
 */
template < int B_, typename KeyTraits, typename TTraits, class Compare = std::less<typename KeyTraits::type> >
class BTreeNodeManager
{
public:
	typedef BTreeNode<B_, KeyTraits, TTraits, Compare> node_type;
	typedef BTree<B_, KeyTraits, TTraits, Compare> tree_type;
	typedef BTreeNodeManager<B_, KeyTraits, TTraits, Compare> handler_type;

	BTreeNodeManager() : m_objects(), m_tree(NULL) {}
	BTreeNodeManager(tree_type* tree_) : m_objects(), m_tree(tree_) {}
	~BTreeNodeManager() {
		typename cxx_um::unordered_map< node_id_t, cxx_mem::weak_ptr<node_type> >::iterator it;
		for (it = m_objects.begin(); it != m_objects.end(); ++it) {
			cxx_mem::weak_ptr<node_type> wp = it->second;
			if ((wp.use_count() > 0) && wp.expired()) {
				std::cerr << "WARNING: weak pointer expired BUT use count not zero for managed node! (in use:" << wp.use_count() << ")" << std::endl;
			} else if (wp.use_count() > 0) {
				std::cerr << "WARNING: use count not zero for managed node (in use:" << wp.use_count() << ")" << std::endl;
			}
		}
		m_objects.clear();
	}

	handler_type& tree(tree_type* tree_) { m_tree = tree_; return *this; }
	tree_type* tree() { return m_tree; }

	cxx_mem::shared_ptr<node_type> get_object(node_id_t id, bool createIfNotFound = true)
	{
		typename cxx_um::unordered_map< node_id_t, cxx_mem::weak_ptr<node_type> >::const_iterator it = m_objects.find(id);
		if (it != m_objects.end()) {
			assert(it->first == id);
			return it->second.lock();
		} else if (createIfNotFound) {
			return make_object(id);
		} else
			return cxx_mem::shared_ptr<node_type>();
	}

	cxx_mem::shared_ptr<node_type> null_object()
	{
		return cxx_mem::shared_ptr<node_type>();
	}

	bool has(node_type id) {
		typename cxx_um::unordered_map< node_id_t, cxx_mem::weak_ptr<node_type> >::const_iterator it = m_objects.find(id);
		return (it != m_objects.end()) ? true : false;
	}

	size_t count() const { return m_objects.size(); }

private:
	BTreeNodeManager(const BTreeNodeManager& other);
	BTreeNodeManager& operator=(const BTreeNodeManager& other);


	typedef cxx_um::unordered_map< node_id_t, cxx_mem::weak_ptr<node_type> > weak_map_t;

	friend class BTreeNode<B_, KeyTraits, TTraits, Compare>;

	class node_deleter;
	friend class node_deleter;

	cxx_mem::shared_ptr<node_type> make_object(node_id_t id)
	{
		assert(m_tree);
		assert(m_objects.count(id) == 0);
		cxx_mem::shared_ptr<node_type> sp(new node_type(m_tree, id), node_deleter(this, id));

		m_objects[id] = sp;

		return sp;
	}

	/* custom node deleter */
	class node_deleter
	{
	public:
		node_deleter(handler_type* handler, node_id_t id) :
			m_handler(handler), m_id(id) {}

		void operator()(node_type* p) {
			assert(m_handler);
			assert(p);
			assert(p->id() == m_id);
			m_handler->m_objects.erase(m_id);
			delete p;
		}
	private:
		handler_type* m_handler;
		node_id_t m_id;
	};

	cxx_um::unordered_map< node_id_t, cxx_mem::weak_ptr<node_type> > m_objects;
	tree_type* m_tree;
};


} /* end of namespace milliways */

#include "BTreeNode.impl.hpp"

#endif /* MILLIWAYS_BTREENODE_H */
