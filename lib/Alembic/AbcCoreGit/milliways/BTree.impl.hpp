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
#include "BTree.h"
#endif

#ifndef MILLIWAYS_BTREE_IMPL_H
//#define MILLIWAYS_BTREE_IMPL_H

namespace milliways {

/* ----------------------------------------------------------------- *
 *   BTree                                                           *
 * ----------------------------------------------------------------- */

template < int B_, typename KeyTraits, typename TTraits, class Compare >
BTree<B_, KeyTraits, TTraits, Compare>::BTree() :
	m_io(NULL), m_io_allocated(false)
{
	typedef BTreeMemoryStorage<B_, KeyTraits, TTraits, Compare> btree_memst_t;

	btree_memst_t* storage_ = new BTreeMemoryStorage<B_, KeyTraits, TTraits, Compare>();
	storage(storage_);
	m_io_allocated = true;
	assert(m_io == storage_);
	assert(m_io_allocated);
}

template < int B_, typename KeyTraits, typename TTraits, class Compare >
BTree<B_, KeyTraits, TTraits, Compare>::BTree(storage_type* storage_) :
		m_io(NULL), m_io_allocated(false)
{
	m_io_allocated = false;
	storage(storage_);
	assert(m_io == storage_);
	assert(! m_io_allocated);
}

template < int B_, typename KeyTraits, typename TTraits, class Compare >
BTree<B_, KeyTraits, TTraits, Compare>::~BTree()
{
	close();

	if (m_io_allocated && m_io)
	{
		/* storage_type* old = m_io; */
		storage(NULL);
		assert(! m_io);
		assert(! m_io_allocated);
		// don't delete old - deleted by storage(NULL) !
	} else if (m_io)
		storage(NULL);

	assert(! m_io);
	assert(! m_io_allocated);
}

template < int B_, typename KeyTraits, typename TTraits, class Compare >
typename BTree<B_, KeyTraits, TTraits, Compare>::node_type* BTree<B_, KeyTraits, TTraits, Compare>::insert(const key_type& key_, const mapped_type& value_)
{
	node_type* root_ = root();
	assert(root_);
	if (root_->full())
	{
		node_type* old_root = root_;
		node_type* new_root = node_alloc();
		new_root->leaf(false);
		assert(new_root->n() == 0);
		new_root->child(0) = old_root->id();
		new_root->rank(old_root->rank() - 1);
		root(new_root);
		new_root->split_child(0);
		root_ = new_root;
	}
	return root_->insert_non_full(key_, value_);
}

template < int B_, typename KeyTraits, typename TTraits, class Compare >
typename BTree<B_, KeyTraits, TTraits, Compare>::node_type* BTree<B_, KeyTraits, TTraits, Compare>::update(const key_type& key_, const mapped_type& value_)
{
	lookup_type where;
	node_type* root_ = root();
	assert(root_);
	if (! root_->search(where, key_))
		return insert(key_, value_);

	assert(where.found());

	node_type* node_ = where.node();
	assert(node_);
	node_->value(where.pos()) = value_;

	return node_;
}

template < int B_, typename KeyTraits, typename TTraits, class Compare >
bool BTree<B_, KeyTraits, TTraits, Compare>::search(lookup_type& res, const key_type& key_)
{
	node_type* root_ = root();
	assert(root_);
	return root_->search(res, key_);
}

template < int B_, typename KeyTraits, typename TTraits, class Compare >
bool BTree<B_, KeyTraits, TTraits, Compare>::remove(lookup_type& res, const key_type& key_)
{
	node_type* root_ = root();
	assert(root_);
	return root_->remove(res, key_);
}


/* -- BTree::iterator ---------------------------------------------- */

template < int B_, typename KeyTraits, typename TTraits, class Compare >
void BTree<B_, KeyTraits, TTraits, Compare>::iterator::update_current()
{
	if (m_end || (! m_current.node()))
	{
		m_current.found(false).node(NULL).pos(-1);
		m_end = true;
	} else
	{
		assert(m_current.node());
		assert(! m_end);
		node_type* node = m_current.node();
		int pos = m_current.pos();
		m_current.found(true).key(node->key(pos));
	}
}

template < int B_, typename KeyTraits, typename TTraits, class Compare >
typename BTree<B_, KeyTraits, TTraits, Compare>::iterator& BTree<B_, KeyTraits, TTraits, Compare>::iterator::rewind(bool end_)
{
	if (end_ || m_end || (! m_root))
	{
		m_current.found(false).node(NULL).pos(-1);
		m_end = true;
	} else
	{
		assert(m_root);
		assert(! end_);

		m_end = false;
		m_first_node = down(m_root, /* right */ (m_forward ? false : true));
		m_current.node(m_first_node).pos(m_forward ? 0 : (m_first_node->n() - 1));

		assert(! m_end);
	}

	update_current();

	return *this;
}

template < int B_, typename KeyTraits, typename TTraits, class Compare >
typename BTree<B_, KeyTraits, TTraits, Compare>::node_type* BTree<B_, KeyTraits, TTraits, Compare>::iterator::down(node_type* node, bool right)
{
	while (node && (! node->leaf()) && node_id_valid(node->child((right ? node->n() : 0))))
		node = node->child_node((right ? node->n() : 0));
	if (node->leaf())
		return node;
	return NULL;
}

template < int B_, typename KeyTraits, typename TTraits, class Compare >
bool BTree<B_, KeyTraits, TTraits, Compare>::iterator::next()
{
	if (! m_current.node()) {
		update_current();
		return false;             /* stop iteration */
	}

	assert(m_current.node());
	node_type* node = m_current.node();
	int pos = m_current.pos();

	if (m_forward)
	{
		if (++pos >= node->n())
		{
			node = node->right();
			pos = 0;
		}
	} else
	{
		if (--pos < 0)
		{
			node = node->left();
			pos = node ? (node->n() - 1) : 0;
		}
	}
	m_current.pos(pos).node(node);
	update_current();

	return true;
}

template < int B_, typename KeyTraits, typename TTraits, class Compare >
bool BTree<B_, KeyTraits, TTraits, Compare>::iterator::prev()
{
	if (! m_current.node()) {
		update_current();
		return false;             /* stop iteration */
	}

	assert(m_current.node());
	node_type* node = m_current.node();
	int pos = m_current.pos();

	if (m_forward)
	{
		if (--pos < 0)
		{
			node = node->left();
			pos = node ? (node->n() - 1) : 0;
		}
	} else
	{
		if (++pos >= node->n())
		{
			node = node->right();
			pos = 0;
		}
	}
	m_current.pos(pos).node(node);
	update_current();

	return true;
}


/* ----------------------------------------------------------------- *
 *   BTreeStorage                                                    *
 * ----------------------------------------------------------------- */

template < int B_, typename KeyTraits, typename TTraits, class Compare >
bool BTreeStorage<B_, KeyTraits, TTraits, Compare>::open()
{
	if (isOpen())
		return true;
	bool created_ = false;
	if (! openHelper(created_))
		return false;
	m_created = created_;
	return postOpen();
}

template < int B_, typename KeyTraits, typename TTraits, class Compare >
bool BTreeStorage<B_, KeyTraits, TTraits, Compare>::close()
{
	if (! isOpen())
		return true;
	if (! preClose())
		return false;
	return closeHelper();
}

template < int B_, typename KeyTraits, typename TTraits, class Compare >
bool BTreeStorage<B_, KeyTraits, TTraits, Compare>::postOpen()
{
	if (! isOpen())
		return false;
	if (! created())
		return header_read();
	return true;
}

template < int B_, typename KeyTraits, typename TTraits, class Compare >
bool BTreeStorage<B_, KeyTraits, TTraits, Compare>::preClose()
{
	if (! isOpen())
		return false;
	return header_write();
}

} /* end of namespace milliways */

#endif /* MILLIWAYS_BTREE_IMPL_H */
