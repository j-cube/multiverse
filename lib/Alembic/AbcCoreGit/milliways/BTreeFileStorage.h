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

#ifndef MILLIWAYS_BTREEFILESTORAGE_H
#define MILLIWAYS_BTREEFILESTORAGE_H

#include <iostream>
#include <fstream>
#include <string>
#include <functional>
#include <array>

#include <stdint.h>
#include <assert.h>

#include "BlockStorage.h"
#include "BTreeCommon.h"
#include "LRUCache.h"

namespace milliways {

template < size_t CACHESIZE, size_t BLOCKSIZE, int B_, typename KeyTraits, typename TTraits, class Compare = std::less<typename KeyTraits::type> >
class LRUNodeCache : public LRUCache< CACHESIZE, node_id_t, MW_SHPTR< BTreeNode<B_, KeyTraits, TTraits, Compare> > >
{
public:
	typedef node_id_t key_type;
	typedef BTreeNode<B_, KeyTraits, TTraits, Compare> node_type;
	typedef MW_SHPTR<node_type> node_ptr_type;
	typedef node_ptr_type mapped_type;
	typedef std::pair<key_type, mapped_type> value_type;
	typedef LRUCache<CACHESIZE, node_id_t, node_ptr_type> base_type;
	typedef typename base_type::size_type size_type;

	typedef BTreeStorage<B_, KeyTraits, TTraits, Compare>* storage_ptr_type;

	static const size_type Size = CACHESIZE;
	static const size_type BlockSize = BLOCKSIZE;
	static const node_id_t InvalidCacheKey = NODE_ID_INVALID;

	LRUNodeCache(storage_ptr_type storage) :
			base_type(LRUNodeCache::InvalidCacheKey), m_storage(storage) {}
	~LRUNodeCache() { this->evict_all(); }

	bool on_miss(typename base_type::op_type op, const key_type& key, mapped_type& value)
	{
		// std::cerr << "node MISS id:" << key << " op:" << (int)op << "\n";
		node_id_t node_id = key;
		if (m_storage->has_id(node_id)) {
			/* allocate node object and read node data from disk */
			// MW_SHPTR<node_type> node( new node_type(m_storage->tree(), node_id) );
			MW_SHPTR<node_type> node_ptr( m_storage->manager().get_object(node_id) );
			assert(node_ptr && (node_ptr->id() == node_id));
			if (! node_ptr) return false;
			bool rv = false;
			assert(node_ptr->id() == node_id);
			switch (op)
			{
			case base_type::op_get:
				rv = m_storage->ll_node_read(*node_ptr);
				assert(rv || node_ptr->dirty());
				if (rv) {
					node_ptr->dirty(false);
					value = node_ptr;
				}
				// std::cerr << "op GET rv:" << (rv ? "OK" : "NO") << "  ptr:" << node_ptr <<  " value:" << value << "\n";
				return rv;
				break;
			case base_type::op_set:
				assert(value);
				*node_ptr = *value;
				// std::cerr << "op SET rv:" << (true ? "OK" : "NO") << "  ptr:" << node_ptr <<  " value:" << value << "\n";
				break;
			case base_type::op_sub:
				//assert(value);
				rv = m_storage->ll_node_read(*node_ptr);
				assert(rv || node_ptr->dirty());
				value = node_ptr;
				// std::cerr << "op [] rv:" << (rv ? "OK" : "NO") << "  ptr:" << node_ptr <<  " value:" << value << "\n";
				return rv;
				break;
			default:
				assert(false);
				return false;
				break;
			}
			return true;
		}
		return false;
	}
	bool on_set(const key_type& /* key */, const mapped_type& /* value */)
	{
		return true;
	}
	//bool on_delete(const key_type& key);
	bool on_eviction(const key_type& /* key */, mapped_type& value)
	{
		/* write back block */
		/* node_id_t node_id = key; */
		if (value) {
			node_type* node = value.get();
			if (node)
			{
				if (node->id() != NODE_ID_INVALID)
				{
#ifdef NDEBUG
					m_storage->ll_node_write(*node);
#else
					bool ok = m_storage->ll_node_write(*node);
					assert(ok);
#endif
				}
				// node->id(NODE_ID_INVALID);
				// value.reset();
			}
		}
		return true;
	}

private:
	LRUNodeCache(const LRUNodeCache&) {}
	LRUNodeCache& operator= (const LRUNodeCache&) {}

	storage_ptr_type m_storage;
};

template <size_t BLOCKSIZE, size_t MAX_SERIALIZED_KEYSIZE, typename TTraits>
int BTreeFileStorage_Compute_Max_B();

template < size_t BLOCKSIZE, int B_, typename KeyTraits, typename TTraits, class Compare = std::less<typename KeyTraits::type> >
class BTreeFileStorage : public BTreeStorage<B_, KeyTraits, TTraits, Compare>
{
public:
	static const size_t BlockSize = BLOCKSIZE;
	static const int BlockCacheSize = MILLIWAYS_DEFAULT_BLOCK_CACHE_SIZE;

	typedef Block<BLOCKSIZE> block_t;
	typedef FileBlockStorage<BLOCKSIZE, BlockCacheSize> block_storage_t;

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

	typedef LRUNodeCache< MILLIWAYS_DEFAULT_NODE_CACHE_SIZE, BLOCKSIZE, B_, KeyTraits, TTraits, Compare > cache_type;

	static const int B = B_;

	BTreeFileStorage(block_storage_t* block_storage) :
			BTreeStorage<B_, KeyTraits, TTraits, Compare>(), m_block_storage(block_storage), m_bs_allocated(false), m_btree_header_uid(-1), m_lru(this)
	{
		assert(block_storage);
		m_btree_header_uid = m_block_storage->allocUserHeader();
		m_bs_allocated = false;
	}

	BTreeFileStorage(const std::string& pathname) :
			BTreeStorage<B_, KeyTraits, TTraits, Compare>(), m_block_storage(NULL), m_bs_allocated(false), m_btree_header_uid(-1), m_lru(this)
	{
		m_block_storage = new block_storage_t(pathname);
		m_bs_allocated = true;
		m_btree_header_uid = m_block_storage->allocUserHeader();
	}

	virtual ~BTreeFileStorage();

	static BTreeFileStorage* createStorage(tree_type* tree_, const std::string& pathname) {
		BTreeFileStorage* storage = new BTreeFileStorage(pathname);
		storage->tree(tree_);
		return storage;
	}

	/* -- General I/O ---------------------------------------------- */

	bool isOpen() const { assert(m_block_storage); return m_block_storage->isOpen(); }
	bool open() { return base_type::open(); }
	bool close() { return base_type::close(); }
	bool flush() { assert(m_block_storage); return m_block_storage->flush(); }

	bool openHelper(bool& created_) { assert(m_block_storage); bool r = m_block_storage->open(); created_ = m_block_storage->created(); return r; }
	bool closeHelper() { assert(m_block_storage); m_lru.evict_all(); return m_block_storage->close(); }

	/* -- Node I/O - low level (direct) ---------------------------- */

	bool has_id(node_id_t node_id) { assert(m_block_storage); return m_block_storage->hasId(static_cast<block_id_t>(node_id)); }
	node_id_t node_alloc_id() { assert(m_block_storage); assert(m_block_storage->isOpen()); return static_cast<node_id_t>(m_block_storage->allocId()); }
	void node_dispose_id_helper(node_id_t node_id);

	bool ll_node_read(node_type& node);
	bool ll_node_write(node_type& node);

	/* -- Node I/O - hight level (cached) -------------------------- */

	MW_SHPTR<node_type> node_alloc(node_id_t node_id);
	void node_dealloc(MW_SHPTR<node_type>& node);
	MW_SHPTR<node_type> node_read(node_id_t node_id);
	MW_SHPTR<node_type> node_write(MW_SHPTR<node_type>& node);

	/* -- Header I/O ----------------------------------------------- */

	bool header_write();
	bool header_read();

	/* -- Serialization -------------------------------------------- */

	bool serialize_node(block_t& dst_block, const node_type& src_node);
	bool deserialize_node(node_type& dst_node, const block_t& src_block);

private:
	BTreeFileStorage(const BTreeFileStorage& other);
	BTreeFileStorage& operator= (const BTreeFileStorage& other);

	block_storage_t* m_block_storage;
	bool m_bs_allocated;
	int m_btree_header_uid;

	cache_type m_lru;
};

} /* end of namespace milliways */

#include "BTreeFileStorage.impl.hpp"

#endif /* MILLIWAYS_BTREEFILESTORAGE_H */
