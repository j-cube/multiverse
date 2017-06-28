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
#include "Utils.h"

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
	typedef ITYPENAME KeyTraits::type key_type;
	typedef ITYPENAME TTraits::type mapped_type;
	typedef std::pair<key_type, mapped_type> value_type;
	typedef Compare key_compare;
	typedef size_t size_type;

	typedef BTree<B_, KeyTraits, TTraits, Compare> tree_type;
	typedef BTreeNode<B_, KeyTraits, TTraits, Compare> node_type;
	typedef BTreeLookup<B_, KeyTraits, TTraits, Compare> lookup_type;
	typedef BTreeStorage<B_, KeyTraits, TTraits, Compare> storage_type;
	typedef BTreeMemoryStorage<B_, KeyTraits, TTraits, Compare> memory_storage_type;
	typedef BTreeNodeManager<B_, KeyTraits, TTraits, Compare> manager_type;

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

	/* -- Node Manager --------------------------------------------- */

	manager_type& manager() { assert(m_io); return m_io->manager(); }
	MW_SHPTR<node_type> node(node_id_t node_id) { return manager().get_object(node_id); }

	/* -- Root ----------------------------------------------------- */

	bool hasRoot() const { assert(m_io); return m_io->hasRoot(); }
	node_id_t rootId() const { assert(m_io); return m_io->rootId(); }
	node_id_t rootId(node_id_t value) { assert(m_io); return m_io->rootId(value); }
	MW_SHPTR<node_type> root(bool create = true) { assert(m_io); return m_io->root(create); }
	void root(const MW_SHPTR<node_type>& new_root) { assert(new_root); assert(node_id_valid(new_root->id())); rootId(new_root->id()); }

	/* -- Misc ----------------------------------------------------- */

	size_type size() const { assert(m_io); return m_io->size(); }

	/* -- Operations ----------------------------------------------- */

	MW_SHPTR<node_type> insert(const key_type& key_, const mapped_type& value_);
	MW_SHPTR<node_type> update(const key_type& key_, const mapped_type& value_);
	bool search(lookup_type& res, const key_type& key_);
	bool remove(lookup_type& res, const key_type& key_);
	iterator find(const key_type& key_);

	/* -- Node I/O ------------------------------------------------- */

	MW_SHPTR<node_type> node_alloc() { assert(m_io); return m_io->node_alloc(); }
	MW_SHPTR<node_type> node_child_alloc(MW_SHPTR<node_type> parent) { assert(m_io); return m_io->node_child_alloc(parent); }
	void node_dispose(MW_SHPTR<node_type>& node_) { assert(m_io); return m_io->node_dispose(node_); }
	MW_SHPTR<node_type> node_read(node_id_t node_id) { assert(m_io); return m_io->node_read(node_id); }
	MW_SHPTR<node_type> node_read(MW_SHPTR<node_type>& node_) { assert(m_io); return m_io->node_read(node_); }
	MW_SHPTR<node_type> node_write(MW_SHPTR<node_type>& node_) { assert(m_io); return m_io->node_write(node_); }

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
		typedef ITYPENAME tree_type::lookup_type value_type;
		typedef value_type& reference;
		typedef value_type* pointer;
		typedef std::forward_iterator_tag iterator_category;
		typedef int difference_type;

		iterator(): m_tree(NULL), m_root(), m_first_node(), m_forward(true), m_end(true), m_current() { update_current(/* strict_found */ false); }
		iterator(bool end_): m_tree(NULL), m_root(), m_first_node(), m_forward(true), m_end(end_), m_current() { update_current(/* strict_found */ false); }
		iterator(tree_type* tree_, const MW_SHPTR<node_type>& root_, bool forward_ = true, bool end_ = false) : m_tree(tree_), m_root(root_), m_first_node(), m_forward(forward_), m_end(end_), m_current() { rewind(end_); }
		iterator(const iterator& other) : m_tree(other.m_tree), m_root(other.m_root), m_first_node(other.m_first_node), m_forward(other.m_forward), m_end(other.m_end), m_current(other.m_current) { }
		iterator& operator= (const iterator& other) { m_tree = other.m_tree; m_root = other.m_root; m_first_node = other.m_first_node; m_forward = other.m_forward; m_end = other.m_end; m_current = other.m_current; return *this; }

		self_type& operator++() { next(); return *this; }									/* prefix  */
		self_type operator++(int) { self_type i = *this; next(); return i; }				/* postfix */
		self_type& operator--() { prev(); return *this; }
		self_type operator--(int) { self_type i = *this; prev(); return i; }
		reference operator*() { return m_current; }
		pointer operator->() { return &m_current; }
		bool operator== (const self_type& rhs) {
			return (m_tree == rhs.m_tree) && (m_root == rhs.m_root) &&
					((m_end && rhs.m_end) ||
					 ((m_first_node == rhs.m_first_node) && (m_forward == rhs.m_forward) && (m_end == rhs.m_end) && (m_current == rhs.m_current))); }
		bool operator!= (const self_type& rhs) { return (! (*this == rhs)); }

		operator bool() const { return (! end()) && m_current; }

		self_type& rewind(bool end_);
		MW_SHPTR<node_type> down(const MW_SHPTR<node_type>& node, bool right = false);
		bool next();
		bool prev();

		tree_type* tree() const { return m_tree; }
		MW_SHPTR<node_type> root() const { return m_root; }
		MW_SHPTR<node_type> first() const { return m_first_node; }
		const value_type& current() const { return m_current; }
		node_id_t current_node_id() const { return m_current.nodeId(); }
		MW_SHPTR<node_type> current_node() const { return m_current.node(); }
		int current_pos() const { return m_current.pos(); }
		bool forward() const { return m_forward; }
		bool backward() const { return (! m_forward); }
		bool end() const { return m_end; }

	protected:
		void update_current(bool strict_found = true);
		void set_current(const value_type& at);			/* needed only by BTree::find(const key_type& key_) */

	private:
		tree_type* m_tree;
		MW_SHPTR<node_type> m_root;
		MW_SHPTR<node_type> m_first_node;
		bool m_forward;
		bool m_end;
		value_type m_current;
		friend class const_iterator;
		friend class BTree<B_, KeyTraits, TTraits, Compare>;
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
	typedef ITYPENAME KeyTraits::type key_type;
	typedef ITYPENAME TTraits::type mapped_type;
	typedef std::pair<key_type, mapped_type> value_type;
	typedef Compare key_compare;
	typedef size_t size_type;

	typedef BTree<B_, KeyTraits, TTraits, Compare> tree_type;
	typedef BTreeNode<B_, KeyTraits, TTraits, Compare> node_type;
	typedef BTreeNodeManager<B_, KeyTraits, TTraits, Compare> manager_type;
	typedef BTreeStorage<B_, KeyTraits, TTraits, Compare> self_type;

	static const int B = B_;

	BTreeStorage() :
			m_tree(NULL), m_root_id(NODE_ID_INVALID), m_n_nodes(0), m_created(false), m_manager() {}
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
		m_manager.tree(m_tree);
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
		m_tree->m_io_allocated = false;
		m_tree = NULL;

		m_root_id = NODE_ID_INVALID;
		m_n_nodes = 0;
		m_manager.tree(NULL);
		return true;
	}

	/* -- Root ----------------------------------------------------- */

	bool hasRoot() const { return m_root_id != NODE_ID_INVALID; }
	node_id_t rootId() const { return m_root_id; }
	node_id_t rootId(node_id_t value) { node_id_t old = m_root_id; m_root_id = value; return old; }
	MW_SHPTR<node_type> root(bool create = true) { if (hasRoot()) return node_read(m_root_id); else return (create ? node_alloc() : MW_SHPTR<node_type>()); }
	void root(const MW_SHPTR<node_type>& new_root) { assert(new_root); assert(node_id_valid(new_root->id())); rootId(new_root->id()); }

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

	virtual bool ll_node_read(node_type& node) = 0;
	virtual bool ll_node_write(node_type& node) = 0;

	/* -- Node I/O - high level (cached) --------------------------- */

	virtual MW_SHPTR<node_type> node_alloc() { return node_child_alloc(MW_SHPTR<node_type>()); }
	virtual MW_SHPTR<node_type> node_child_alloc(MW_SHPTR<node_type> parent)
	{
		node_id_t node_id = node_alloc_id();
		MW_SHPTR<node_type> child( node_alloc(node_id) );		// node_alloc(node_id_t) also puts into cache (is equal to alloc + node_write)
		assert(child);
		m_n_nodes++;
		if (! hasRoot())
			m_root_id = node_id;
		if (parent)
		{
			child->parentId(parent->id());
			parent->leaf(false);
			child->rank(parent->rank() + 1);
			node_write(parent);
			node_write(child);
		}
		assert(child->leaf());
		// node_write(child);
		return child;
	}

	virtual void node_dispose(MW_SHPTR<node_type>& node_)
	{
		assert(node_);
		node_dispose_id(node_->id());
		node_dealloc(node_);
	}

	virtual MW_SHPTR<node_type> node_alloc(node_id_t node_id) = 0;		// this must also perform a node_write() (put into cache)
	virtual void node_dealloc(MW_SHPTR<node_type>& node_)
	{
		if (node_) {
			if (this->rootId() == node_->id())
				this->rootId(NODE_ID_INVALID);
			node_->id(NODE_ID_INVALID);
			// delete node_;
		}
	}
	virtual MW_SHPTR<node_type> node_read(node_id_t node_id) = 0;
	virtual MW_SHPTR<node_type> node_read(MW_SHPTR<node_type>& node_)
	{
		assert(node_);
		assert(node_->id() != NODE_ID_INVALID);
		MW_SHPTR<node_type> src( node_read(node_->id()) );
		if (src)
		{
			assert(src->id() == node_->id());
			*node_ = *src;
			return node_;
		}
		return MW_SHPTR<node_type>();
	}
	virtual MW_SHPTR<node_type> node_write(MW_SHPTR<node_type>& node_) = 0;

	/* -- Header I/O ----------------------------------------------- */

	virtual bool header_write() { return true; }
	virtual bool header_read()  { return true; }

	/* -- Node Manager --------------------------------------------- */

	manager_type& manager() { return m_manager; }
	MW_SHPTR<node_type> node(node_id_t node_id) { return manager().get_object(node_id); }
	MW_SHPTR<node_type> null_node() { return manager().null_object(); }

protected:
	tree_type* m_tree;
	node_id_t m_root_id;
	size_type m_n_nodes;
	bool m_created;
	manager_type m_manager;

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
	typedef ITYPENAME KeyTraits::type key_type;
	typedef ITYPENAME TTraits::type mapped_type;
	typedef std::pair<key_type, mapped_type> value_type;
	typedef Compare key_compare;
	typedef size_t size_type;

	typedef BTree<B_, KeyTraits, TTraits, Compare> tree_type;
	typedef BTreeNode<B_, KeyTraits, TTraits, Compare> node_type;
	typedef BTreeStorage<B_, KeyTraits, TTraits, Compare> base_type;
	typedef std::map< node_id_t, MW_SHPTR<node_type> > node_map_type;

	static const int B = B_;

	BTreeMemoryStorage() :
		BTreeStorage<B_, KeyTraits, TTraits, Compare>(), m_next_id(1), m_nodes() {}
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

	bool ll_node_read(node_type& node_)
	{
		assert(node_.id() != NODE_ID_INVALID);
		if (m_nodes.count(node_.id()) > 0) {
			MW_SHPTR<node_type> node_ptr( m_nodes[node_.id()] );
			assert(node_ptr);
			if (node_ptr.get() != &node_)
				node_ = *node_ptr;
			return true;
		}
		node_.dirty(true);
		return false;
	}
	bool ll_node_write(node_type& node_)
	{
		assert(node_.id() != NODE_ID_INVALID);
		if (m_nodes.count(node_.id()) > 0)
		{
			MW_SHPTR<node_type> node_ptr( m_nodes[node_.id()] );
			assert(node_ptr);
			if (node_ptr.get() != &node_)
				*node_ptr = node_;
		} else
		{
			// node_ptr.reset( new node_type(this->tree(), node_.id()) );
			MW_SHPTR<node_type> node_ptr( this->manager().get_object(node_.id()) );
			assert(node_ptr && (node_ptr->id() == node_.id()));
			*node_ptr = node_;
			m_nodes[node_.id()] = node_ptr;
		}
		return true;
	}

	/* -- Node I/O - high level (cached) --------------------------- */

	MW_SHPTR<node_type> node_alloc(node_id_t node_id)
	{
		// this must also perform a node_write() (put into cache)
		assert(node_id != NODE_ID_INVALID);
		// MW_SHPTR<node_type> node_ptr( new node_type(this->tree(), node_id) );
		MW_SHPTR<node_type> node_ptr( this->manager().get_object(node_id) );
		assert(node_ptr && (node_ptr->id() == node_id));
		assert(! node_ptr->dirty());
		m_nodes[node_id] = node_ptr;
		return node_ptr;
	}

	void node_dealloc(MW_SHPTR<node_type>& node_)
	{
		if (node_ && (node_->id() != NODE_ID_INVALID))
			m_nodes.erase(node_->id());
		base_type::node_dealloc(node_);
	}

	MW_SHPTR<node_type> node_read(node_id_t node_id)
	{
		typename node_map_type::const_iterator it = m_nodes.find(node_id);
		if (it != m_nodes.end())
		{
			assert(node_id == it->first);
			return it->second;
		}
		return MW_SHPTR<node_type>();
	}

	MW_SHPTR<node_type> node_write(MW_SHPTR<node_type>& node_)
	{
		assert(node_);
		assert(node_->id() != NODE_ID_INVALID);
		MW_SHPTR<node_type> node_ptr( m_nodes[node_->id()] );
		if (! node_ptr)
		{
			m_nodes[node_->id()] = node_;
			node_ptr = node_;
		} else if (node_ptr != node_)
			*node_ptr = *node_;
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
