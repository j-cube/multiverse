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
#include "BTreeFileStorage.h"
#endif

#ifndef MILLIWAYS_BTREEFILESTORAGE_IMPL_H
//#define MILLIWAYS_BTREEFILESTORAGE_IMPL_H

#include "Seriously.h"
#include "Utils.h"

namespace milliways {

template < size_t CACHESIZE, size_t BLOCKSIZE, int B_, typename KeyTraits, typename TTraits, class Compare >
const typename LRUNodeCache<CACHESIZE, BLOCKSIZE, B_, KeyTraits, TTraits, Compare>::size_type LRUNodeCache<CACHESIZE, BLOCKSIZE, B_, KeyTraits, TTraits, Compare>::Size;

template < size_t CACHESIZE, size_t BLOCKSIZE, int B_, typename KeyTraits, typename TTraits, class Compare >
const typename LRUNodeCache<CACHESIZE, BLOCKSIZE, B_, KeyTraits, TTraits, Compare>::size_type LRUNodeCache<CACHESIZE, BLOCKSIZE, B_, KeyTraits, TTraits, Compare>::BlockSize;

template < size_t CACHESIZE, size_t BLOCKSIZE, int B_, typename KeyTraits, typename TTraits, class Compare >
const node_id_t LRUNodeCache<CACHESIZE, BLOCKSIZE, B_, KeyTraits, TTraits, Compare>::InvalidCacheKey;


template < size_t BLOCKSIZE, int B_, typename KeyTraits, typename TTraits, class Compare >
BTreeFileStorage<BLOCKSIZE, B_, KeyTraits, TTraits, Compare>::~BTreeFileStorage()
{
	if (isOpen())
	{
		std::cerr << std::endl << "WARNING: BTreeFileStorage still open at destruction time. Call close() *BEFORE* destruction!." << std::endl << std::endl;
		close();
	}
	assert(! isOpen());
	if (m_bs_allocated && m_block_storage)
	{
		delete m_block_storage;
		m_block_storage = NULL;
		m_bs_allocated = false;
	}
}

template < size_t BLOCKSIZE, int B_, typename KeyTraits, typename TTraits, class Compare >
void BTreeFileStorage<BLOCKSIZE, B_, KeyTraits, TTraits, Compare>::node_dispose_id_helper(node_id_t node_id)
{
	assert(m_block_storage);
	assert(m_block_storage->isOpen());
	assert(node_id != NODE_ID_INVALID);
	if (this->rootId() == node_id)
		this->rootId(NODE_ID_INVALID);
	m_block_storage->dispose(static_cast<block_id_t>(node_id));
}

template < size_t BLOCKSIZE, int B_, typename KeyTraits, typename TTraits, class Compare >
bool BTreeFileStorage<BLOCKSIZE, B_, KeyTraits, TTraits, Compare>::node_read(node_type& node)
{
	assert(m_block_storage);
	assert(m_block_storage->isOpen());

	// std::cerr << "nFS::node_read(" << node.id() << ")\n";
	node_id_t node_id = node.id();
	shptr<block_t> block( m_block_storage->get(static_cast<block_id_t>(node_id)) );
	if ((! block) || block->dirty())
	{
		node.dirty(true);
		// std::cerr << "nFS::node_read(" << node.id() << ") <- DIRTY\n";
		return false;
	}
	assert(block && (! block->dirty()));
	// TODO: convert block to node
	// TODO: implement LRU node cache
	bool ok = deserialize_node(node, *block);
	node.dirty(!ok);
	assert(node.id() == node_id);
	assert(! node.dirty());
	// std::cerr << "nFS::node_read(" << node.id() << ") <- " << (ok ? "OK" : "NO") << std::endl;
	return ok;
}

template < size_t BLOCKSIZE, int B_, typename KeyTraits, typename TTraits, class Compare >
bool BTreeFileStorage<BLOCKSIZE, B_, KeyTraits, TTraits, Compare>::node_write(node_type& node)
{
	assert(m_block_storage);
	assert(m_block_storage->isOpen());

	assert(node.valid());
	assert(! node.dirty());

	// std::cerr << "nFS::node_Write(" << node.id() << ")\n";

	node_id_t node_id = node.id();

	assert(node_id != NODE_ID_INVALID);

	block_t block(static_cast<block_id_t>(node_id));
	assert(! block.dirty());

	bool ok = serialize_node(block, node);
	m_block_storage->put(block);
	assert(block.index() == static_cast<block_id_t>(node_id));
	node.dirty(!ok);

	// std::cerr << "nFS::node_write(" << node.id() << ") <- " << (ok ? "OK" : "NO") << std::endl;
	return ok;
}

template < size_t BLOCKSIZE, int B_, typename KeyTraits, typename TTraits, class Compare >
shptr<typename BTreeFileStorage<BLOCKSIZE, B_, KeyTraits, TTraits, Compare>::node_type> BTreeFileStorage<BLOCKSIZE, B_, KeyTraits, TTraits, Compare>::node_alloc(node_id_t node_id)
{
	// std::cerr << "nFS::node_alloc(" << node_id << ")\n";
	assert(node_id != NODE_ID_INVALID);
	assert(m_block_storage);
	assert(m_block_storage->isOpen());

	assert (! m_lru.has(node_id));
	shptr<node_type> node_ptr( new node_type(this->tree(), node_id) );
	assert(node_ptr && (node_ptr->id() == node_id));
	m_lru.set(node_id, node_ptr);
	assert(! node_ptr->dirty());
	return node_ptr;
}

template < size_t BLOCKSIZE, int B_, typename KeyTraits, typename TTraits, class Compare >
void BTreeFileStorage<BLOCKSIZE, B_, KeyTraits, TTraits, Compare>::node_dealloc(shptr<node_type>& node)
{
	// std::cerr << "nFS::node_dealloc(" << node->id() << ")\n";
	if (node && (node->id() != NODE_ID_INVALID))
	{
		node_id_t node_id = node->id();
		assert(node_id != NODE_ID_INVALID);

		base_type::node_dealloc(node);

		shptr<node_type> cached_ptr;
		if (m_lru.get(cached_ptr, node_id))
		{
			assert(cached_ptr == node);
			m_lru.del(node_id);
			// TODO: how to handle deletion? should we reset the ptr?
			// TODO: cached_ptr.reset() ?
		}
	}
}

template < size_t BLOCKSIZE, int B_, typename KeyTraits, typename TTraits, class Compare >
shptr<typename BTreeFileStorage<BLOCKSIZE, B_, KeyTraits, TTraits, Compare>::node_type> BTreeFileStorage<BLOCKSIZE, B_, KeyTraits, TTraits, Compare>::node_get(node_id_t node_id)
{
	// std::cerr << "nFS::node_get(" << node_id << ")\n";
	// block_t* block = m_block_storage->get(static_cast<block_id_t>(node_id));
	// node_type* node = new node_type(this->tree(), node_id);
	// deserialize_node(*node, *block);
	// assert(node->id() == node_id);
	// return node;

	return m_lru[node_id];
}

template < size_t BLOCKSIZE, int B_, typename KeyTraits, typename TTraits, class Compare >
shptr<typename BTreeFileStorage<BLOCKSIZE, B_, KeyTraits, TTraits, Compare>::node_type> BTreeFileStorage<BLOCKSIZE, B_, KeyTraits, TTraits, Compare>::node_put(shptr<node_type>& node)
{
	// std::cerr << "nFS::node_put(" << node->id() << ")\n";
	assert(node);
	assert(m_block_storage);

	// block_t block(static_cast<block_id_t>(node->id()));

	// serialize_node(block, *node);
	// m_block_storage->put(block);
	// assert(block.index() == static_cast<block_id_t>(node->id()));

	// return node;

	node_id_t node_id = node->id();
	assert(node_id != NODE_ID_INVALID);
	assert(! node->dirty());
	shptr<node_type> node_ptr( m_lru[node_id] );
	if (! node_ptr)
	{
		node_ptr = node;
		m_lru[node_id] = node;
	} else if (node_ptr != node)
	{
		*node_ptr = *node;
	}
	assert(! node_ptr->dirty());
	return node_ptr;
}

/* -- Header I/O ----------------------------------------------- */

#define MAX_USER_HEADER 240

template < size_t BLOCKSIZE, int B_, typename KeyTraits, typename TTraits, class Compare >
bool BTreeFileStorage<BLOCKSIZE, B_, KeyTraits, TTraits, Compare>::header_write()
{
	seriously::Packer<MAX_USER_HEADER> packer;
	std::string headerPrefix("MWB+TREE");
	packer << headerPrefix <<
		static_cast<uint32_t>(B) << static_cast<uint32_t>(BLOCKSIZE) <<
		static_cast<uint64_t>(this->size()) << static_cast<uint64_t>(this->rootId());
	// std::cerr << "<- WRITE B:" << B << " BLOCKSIZE:" << BLOCKSIZE << " count:" << this->size() << " rootId:" << this->rootId() << std::endl;

	std::string userHeader(packer.data(), packer.size());
	m_block_storage->setUserHeader(m_btree_header_uid, userHeader);

//	std::cerr << "W userHeader[" << m_btree_header_uid << "]:" << std::endl;
//	std::cerr << s_hexdump(userHeader.data(), userHeader.size()) << std::endl;

	return true;
}

template < size_t BLOCKSIZE, int B_, typename KeyTraits, typename TTraits, class Compare >
bool BTreeFileStorage<BLOCKSIZE, B_, KeyTraits, TTraits, Compare>::header_read()
{
	std::string userHeader = m_block_storage->getUserHeader(m_btree_header_uid);

	seriously::Packer<MAX_USER_HEADER> packer(userHeader);

//	std::cerr << "R userHeader[" << m_btree_header_uid << "]:" << std::endl;
//	std::cerr << s_hexdump(userHeader.data(), userHeader.size()) << std::endl;

	std::string headerPrefix;
	uint32_t v_B, v_BLOCKSIZE;
	uint64_t v_size, v_root_id;

	packer >> headerPrefix >> v_B >> v_BLOCKSIZE >> v_size >> v_root_id;

	// std::cerr << "-> READ B:" << v_B << " BLOCKSIZE:" << v_BLOCKSIZE << " count:" << v_size << " rootId:" << v_root_id << std::endl;

	if ((v_B != B) || (v_BLOCKSIZE != BLOCKSIZE))
	{
		std::cerr << "ERROR: '" << m_block_storage->pathname() << "' doesn't match with btree properties (B/BLOCKSIZE)" << std::endl;
		return false;
	}

	assert(v_B == B);
	assert(v_BLOCKSIZE == BLOCKSIZE);
	this->rootId(v_root_id);
	this->size(v_size);

	return true;
}

/* -- Serialization -------------------------------------------- */

template <size_t BLOCKSIZE, size_t MAX_SERIALIZED_KEYSIZE, typename TTraits>
inline int BTreeFileStorage_Compute_Max_B()
{
	/*
	 * full node: n == (2 * B - 1)
	 *
	 * Leaf:
	 *   BLOCKSIZE >= HEAD_SIZE + (2*B-1) * KEY_SIZE + (2*B-1) * VALUE_SIZE
	 * Internal:
	 *   BLOCKSIZE >= HEAD_SIZE + (2*B-1) * KEY_SIZE + 2*B * sizeof(uint32_t)
	 *
	 * leaf worst case:
	 *   BLOCKSIZE == HEAD_SIZE + (2*B-1) * KEY_SIZE + (2*B-1) * VALUE_SIZE
	 *   (2*B-1) == (BLOCKSIZE - HEAD_SIZE) / (KEY_SIZE + VALUE_SIZE)
	 *   B == ((BLOCKSIZE - HEAD_SIZE) / (KEY_SIZE + VALUE_SIZE) + 1) / 2
	 *
	 * internal worst case:
	 *   BLOCKSIZE == HEAD_SIZE + (2*B-1) * KEY_SIZE + 2*B * sizeof(uint32_t)
	 *   2*B * (KEY_SIZE + sizeof(uint32_t)) == (BLOCKSIZE - HEAD_SIZE + KEY_SIZE)
	 *   B == ((BLOCKSIZE - HEAD_SIZE + KEY_SIZE) / (KEY_SIZE + sizeof(uint32_t))) / 2
	 */

	// head_size should be 21
	size_t head_size = seriously::Traits<uint32_t>::SerializedSize * 4 +
		seriously::Traits<bool>::SerializedSize * 1 +
		seriously::Traits<uint16_t>::SerializedSize * 1 +
		seriously::Traits<int16_t>::SerializedSize * 1;

	size_t leaf_B = ((BLOCKSIZE - head_size) / (MAX_SERIALIZED_KEYSIZE + TTraits::SerializedSize) + 1) / 2;
	size_t internal_B = ((BLOCKSIZE - head_size + MAX_SERIALIZED_KEYSIZE) / (MAX_SERIALIZED_KEYSIZE + sizeof(uint32_t))) / 2;

	// with BLOCKSIZE == 4096:
	//   (TTraits::SerializedSize == 4 + 2 == 6)
	// KeyValueStore leaf_B: 68       (MAX_SERIALIZED_KEYSIZE==24 for 20 byte *binary* + 4 length)
	// KeyValueStore internal_B: 73   (MAX_SERIALIZED_KEYSIZE==24 for 20 byte *binary* + 4 length)

	// KeyValueStore leaf_B: 41       (MAX_SERIALIZED_KEYSIZE==44 for 40 byte *ASCII* + 4 length)
	// KeyValueStore internal_B: 42   (MAX_SERIALIZED_KEYSIZE==44 for 40 byte *ASCII* + 4 length)

	return (leaf_B < internal_B) ? leaf_B : internal_B;
}

template < size_t BLOCKSIZE, int B_, typename KeyTraits, typename TTraits, class Compare >
bool BTreeFileStorage<BLOCKSIZE, B_, KeyTraits, TTraits, Compare>::serialize_node(block_t& dst_block, const node_type& src_node)
{
	seriously::Packer<BLOCKSIZE> packer;

	// std::cerr << "nFS::serialize_node(id:" << src_node.id() << ")\n";

	packer << static_cast<uint32_t>(src_node.id()) <<
			static_cast<uint32_t>(src_node.parentId()) <<
			static_cast<uint32_t>(src_node.leftId()) <<
			static_cast<uint32_t>(src_node.rightId()) <<
			static_cast<bool>(src_node.leaf()) <<
			static_cast<uint16_t>(src_node.n()) <<
			static_cast<int16_t>(src_node.rank());
	assert(! packer.error());

	// std::cerr << "id:" << src_node.id() << " parent:" << src_node.parentId() <<
	// 		" left:" << src_node.leftId() << " right:" << src_node.rightId() <<
	// 		" leaf:" << (src_node.leaf() ? "t" : "f") << " n:" << src_node.n() << " rank:" << src_node.rank() << "\n";

	int n = src_node.n();

	for (int i = 0; i < n; i++)
		packer << src_node.key(i);
	if (src_node.leaf())
	{
		for (int i = 0; i < n; i++)
			packer << src_node.value(i);
	} else
	{
		for (int i = 0; i <= n; i++)
			packer << static_cast<uint32_t>(src_node.child(i));
	}
	assert(! packer.error());
	assert(packer.size() <= dst_block.size());
	memcpy(dst_block.data(), packer.data(), packer.size());

	return (! packer.error());
}

template < size_t BLOCKSIZE, int B_, typename KeyTraits, typename TTraits, class Compare >
bool BTreeFileStorage<BLOCKSIZE, B_, KeyTraits, TTraits, Compare>::deserialize_node(node_type& dst_node, const block_t& src_block)
{
	seriously::Packer<BLOCKSIZE> packer(src_block.data(), src_block.size());

	// std::cerr << "nFS::deserialize_node(id:" << src_block.index() << ")\n";

	assert(! packer.error());
	assert(packer.size() <= src_block.size());

	uint32_t v_node_id, v_parent_id, v_left_id, v_right_id;
	bool v_leaf;
	uint16_t v_n;
	int16_t v_rank;

	packer >> v_node_id >> v_parent_id >> v_left_id >> v_right_id >>
			v_leaf >> v_n >> v_rank;
	assert(! packer.error());

	dst_node.id(v_node_id);
	dst_node.parentId(v_parent_id);
	dst_node.leftId(v_left_id);
	dst_node.rightId(v_right_id);
	dst_node.leaf(v_leaf);
	dst_node.n(v_n);
	dst_node.rank(v_rank);

	for (int i = 0; i < v_n; i++)
		packer >> dst_node.key(i);
	if (v_leaf)
	{
		for (int i = 0; i < v_n; i++)
			packer >> dst_node.value(i);
	} else
	{
		uint32_t v_child;
		for (int i = 0; i <= v_n; i++)
		{
			packer >> v_child;
			dst_node.child(i) = static_cast<node_id_t>(v_child);
		}
	}

	// std::cerr << "id:" << dst_node.id() << " parent:" << dst_node.parentId() <<
	// 	" left:" << dst_node.leftId() << " right:" << dst_node.rightId() <<
	// 	" leaf:" << (dst_node.leaf() ? "t" : "f") << " n:" << dst_node.n() << " rank:" << dst_node.rank() << "\n";

	return (! packer.error());
}

} /* end of namespace milliways */

#endif /* MILLIWAYS_BTREEFILESTORAGE_IMPL_H */
