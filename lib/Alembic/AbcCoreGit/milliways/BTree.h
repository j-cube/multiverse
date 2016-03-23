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

#ifndef MILLIWAYS_BTREE_H
#define MILLIWAYS_BTREE_H

#include <iostream>
#include <fstream>
#include <string>
#include <functional>

#include <stdint.h>
#include <assert.h>

#include "BlockStorage.h"
#include "BTreeCommon.h"

namespace milliways {

/* ----------------------------------------------------------------- *
 *   BTree                                                           *
 * ----------------------------------------------------------------- */

template < int B_, typename KeyTraits, typename TTraits, class Compare = std::less<typename KeyTraits::type> >
class BTree
{
public:
	typedef KeyTraits key_traits_type;
	typedef TTraits mapped_traits_type;
	typedef typename KeyTraits::type key_type;
	typedef typename TTraits::type mapped_type;
	typedef std::pair<key_type, mapped_type> value_type;
	typedef Compare key_compare;
	typedef size_t size_type;

	typedef BTree<B_, KeyTraits, TTraits, Compare> tree_type;
	typedef BTreeNode<B_, KeyTraits, TTraits, Compare> node_type;
	typedef BTreeLookup<B_, KeyTraits, TTraits, Compare> lookup_type;
	typedef BTreeStorage<B_, KeyTraits, TTraits, Compare> storage_type;
	typedef BTreeMemoryStorage<B_, KeyTraits, TTraits, Compare> memory_storage_type;

	static const int B = B_;

	class iterator;
	class const_iterator;

	BTree();
	BTree(storage_type* storage_);
	~BTree();

	/* -- Storage -------------------------------------------------- */

	storage_type* storage() { return m_io; }
	const storage_type* storage() const { return m_io; }
	storage_type* storage(storage_type* value) {
		storage_type* old = m_io;
		if (old == value)
			return old;
		if (old && (old != value))
			old->detach();
		if (m_io_allocated)
		{
			delete old;
			old = NULL;
		}
		m_io_allocated = false;
		assert(! m_io);
		assert(! m_io_allocated);
		if (value) value->attach(this);
		return old;
	}

	/* -- General I/O ---------------------------------------------- */

	bool isOpen() const { return m_io ? m_io->isOpen() : false; }
	bool open() { return m_io ? m_io->open() : false; }
	bool close(){ return m_io ? m_io->close() : false; }
	bool flush() { return m_io ? m_io->flush() : false; }

	/* -- Root ----------------------------------------------------- */

	bool hasRoot() const { assert(m_io); return m_io->hasRoot(); }
	node_id_t rootId() const { assert(m_io); return m_io->rootId(); }
	node_id_t rootId(node_id_t value) { assert(m_io); return m_io->rootId(value); }
	node_type* root(bool create = true) { assert(m_io); return m_io->root(create); }
	void root(node_type* new_root) { assert(new_root); assert(node_id_valid(new_root->id())); rootId(new_root->id()); }

	/* -- Misc ----------------------------------------------------- */

	size_type size() const { assert(m_io); return m_io->size(); }

	/* -- Operations ----------------------------------------------- */

	node_type* insert(const key_type& key_, const mapped_type& value_);
	node_type* update(const key_type& key_, const mapped_type& value_);
	bool search(lookup_type& res, const key_type& key_);
	bool remove(lookup_type& res, const key_type& key_);

	/* -- Node I/O ------------------------------------------------- */

	node_type* node_alloc() { assert(m_io); return m_io->node_alloc(); }
	node_type* node_child_alloc(node_type* parent = NULL) { assert(m_io); return m_io->node_child_alloc(parent); }
	void node_dispose(node_type* node) { assert(m_io); return m_io->node_dispose(node); }
	node_type* node_get(node_id_t node_id) { assert(m_io); return m_io->node_get(node_id); }
	node_type* node_get(node_type* node) { assert(m_io); return m_io->node_get(node); }
	node_type* node_put(node_type* node) { assert(m_io); return m_io->node_put(node); }

	/* -- Output --------------------------------------------------- */

	std::ostream& dotGraph(std::ostream& out) { return root()->dotGraph(out); }
	void dotGraph(const std::string& basename, bool display = false) { return root()->dotGraph(basename, display); }

	/* -- Iteration ------------------------------------------------ */

	iterator begin() { return iterator(this, root()); }
	iterator end() { return iterator(this, root(), /* forward */ true, /* end */ true); }

	iterator rbegin() { return iterator(this, root(), /* forward */ false); }
	iterator rend() { return iterator(this, root(), /* forward */ false, /* end */ true); }

	class iterator
	{
	public:
		typedef iterator self_type;
		typedef typename tree_type::lookup_type value_type;
		typedef value_type& reference;
		typedef value_type* pointer;
		typedef std::forward_iterator_tag iterator_category;
		typedef int difference_type;

		iterator(): m_tree(NULL), m_root(NULL), m_first_node(NULL), m_forward(true), m_end(true) { update_current(); }
		iterator(bool end_): m_tree(NULL), m_root(NULL), m_first_node(NULL), m_forward(true), m_end(end_) { update_current(); }
		iterator(tree_type* tree_, node_type* root_, bool forward_ = true, bool end_ = false) : m_tree(tree_), m_root(root_), m_first_node(NULL), m_forward(forward_), m_end(end_) { rewind(end_); }
		iterator(const iterator& other) : m_tree(other.m_tree), m_root(other.m_root), m_first_node(other.m_first_node), m_forward(other.m_forward), m_end(other.m_end), m_current(other.m_current) { }
		iterator& operator= (const iterator& other) { m_tree = other.m_tree; m_root = other.m_root; m_first_node = other.m_first_node; m_forward = other.m_forward; m_end = other.m_end; m_current = other.m_current; return *this; }

		self_type& operator++() { next(); return *this; }
		self_type& operator++(int junk) { next(); return *this; }
		self_type& operator--() { prev(); return *this; }
		self_type& operator--(int junk) { prev(); return *this; }
		reference operator*() { return m_current; }
		pointer operator->() { return &m_current; }
		bool operator== (const self_type& rhs) {
			return (m_tree == rhs.m_tree) && (m_root == rhs.m_root) &&
					((m_end && rhs.m_end) ||
					 ((m_first_node == rhs.m_first_node) && (m_forward == rhs.m_forward) && (m_end == rhs.m_end) && (m_current == rhs.m_current))); }
		bool operator!= (const self_type& rhs) { return (! (*this == rhs)); }

		operator bool() const { return (! end()) && m_current; }

		self_type& rewind(bool end_);
		node_type* down(node_type* node, bool right = false);
		bool next();
		bool prev();

		tree_type* tree() const { return m_tree; }
		node_type* root() const { return m_root; }
		node_type* first() const { return m_first_node; }
		const value_type& current() const { return m_current; }
		node_id_t current_node_id() const { return m_current.nodeId(); }
		node_type* current_node() const { return m_current.node(); }
		int current_pos() const { return m_current.pos(); }
		bool forward() const { return m_forward; }
		bool backward() const { return (! m_forward); }
		bool end() const { return m_end; }

	protected:
		void update_current();

	private:
		tree_type* m_tree;
		node_type* m_root;
		node_type* m_first_node;
		bool m_forward;
		bool m_end;
		value_type m_current;
		friend class const_iterator;
	};

private:
	BTree(const BTree& other);
	BTree& operator= (const BTree& other);

	storage_type* m_io;
	bool m_io_allocated;

	friend class BTreeStorage<B_, KeyTraits, TTraits, Compare>;
};


/* ----------------------------------------------------------------- *
 *   BTreeStorage                                                    *
 * ----------------------------------------------------------------- */

template < int B_, typename KeyTraits, typename TTraits, class Compare = std::less<typename KeyTraits::type> >
class BTreeStorage
{
public:
	typedef KeyTraits key_traits_type;
	typedef TTraits mapped_traits_type;
	typedef typename KeyTraits::type key_type;
	typedef typename TTraits::type mapped_type;
	typedef std::pair<key_type, mapped_type> value_type;
	typedef Compare key_compare;
	typedef size_t size_type;

	typedef BTree<B_, KeyTraits, TTraits, Compare> tree_type;
	typedef BTreeNode<B_, KeyTraits, TTraits, Compare> node_type;
	typedef BTreeStorage<B_, KeyTraits, TTraits, Compare> self_type;

	static const int B = B_;

	BTreeStorage() :
			m_tree(NULL), m_root_id(NODE_ID_INVALID), m_n_nodes(0), m_created(false) {}
	virtual ~BTreeStorage() { /* call close from the most derived class */ }

	/* -- Tree ----------------------------------------------------- */

	tree_type* tree() { return m_tree; }

	virtual bool attach(tree_type* tree_)
	{
		assert(tree_);

		if (tree_ == m_tree)
			return true;

		assert(! m_tree);
		assert(! isOpen());

		self_type* old_storage = tree_->storage();
		if (old_storage && (old_storage != this))
			old_storage->detach();

		m_tree = tree_;
		m_tree->m_io = this;

		m_root_id = NODE_ID_INVALID;
		m_n_nodes = 0;
		return true;
	}

	virtual bool detach()
	{
		if (! m_tree)
			return false;
		assert(m_tree);

		if (isOpen())
			close();
		assert(! isOpen());

		m_tree->m_io = NULL;
		m_tree = NULL;

		m_root_id = NODE_ID_INVALID;
		m_n_nodes = 0;
		return true;
	}

	/* -- Root ----------------------------------------------------- */

	bool hasRoot() const { return m_root_id != NODE_ID_INVALID; }
	node_id_t rootId() const { return m_root_id; }
	node_id_t rootId(node_id_t value) { node_id_t old = m_root_id; m_root_id = value; return old; }
	node_type* root(bool create = true) { if (hasRoot()) return node_get(m_root_id); else return (create ? node_alloc() : NULL); }
	void root(node_type* new_root) { assert(new_root); assert(node_id_valid(new_root->id())); rootId(new_root->id()); }

	/* -- Misc ----------------------------------------------------- */

	size_type size() const { return m_n_nodes; }

	/* -- General I/O ---------------------------------------------- */

	virtual bool isOpen() const { return false; }
	virtual bool open();
	virtual bool close();
	virtual bool flush() { return true; }

	virtual bool postOpen();
	virtual bool preClose();

	virtual bool openHelper(bool& created_) { created_ = false; return true; }
	virtual bool closeHelper() { return true; }

	bool created() const { return m_created; }

	/* -- Node I/O - low level (direct) ---------------------------- */

	virtual bool has_id(node_id_t node_id) = 0;
	virtual node_id_t node_alloc_id() = 0;
	virtual void node_dispose_id(node_id_t node_id)
	{
		assert(node_id != NODE_ID_INVALID);
		node_dispose_id_helper(node_id);
		if (this->rootId() == node_id)
			this->rootId(NODE_ID_INVALID);
	}
	virtual void node_dispose_id_helper(node_id_t node_id) = 0;

	virtual bool node_read(node_type& node) = 0;
	virtual bool node_write(node_type& node) = 0;

	/* -- Node I/O - hight level (cached) -------------------------- */

	virtual node_type* node_alloc() { return node_child_alloc(); }
	virtual node_type* node_child_alloc(node_type* parent = NULL)
	{
		node_id_t node_id = node_alloc_id();
		node_type* child = node_alloc(node_id);		// node_alloc(node_id_t) also puts into cache (is equal to alloc + node_put)
		assert(child);
		m_n_nodes++;
		if (! hasRoot())
			m_root_id = node_id;
		if (parent)
		{
			child->parentId(parent->id());
			parent->leaf(false);
			child->rank(parent->rank() + 1);
			node_put(parent);
			node_put(child);
		}
		assert(child->leaf());
		// node_put(child);
		return child;
	}

	virtual void node_dispose(node_type* node)
	{
		assert(node);
		node_dispose_id(node->id());
		node_dealloc(node);
	}

	virtual node_type* node_alloc(node_id_t node_id) = 0;		// this must also perform a node_put() (put into cache)
	virtual void node_dealloc(node_type* node)
	{
		if (node) {
			if (this->rootId() == node->id())
				this->rootId(NODE_ID_INVALID);
			node->id(NODE_ID_INVALID);
			// delete node;
		}
	}
	virtual node_type* node_get(node_id_t node_id) = 0;
	virtual node_type* node_get(node_type* node)
	{
		assert(node);
		assert(node->id() != NODE_ID_INVALID);
		node_type* src = node_get(node->id());
		if (src)
		{
			assert(src->id() == node->id());
			*node = *src;
			return node;
		}
		return NULL;
	}
	virtual node_type* node_put(node_type* node) = 0;

	/* -- Header I/O ----------------------------------------------- */

	virtual bool header_write() { return true; }
	virtual bool header_read()  { return true; }

protected:
	tree_type* m_tree;
	node_id_t m_root_id;
	size_type m_n_nodes;
	bool m_created;

	size_type size(size_type n) { size_type old = m_n_nodes; m_n_nodes = n; return old; }

private:
	BTreeStorage(const BTreeStorage& other);
	BTreeStorage& operator= (const BTreeStorage& other);
};

template < int B_, typename KeyTraits, typename TTraits, class Compare = std::less<typename KeyTraits::type> >
class BTreeMemoryStorage : public BTreeStorage<B_, KeyTraits, TTraits, Compare>
{
public:
	typedef KeyTraits key_traits_type;
	typedef TTraits mapped_traits_type;
	typedef typename KeyTraits::type key_type;
	typedef typename TTraits::type mapped_type;
	typedef std::pair<key_type, mapped_type> value_type;
	typedef Compare key_compare;
	typedef size_t size_type;

	typedef BTree<B_, KeyTraits, TTraits, Compare> tree_type;
	typedef BTreeNode<B_, KeyTraits, TTraits, Compare> node_type;
	typedef BTreeStorage<B_, KeyTraits, TTraits, Compare> base_type;
	typedef std::map<int, node_type*> node_map_type;

	static const int B = B_;

	BTreeMemoryStorage() :
		BTreeStorage<B_, KeyTraits, TTraits, Compare>(), m_next_id(1) {}
	~BTreeMemoryStorage() { close(); }

	static BTreeMemoryStorage* createStorage(tree_type* tree_) {
		BTreeMemoryStorage* storage = new BTreeMemoryStorage();
		storage->tree(tree_);
		return storage;
	}

	/* -- General I/O ---------------------------------------------- */

	bool isOpen() const { return this->created(); }
	bool open() { return base_type::open(); }
	bool close() { return base_type::close(); }
	bool flush() { return true; }

	bool openHelper(bool& created_) { created_ = true; m_next_id = 1; m_nodes.clear(); return true; }
	bool closeHelper() { m_next_id = 1; m_nodes.clear(); return true; }

	/* -- Node I/O - low level (direct) ---------------------------- */

	bool has_id(node_id_t node_id) { return m_nodes.count(node_id) ? true : false; }
	node_id_t node_alloc_id() { node_id_t id = m_next_id++; return id; }
	void node_dispose_id_helper(node_id_t node_id)
	{
		assert(node_id != NODE_ID_INVALID);
		if (m_next_id == (node_id + 1))
			m_next_id--;
		if (this->rootId() == node_id)
			this->rootId(NODE_ID_INVALID);
	}

	bool node_read(node_type& node)
	{
		assert(node.id() != NODE_ID_INVALID);
		node_type* node_ptr = m_nodes[node.id()];
		if (node_ptr)
		{
			if (node_ptr != &node)
				node = *node_ptr;
			return true;
		}
		node.dirty(true);
		return false;
	}
	bool node_write(node_type& node)
	{
		assert(node.id() != NODE_ID_INVALID);
		node_type* node_ptr = m_nodes[node.id()];
		if (node_ptr)
		{
			if (node_ptr != &node)
				*node_ptr = node;
		} else
		{
			node_ptr = new node_type(this->tree(), node.id());
			*node_ptr = node;
			m_nodes[node.id()] = node_ptr;
		}
		return true;
	}

	/* -- Node I/O - hight level (cached) -------------------------- */

	node_type* node_alloc(node_id_t node_id)
	{
		// this must also perform a node_put() (put into cache)
		assert(node_id != NODE_ID_INVALID);
		node_type* node_ptr = new node_type(this->tree(), node_id);
		assert(node_ptr && (node_ptr->id() == node_id));
		assert(! node_ptr->dirty());
		m_nodes[node_id] = node_ptr;
		return node_ptr;
	}

	void node_dealloc(node_type* node)
	{
		if (node && (node->id() != NODE_ID_INVALID))
			m_nodes.erase(node->id());
		base_type::node_dealloc(node);
	}

	node_type* node_get(node_id_t node_id)
	{
		typename node_map_type::const_iterator it = m_nodes.find(node_id);
		if (it != m_nodes.end())
		{
			assert(node_id == it->first);
			return it->second;
		}
		return NULL;
	}

	node_type* node_put(node_type* node)
	{
		assert(node);
		assert(node->id() != NODE_ID_INVALID);
		node_type *node_ptr = m_nodes[node->id()];
		if (! node_ptr)
		{
			m_nodes[node->id()] = node;
			node_ptr = node;
		} else if (node_ptr != node)
			*node_ptr = *node;
		return node_ptr;
	}

	/* -- Header I/O ----------------------------------------------- */

	bool header_write() { return true; }
	bool header_read() { return true; }

private:
	BTreeMemoryStorage(const BTreeMemoryStorage& other);
	BTreeMemoryStorage& operator= (const BTreeMemoryStorage& other);

	node_id_t m_next_id;
	node_map_type m_nodes;
};

} /* end of namespace milliways */

#include "BTree.impl.hpp"

#endif /* MILLIWAYS_BTREE_H */
