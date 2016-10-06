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

#ifndef MILLIWAYS_KEYVALUESTORE_H
#define MILLIWAYS_KEYVALUESTORE_H

#include <iostream>
#include <fstream>
#include <string>
#include <functional>

#include <stdint.h>
#include <assert.h>

#include "Seriously.h"
#include "BlockStorage.h"
#include "BTreeCommon.h"
#include "BTreeNode.h"
#include "BTree.h"
#include "BTreeFileStorage.h"

namespace milliways {

static const size_t KV_BLOCKSIZE = MILLIWAYS_DEFAULT_BLOCK_SIZE;			/* default: 4096 */
static const int KV_BLOCK_CACHESIZE = MILLIWAYS_DEFAULT_BLOCK_CACHE_SIZE;	/* default: 8192 */
static const int KV_NODE_CACHESIZE = MILLIWAYS_DEFAULT_NODE_CACHE_SIZE;		/* default: 1024 */
static const int KV_B = MILLIWAYS_DEFAULT_B_FACTOR;							/* default: 73   */

typedef uint64_t serialized_data_pos_type;
typedef uint16_t serialized_data_offset_type;
typedef uint32_t serialized_value_size_type;

/* ----------------------------------------------------------------- *
 *   FullLocator                                                     *
 *     full size + compressed size + payload                         *
 * ----------------------------------------------------------------- */

typedef StreamPos<KV_BLOCKSIZE> kv_stream_pos_t;
typedef StreamSizedPos<KV_BLOCKSIZE> kv_stream_sized_pos_t;

struct FullLocator : public kv_stream_sized_pos_t
{
public:
	static const size_type ENVELOPE_SIZE = (2 * sizeof(serialized_value_size_type));

	FullLocator() : kv_stream_sized_pos_t(), m_uncompressed(0) {}
	FullLocator(size_t linear_pos_, size_t size_) :
		kv_stream_sized_pos_t(linear_pos_, size_), m_uncompressed(0) {}
	FullLocator(const FullLocator& other) :
		kv_stream_sized_pos_t(other.pos(), other.full_size()), m_uncompressed(other.m_uncompressed) {}
	FullLocator(const kv_stream_sized_pos_t& sizedLocator_, size_t uncompressed_) :
		kv_stream_sized_pos_t(sizedLocator_), m_uncompressed(uncompressed_) {}
	FullLocator(const FullLocator& other, offset_t delta_) :
		kv_stream_sized_pos_t(other.pos(), other.full_size()), m_uncompressed(other.m_uncompressed) { delta(delta_); }
	FullLocator& operator=(const FullLocator& other) { m_pos = other.m_pos; m_full_size = other.m_full_size; m_uncompressed = other.m_uncompressed; return *this; }
	FullLocator& operator=(const kv_stream_sized_pos_t& sl) { m_pos = sl.pos(); m_full_size = sl.size(); return *this; }
	FullLocator& operator=(const kv_stream_pos_t& dl) { m_pos = dl.pos(); return *this; }

	bool operator==(const FullLocator& rhs) const
	{
		return (((m_pos < 0) && (rhs.m_pos < 0)) ||
		        ((m_pos == rhs.m_pos) && (m_full_size == rhs.m_full_size) && (m_uncompressed == rhs.m_uncompressed)));
	}
	bool operator!=(const FullLocator& rhs) const { return (!(*this == rhs)); }
	bool operator<(const FullLocator& rhs) const {
		if ((m_pos < 0) && (rhs.m_pos < 0))
			return false;
		if (m_pos < rhs.m_pos)
			return true;
		else if (m_pos == rhs.m_pos)
		{
			if (m_full_size < rhs.m_full_size)
				return true;
			else
				return (m_uncompressed < rhs.m_uncompressed);
		}
		return false;
	}

	kv_stream_sized_pos_t sizedLocator() const { return kv_stream_sized_pos_t(m_pos, m_full_size); }

	// kv_stream_sized_pos_t sizedLocator() const { return kv_stream_sized_pos_t(m_block_id, m_offset); }
	// FullLocator& dataLocator(const kv_stream_pos_t& value) { block_id(value.block_id()); offset(value.offset()); return *this; }

	size_type size() const { assert(false); return (size_type) -1; }
	size_type size(size_type /* value */) { assert(false); return (size_type) -1; }

	size_type full_size() const { return m_full_size; }
	size_type full_size(size_type value) { size_type old = m_full_size; m_full_size = value; return old; }

	size_type header_size() const { return FullLocator::ENVELOPE_SIZE; }

	/* compressed _payload_ (only) size */
	size_type compressed_size() const { return (enveloped_size() - ENVELOPE_SIZE); }

	/* uncompressed data (only) size */
	size_type uncompressed_size() const { return (m_uncompressed == 0) ? compressed_size() : m_uncompressed; }

	/* compressed payload size (same as compressed_size) */
	size_type payload_size() const { return compressed_size(); }

	/* same as uncompressed data (only) */
	size_type contents_size() const { return uncompressed_size(); }

	FullLocator& set_uncompressed(size_type contents_) { m_uncompressed = 0; payload_size(contents_); return *this; }
	FullLocator& set_compressed(size_type compressed_, size_type contents_) { payload_size(compressed_); m_uncompressed = contents_; return *this; }

	bool isCompressed() const { return (m_uncompressed != 0); }

	kv_stream_sized_pos_t headLocator() const { return sizedLocator(); }
	kv_stream_sized_pos_t payloadLocator() const { size_t off = FullLocator::ENVELOPE_SIZE; kv_stream_sized_pos_t pl = sizedLocator(); pl.delta(static_cast<offset_t>(off)); pl.shrink(off); return pl; }

protected:
	size_type enveloped_size() const { return full_size(); }
	size_type enveloped_size(size_type value) { return full_size(value); }
	size_type compressed_size(size_type value) { return enveloped_size(value + ENVELOPE_SIZE); }
	size_type payload_size(size_type value) { return compressed_size(value); }

	size_type m_uncompressed;		// uncompressed size (w/o header)
};

inline std::ostream& operator<< (std::ostream& out, const FullLocator& value)
{
	if (value.valid())
		out << "<KVFullLocator block:" << value.block_id() << " offset:" << (int)value.offset() << " enveloped-size:" << value.full_size() << " compressed-size:" << value.compressed_size() << " contents-size:" << value.contents_size() << ">";
	else
		out << "<KVFullLocator invalid>";
	return out;
}

} /* end of namespace milliways */

namespace seriously {

/* ----------------------------------------------------------------- *
 *   ::seriously::Traits<milliways::kv_stream_pos_t>                     *
 * ----------------------------------------------------------------- */

//template <typename T>
//struct KVTraits;

template <>
struct Traits<milliways::kv_stream_pos_t>
{
	typedef milliways::kv_stream_pos_t type;
	typedef milliways::serialized_data_pos_type serialized_type;
	typedef milliways::serialized_data_pos_type serialized_data_pos_type;
	enum { Size = sizeof(type) };
	enum { SerializedSize = (sizeof(serialized_type)) };

	static ssize_t serialize(char*& dst, size_t& avail, const type& v);
	static ssize_t deserialize(const char*& src, size_t& avail, type& v);

	static size_t size(const type& value)    { UNUSED(value); return Size; }
	static size_t maxsize(const type& value) { UNUSED(value); return Size; }
	static size_t serializedsize(const type& value) { UNUSED(value); return SerializedSize; }

	static bool valid(const type& value)     { return value.valid(); }

	static int compare(const type& a, const type& b) { if (a == b) return 0; else if (a < b) return -1; else return +1; }
};

template <>
struct Traits<milliways::kv_stream_sized_pos_t>
{
	typedef milliways::kv_stream_sized_pos_t type;
	typedef type serialized_type;
	typedef milliways::serialized_data_pos_type serialized_data_pos_type;
	typedef milliways::serialized_value_size_type serialized_size_type;	/* uint32_t */
	enum { Size = sizeof(type) };
	enum { SerializedSize = (sizeof(serialized_data_pos_type) + sizeof(serialized_size_type)) };

	static ssize_t serialize(char*& dst, size_t& avail, const type& v);
	static ssize_t deserialize(const char*& src, size_t& avail, type& v);

	static size_t size(const type& value)    { UNUSED(value); return Size; }
	static size_t maxsize(const type& value) { UNUSED(value); return Size; }
	static size_t serializedsize(const type& value) { UNUSED(value); return SerializedSize; }

	static bool valid(const type& value)     { return value.valid(); }

	static int compare(const type& a, const type& b) { if (a == b) return 0; else if (a < b) return -1; else return +1; }
};

} /* end of namespace seriously */

namespace milliways {

/* ----------------------------------------------------------------- *
 *   KeyValueStore                                                   *
 * ----------------------------------------------------------------- */

class KeyValueStore
{
public:
	static const uint32_t MAJOR_VERSION = 0;
	static const uint32_t MINOR_VERSION = 1;


	static const size_t BLOCKSIZE = KV_BLOCKSIZE;
	static const size_t NODE_CACHESIZE = KV_NODE_CACHESIZE;
	static const size_t BLOCK_CACHESIZE = KV_BLOCK_CACHESIZE;
	static const int    B = KV_B;
	static const size_t KEY_HASH_SIZE = 20;

	static const size_t KEY_MAX_SIZE = 20;

	class kv_write_stream;
	class kv_read_stream;

	/*
	 * we use our B+Tree to map a hash of the original key to a value-locator
	 * (kv_stream_pos_t struct above). So from the BTree point of view, its key is
	 * the hash of the user key.
	 * The hash is assembled this way:
	 *   4 bytes uid + 128-bit MurmurHash3 (16 bytes) == 20 bytes
	 *
	 * Alternative:
	 *   1st 2 bytes + 4-bytes length + 128-bit MurmurHash3 (16 bytes) + last 2 bytes == 24 bytes
	 */
	typedef seriously::Traits<std::string> key_traits;
	typedef seriously::Traits<kv_stream_pos_t> mapped_traits;
	typedef BTree< B, key_traits, mapped_traits > kv_tree_type;
	typedef BTreeFileStorage< BLOCKSIZE, B, key_traits, mapped_traits > kv_tree_storage_type;
	typedef XTYPENAME kv_tree_type::node_type kv_tree_node_type;
	typedef XTYPENAME kv_tree_type::lookup_type kv_tree_lookup_type;
	typedef XTYPENAME kv_tree_type::iterator kv_tree_iterator_type;

//	typedef FileBlockStorage<BLOCKSIZE, BLOCK_CACHESIZE> block_storage_type;
	typedef XTYPENAME kv_tree_storage_type::block_storage_t block_storage_type;
	typedef XTYPENAME block_storage_type::block_t block_type;

	typedef int32_t key_index_type;
	typedef seriously::Traits<key_index_type> index_key_traits;
	typedef seriously::Traits<kv_stream_pos_t> index_mapped_traits;
	typedef BTree< B, index_key_traits, index_mapped_traits > kv_index_tree_type;

	typedef XTYPENAME block_storage_type::stream_pos_t       stream_pos_t;
	typedef XTYPENAME block_storage_type::stream_sized_pos_t stream_sized_pos_t;
	typedef XTYPENAME 
	block_storage_type::write_stream_t write_stream_t;
	typedef XTYPENAME block_storage_type::read_stream_t read_stream_t;

	class iterator;
	class const_iterator;

	struct Search
	{
	public:
		typedef XTYPENAME FullLocator::offset_t offset_t;
		typedef XTYPENAME FullLocator::block_offset_t block_offset_t;
		typedef XTYPENAME FullLocator::size_type size_type;

		Search() : m_lookup(), m_full_loc() {}
		Search(const Search& other) : m_lookup(other.m_lookup), m_full_loc(FullLocator(other.m_full_loc)) {}
		Search& operator= (const Search& other) { m_lookup = other.m_lookup; m_full_loc = other.m_full_loc; return *this; }

		bool operator== (const Search& rhs) const { return (m_lookup == rhs.m_lookup) && (m_full_loc == rhs.m_full_loc); }
		bool operator!= (const Search& rhs) const { return (! (*this == rhs)); }

		operator bool() const { return valid() && found(); }

		const kv_tree_lookup_type& lookup() const { return m_lookup; }
		kv_tree_lookup_type& lookup() { return m_lookup; }

		const FullLocator& locator() const { return m_full_loc; }
		FullLocator& locator() { return m_full_loc; }
		Search& locator(const FullLocator& locator_) { m_full_loc = locator_; return *this; }

		bool found() const { return m_lookup.found(); }
		const std::string& key() const { return m_lookup.key(); }
		MW_SHPTR<kv_tree_node_type> node() const { return m_lookup.node(); }
		int pos() const { return m_lookup.pos(); }
		node_id_t nodeId() const { return m_lookup.nodeId(); }

		bool valid() const { return m_full_loc.valid(); }
		Search& invalidate() { m_full_loc.invalidate(); return *this; }

		offset_t position() const { return m_full_loc.pos(); }
		offset_t position(offset_t value) { return m_full_loc.pos(value); }

		block_id_t block_id() const { return m_full_loc.block_id(); }
		block_offset_t offset() const { return m_full_loc.offset(); }

		kv_stream_pos_t dataLocator() const { return m_full_loc.dataLocator(); }
		Search& dataLocator(const kv_stream_pos_t& value) { m_full_loc.dataLocator(value); return *this; }

		const FullLocator& headLocator() const { return locator(); }
		FullLocator& headLocator() { return locator(); }
		kv_stream_sized_pos_t payloadLocator() const { size_t off = FullLocator::ENVELOPE_SIZE; kv_stream_sized_pos_t pl = locator().sizedLocator(); pl.delta(static_cast<offset_t>(off)); pl.shrink(off); return pl; }

		kv_stream_pos_t headDataLocator() const { return headLocator().dataLocator(); }
		kv_stream_pos_t payloadDataLocator() const { return payloadLocator().dataLocator(); }

		size_type full_size() const { return m_full_loc.full_size(); }
		size_type full_size(size_type value) { return m_full_loc.full_size(value); }

		size_type compressed_size() const { return m_full_loc.compressed_size(); }
		size_type uncompressed_size() const { return m_full_loc.uncompressed_size(); }

		/* compressed payload size (same as compressed_size) */
		size_type payload_size() const { return m_full_loc.payload_size(); }

		/* same as uncompressed data (only) */
		size_type contents_size() const { return m_full_loc.contents_size(); }

		FullLocator& set_uncompressed(size_type contents_) { return m_full_loc.set_uncompressed(contents_); }
		FullLocator& set_compressed(size_type compressed_, size_type contents_) { return m_full_loc.set_compressed(compressed_, contents_); }

		bool isCompressed() const { return m_full_loc.isCompressed(); }

	private:
		Search(const kv_tree_lookup_type& lookup_, const FullLocator& vl) :
			m_lookup(lookup_), m_full_loc(FullLocator(vl)) {}

		kv_tree_lookup_type m_lookup;
		FullLocator m_full_loc;
	};

	KeyValueStore(block_storage_type* blockstorage);
	~KeyValueStore();

	bool isOpen() const;
	bool open();
	bool close();

	bool has(const std::string& key);
	bool find(const std::string& key, Search& result);
	Search find(const std::string& key) { Search result; find(key, result); return result; }
	bool get(const std::string& key, std::string& value);
	bool get(Search& result, std::string& value, ssize_t partial = -1);			/* streaming/partial reads */ 
	std::string get(const std::string& key);
	bool put(const std::string& key, const std::string& value, bool overwrite = true);
	bool rename(const std::string& old_key, const std::string& new_key);

	/* -- Iteration ------------------------------------------------ */

	iterator begin() { return iterator(this); }
	iterator end() { return iterator(this, /* forward */ true, /* end */ true); }

	iterator rbegin() { return iterator(this, /* forward */ false); }
	iterator rend() { return iterator(this, /* forward */ false, /* end */ true); }

	class base_iterator
	{
	public:
		typedef KeyValueStore kv_type;
		typedef base_iterator self_type;
		typedef XTYPENAME kv_type::kv_tree_type kv_tree_type;
		typedef XTYPENAME kv_type::kv_tree_node_type kv_tree_node_type;
		typedef XTYPENAME kv_type::kv_tree_lookup_type kv_tree_lookup_type;
		typedef XTYPENAME kv_type::kv_tree_iterator_type kv_tree_iterator_type;
		typedef std::string value_type;
		typedef value_type& reference;
		typedef const value_type& const_reference;
		typedef value_type* pointer;
		typedef const value_type* const_pointer;
		typedef std::forward_iterator_tag iterator_category;
		typedef int difference_type;

		base_iterator() : m_kv(NULL), m_tree(NULL), m_tree_it(), m_forward(true), m_end(true), m_current_key() {}
		base_iterator(kv_type* kv_, bool forward_ = true, bool end_ = false) : m_kv(kv_), m_tree(NULL), m_tree_it(), m_forward(forward_), m_end(end_), m_current_key() { m_tree = m_kv->kv_tree(); kv_tree_iterator_type it(m_tree, m_tree->root(), forward_, end_); m_tree_it = it; rewind(end_); }
		base_iterator(const base_iterator& other) : m_kv(other.m_kv), m_tree(other.m_tree), m_tree_it(other.m_tree_it), m_forward(other.m_forward), m_end(other.m_end), m_current_key(other.m_current_key) { }
		base_iterator& operator= (const base_iterator& other) { m_kv = other.m_kv; m_tree = other.m_tree; m_tree_it = other.m_tree_it; m_forward = other.m_forward; m_end = other.m_end; m_current_key = other.m_current_key; return *this; }
		~base_iterator() { m_kv = NULL; m_tree = NULL; m_end = true; }

		self_type& operator++() { next(); return *this; }									/* prefix  */
		self_type operator++(int) { self_type i = *this; next(); return i; }				/* postfix */
		self_type& operator--() { prev(); return *this; }
		self_type operator--(int) { self_type i = *this; prev(); return i; }
		// reference operator*() { m_current_key = (*m_tree_it).key(); return m_current_key; }
		const_reference operator*() const { m_current_key = (*m_tree_it).key(); return m_current_key; }
		// pointer operator->() { m_current_key = (*m_tree_it).key(); return &m_current_key; }
		const_pointer operator->() const { m_current_key = (*m_tree_it).key(); return &m_current_key; }
		bool operator== (const self_type& rhs) {
			return (m_kv == rhs.m_kv) && (m_tree == rhs.m_tree) &&
					((end() && rhs.end()) ||
					 ((m_forward == rhs.m_forward) && (m_end == rhs.m_end) && (m_tree_it == rhs.m_tree_it))); }
		bool operator!= (const self_type& rhs) { return (! (*this == rhs)); }

		operator bool() const { return (! end()) && m_tree_it; }

		self_type& rewind(bool end_) { m_tree_it.rewind(end_); return *this; }
		bool next() { return m_tree_it.next(); }
		bool prev() { return m_tree_it.prev(); }

		kv_type* kv() const { return m_kv; }
		const_reference key() const { m_current_key = (*m_tree_it).key(); return m_current_key; }
		bool forward() const { return m_forward; }
		bool backward() const { return (! m_forward); }
		bool end() const { return m_end || (m_tree_it.end()); }

	protected:
		kv_type* m_kv;
		kv_tree_type* m_tree;
		mutable kv_tree_iterator_type m_tree_it;
		bool m_forward;
		bool m_end;
		mutable std::string m_current_key;
	};

	class iterator : public base_iterator
	{
	public:
		iterator(kv_type* kv_, bool forward_ = true, bool end_ = false) : base_iterator(kv_, forward_, end_) {}
		iterator(const iterator& other) : base_iterator(other) {}

		reference operator*() { m_current_key = (*m_tree_it).key(); return m_current_key; }
		pointer operator->() { m_current_key = (*m_tree_it).key(); return &m_current_key; }
	};

	class const_iterator : public base_iterator
	{
	public:
		const_iterator(kv_type* kv_, bool forward_ = true, bool end_ = false) : base_iterator(kv_, forward_, end_) {}
		const_iterator(const const_iterator& other) : base_iterator(other) {}
	};

	friend class base_iterator;
	friend class iterator;
	friend class const_iterator;

	/* -- Streaming ------------------------------------------------ */

protected:
	bool find(const std::string& key, kv_stream_pos_t& data_pos);
	// bool find(const std::string& key, kv_stream_sized_pos_t& sized_pos);
	// bool find(const std::string& key, FullLocator& full_pos);

	/* streaming read */
	bool read_lz4(read_stream_t& rs, char*& dstp, size_t nbytes, size_t& compressed_size) { return m_blockstorage->read_lz4(rs, dstp, nbytes, compressed_size); }
	bool read_lz4(read_stream_t& rs, std::string& dst, size_t nbytes, size_t& compressed_size) { return m_blockstorage->read_lz4(rs, dst, nbytes, compressed_size); }

	/* streaming write */
	bool write_lz4(write_stream_t& ws, const char*& srcp, size_t nbytes, size_t& compressed_size) { return m_blockstorage->write_lz4(ws, srcp, nbytes, compressed_size); }
	bool write_lz4(write_stream_t& ws, const std::string& src, size_t& compressed_size) { const char *srcp = src.data(); return write_lz4(ws, srcp, src.length(), compressed_size); }

	struct kv_alloc_info
	{
		kv_alloc_info() :
			initial_next_location(), returned(), initial_amount_req(0), allocated(false) {}

		kv_stream_sized_pos_t initial_next_location;
		kv_stream_sized_pos_t returned;
		size_t initial_amount_req;
		bool allocated;
	};

	bool alloc_space(kv_alloc_info& info, kv_stream_sized_pos_t& dst, size_t amount);
	bool mark_partial_use(kv_alloc_info& info, size_t used);
	bool extend_allocated_space(kv_alloc_info& info, kv_stream_sized_pos_t& dst, size_t amount);
	size_t size_in_blocks(size_t size);

	/* -- Header I/O ----------------------------------------------- */

	bool header_write();
	bool header_read();

	/* -- Block I/O ------------------------------------------------ */

	block_id_t block_alloc_id(int n_blocks = 1) { assert(m_blockstorage); return m_blockstorage->allocId(n_blocks); }
	bool block_dispose(block_id_t block_id, int count = 1) { assert(m_blockstorage); return m_blockstorage->dispose(block_id, count); }
	MW_SHPTR<block_type> block_get(block_id_t block_id) { assert(m_blockstorage); return m_blockstorage->get(block_id); }
	bool block_put(const block_type& src) { assert(m_blockstorage); return m_blockstorage->put(src); }

	/* -- Tree access ---------------------------------------------- */

	kv_tree_type* kv_tree() { return m_kv_tree; }

private:
	KeyValueStore();
	KeyValueStore(const KeyValueStore& other);
	KeyValueStore& operator= (const KeyValueStore& other);

	block_storage_type* m_blockstorage;
	kv_tree_storage_type* m_storage;
	kv_tree_type* m_kv_tree;

	/*
	 * We either allocate a single block for a short string
	 * or multiple consecutive blocks for a long string.
	 * When allocating multiple blocks, these are initially all free
	 * and at the end of their use, only the last block could remain
	 * partially available.
	 * So we store only:
	 *   - the current free block index (first in the span)
	 *   - the current offset in the block (for available space)
	 *   - currently available space
	 * If the currently available space is > BLOCKSIZE then
	 * we have multiple blocks in the span and they are completely
	 * available (the offset has no meaning). Otherwise only one
	 * block is in the span and the offset indicates where the
	 * free space starts.
	 */
	block_id_t m_first_block_id;
	kv_stream_sized_pos_t m_next_location;

	int m_kv_header_uid;
};

inline std::ostream& operator<< ( std::ostream& out, const KeyValueStore::iterator& value )
{
	out << "<KeyValueStore::iterator " << (value.forward() ? "forward" : "backward") << " " << (value.end() ? "END " : "") << "key:'" << value.key() << "'>";
	return out;
}

inline std::ostream& operator<< ( std::ostream& out, const KeyValueStore::const_iterator& value )
{
	out << "<KeyValueStore::const_iterator " << (value.forward() ? "forward" : "backward") << " " << (value.end() ? "END " : "") << "key:'" << value.key() << "'>";
	return out;
}

inline std::ostream& operator<< (std::ostream& out, const KeyValueStore::Search& value)
{
	if (value.valid())
		out << "<KV::Search lookup:" << value.lookup() << " locator:" << value.locator() << ">";
	else
		out << "<KV::Search invalid>";
	return out;
}

} /* end of namespace milliways */

#include "KeyValueStore.impl.hpp"

#endif /* MILLIWAYS_KEYVALUESTORE_H */
