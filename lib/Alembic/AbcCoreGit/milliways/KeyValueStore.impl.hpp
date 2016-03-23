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
#include "KeyValueStore.h"
#endif

#ifndef MILLIWAYS_KEYVALUESTORE_IMPL_H
//#define MILLIWAYS_KEYVALUESTORE_IMPL_H

/* ----------------------------------------------------------------- *
 *   ::seriously::Traits<milliways::DataLocator>                     *
 * ----------------------------------------------------------------- */

namespace seriously {

inline ssize_t Traits<milliways::DataLocator>::serialize(char*& dst, size_t& avail, const type& v)
{
	typedef typename type::uoffset_t uoffset_t;

	char* dstp = dst;
	size_t initial_avail = avail;

	if ((sizeof(uint32_t) + sizeof(uoffset_t)) > avail)
		return -1;

	assert((sizeof(uint32_t) + sizeof(uoffset_t)) <= avail);

	type nv(v);
	nv.normalize();
	uint32_t v_block_id = static_cast<uint32_t>(nv.block_id());
	uint16_t v_offset = static_cast<uoffset_t>(nv.uoffset());

	Traits<uint32_t>::serialize(dstp, avail, v_block_id);
	Traits<uoffset_t>::serialize(dstp, avail, v_offset);

	if (avail > 0)
		*dstp = '\0';

	dst = dstp;
	return (initial_avail - avail);
}

inline ssize_t Traits<milliways::DataLocator>::deserialize(const char*& src, size_t& avail, type& v)
{
	typedef typename type::uoffset_t uoffset_t;

	if (avail < (sizeof(uint32_t) + sizeof(uoffset_t)))
		return -1;

	const char* srcp = src;
	size_t initial_avail = avail;

	uint32_t v_block_id = milliways::BLOCK_ID_INVALID;
	uoffset_t v_offset = 0;

	if (Traits<uint32_t>::deserialize(srcp, avail, v_block_id) < 0)
		return -1;
	if (Traits<uoffset_t>::deserialize(srcp, avail, v_offset) < 0)
		return -1;

	v.block_id(v_block_id);
	v.offset(static_cast<typename type::offset_t>(v_offset));

	src = srcp;
	return (initial_avail - avail);
}

inline ssize_t Traits<milliways::SizedLocator>::serialize(char*& dst, size_t& avail, const type& v)
{
	typedef typename type::uoffset_t uoffset_t;

	char* dstp = dst;
	size_t initial_avail = avail;

	if ((sizeof(uint32_t) + sizeof(uoffset_t) + sizeof(serialized_size_type)) > avail)
		return -1;

	assert((sizeof(uint32_t) + sizeof(uoffset_t) + sizeof(serialized_size_type)) <= avail);

	type nv(v);
	nv.normalize();
	uint32_t v_block_id = static_cast<uint32_t>(nv.block_id());
	uint16_t v_offset = static_cast<uoffset_t>(nv.uoffset());
	serialized_size_type v_size = static_cast<serialized_size_type>(nv.size());

	Traits<uint32_t>::serialize(dstp, avail, v_block_id);
	Traits<uoffset_t>::serialize(dstp, avail, v_offset);
	Traits<serialized_size_type>::serialize(dstp, avail, v_size);

	if (avail > 0)
		*dstp = '\0';

	dst = dstp;
	return (initial_avail - avail);
}

inline ssize_t Traits<milliways::SizedLocator>::deserialize(const char*& src, size_t& avail, type& v)
{
	typedef typename type::uoffset_t uoffset_t;

	if (avail < (sizeof(uint32_t) + sizeof(uoffset_t) + sizeof(serialized_size_type)))
		return -1;

	const char* srcp = src;
	size_t initial_avail = avail;

	uint32_t v_block_id = milliways::BLOCK_ID_INVALID;
	uoffset_t v_offset = 0;
	serialized_size_type v_size = 0;

	if (Traits<uint32_t>::deserialize(srcp, avail, v_block_id) < 0)
		return -1;
	if (Traits<uoffset_t>::deserialize(srcp, avail, v_offset) < 0)
		return -1;
	if (Traits<serialized_size_type>::deserialize(srcp, avail, v_size) < 0)
		return -1;

	v.block_id(v_block_id);
	v.offset(static_cast<typename type::offset_t>(v_offset));
	v.size(static_cast<typename type::size_type>(v_size));

	src = srcp;
	return (initial_avail - avail);
}

} /* end of namespace seriously */


namespace milliways {

/* ----------------------------------------------------------------- *
 *   KeyValueStore                                                   *
 * ----------------------------------------------------------------- */

inline KeyValueStore::KeyValueStore(block_storage_type* blockstorage) :
	m_blockstorage(blockstorage), m_storage(NULL), m_kv_tree(NULL),
	m_first_block_id(BLOCK_ID_INVALID), m_current_block_id(BLOCK_ID_INVALID),
	m_current_block_offset(0), m_current_block_avail(0), m_current_block(NULL),
	m_kv_header_uid(-1)
{
	m_storage = new kv_tree_storage_type(m_blockstorage);
	m_kv_tree = new kv_tree_type(m_storage);

	m_kv_header_uid = m_blockstorage->allocUserHeader();
}

inline KeyValueStore::~KeyValueStore()
{
	close();

	if (m_kv_tree)
	{
		if (m_kv_tree->isOpen())
			m_kv_tree->close();
		delete m_kv_tree;
		m_kv_tree = NULL;
	}

	if (m_storage)
	{
		if (m_storage->isOpen())
			m_storage->close();
		delete m_storage;
		m_storage = NULL;
	}

	assert(! m_storage);
	assert(! m_kv_tree);
}

inline bool KeyValueStore::isOpen() const
{
	assert(m_kv_tree);
	return m_kv_tree->isOpen();
}

inline bool KeyValueStore::open()
{
	assert(m_kv_tree);
	if (isOpen())
		return true;
	bool ok = m_kv_tree->open();
	if (m_kv_tree->storage()->created())
		header_write();
	else
		header_read();
	return ok;
}

inline bool KeyValueStore::close()
{
	assert(m_kv_tree);
	if (! isOpen())
		return true;
	header_write();
	return m_kv_tree->close();
}

inline bool KeyValueStore::has(const std::string& key)
{
	DataLocator head_pos;
	return find(key, head_pos);
}

inline bool KeyValueStore::find(const std::string& key, Search& result)
{
	if (key.length() > KEY_MAX_SIZE)
	{
		result.invalidate();
		return false;
	}
	assert(key.size() <= KEY_MAX_SIZE);

	assert(m_kv_tree);
	assert(m_kv_tree->isOpen());

	// do we have this key?
	kv_tree_lookup_type& where = result.lookup();
	if (m_kv_tree->search(where, key))
	{
		assert(where.found());

		kv_tree_node_type* node = where.node();
		assert(node);

		result.dataLocator(node->value(where.pos()));
		result.envelope_size(0);
		assert(result.locator().valid());
		assert(result.valid());
	} else
	{
		result.invalidate();
		return false;
	}

	assert(result.valid());

	block_type* block = block_get(result.block_id());
	assert(block);

	/*
	 * Key-Value Storage
	 * -----------------
	 *
	 * [ key-length | value-length | key | value ]
	 *
	 * key-length: (4 bytes) key length in bytes (serialized)
	 * value-length: (4 bytes) value length in bytes (serialized)
	 * key: key data (key-length bytes)
	 * value: value data (value-length bytes)
	 */

	/*
	 * Step 1 - Read key-length and value-length
	 */

	assert((result.offset() >= 0) && (result.offset() < static_cast<SizedLocator::offset_t>(BLOCKSIZE)));
	const char* block_data = block->data();
	const char* srcp = block_data + result.offset();
	size_t avail = BLOCKSIZE - result.offset();

#if 0
	uint32_t v_key_length = 0;
	uint32_t v_value_length = 0;

	assert(avail >= (2 * sizeof(uint32_t)));
	if (seriously::Traits<uint32_t>::deserialize(srcp, avail, v_key_length) < 0)
		return false;
	if (seriously::Traits<uint32_t>::deserialize(srcp, avail, v_value_length) < 0)
		return false;
#endif

#if 1
	serialized_value_size_type v_value_length = 0;

	assert(avail >= sizeof(serialized_value_size_type));
	if (seriously::Traits<serialized_value_size_type>::deserialize(srcp, avail, v_value_length) < 0)
	{
		result.invalidate();
		return false;
	}
#endif

	result.contents_size(static_cast<SizedLocator::size_type>(v_value_length));
	return true;
}

inline bool KeyValueStore::get(const std::string& key, std::string& value)
{
	if (key.length() > KEY_MAX_SIZE)
		return false;
	assert(key.length() <= KEY_MAX_SIZE);

	Search result;
	if (! find(key, result))
		return false;

	assert(result.valid());
	assert(result.found());

	/*
	 * Step 2 - Read key
     */

	/*
     * Step 3 - Read value (contents)
     */
    SizedLocator contents_loc(result.contentsLocator());
    size_t initial = contents_loc.size();
	bool ok = read(value, contents_loc);
	size_t consumed = initial - contents_loc.size();
	result.locator().delta(consumed);
	result.locator().shrink(consumed);
	return ok;
}

inline bool KeyValueStore::get(Search& result, std::string& value, ssize_t partial)
{
	if (! result.found())
		return false;

	assert(result.valid());
	assert(result.found());

	if (result.contents_size() <= 0)
	{
		value.clear();
		return false;
	}

	SizedLocator contents_loc(result.contentsLocator());
	if (partial > 0)
		contents_loc.size(static_cast<SizedLocator::size_type>(partial));
	size_t initial = contents_loc.size();
	bool ok = read(value, contents_loc);
	size_t consumed = initial - contents_loc.size();
	result.locator().delta(consumed);
	result.locator().shrink(consumed);
	return ok;
}

inline std::string KeyValueStore::get(const std::string& key)
{
	std::string value;
	get(key, value);
	return value;
}

inline bool KeyValueStore::rename(const std::string& old_key, const std::string& new_key)
{
	if ((old_key.length() > KEY_MAX_SIZE) || (new_key.length() > KEY_MAX_SIZE))
		return false;

	DataLocator head_pos;
	if (! find(old_key, head_pos))
		return false;

	assert(head_pos.valid());

	kv_tree_lookup_type where_old;
	if (! m_kv_tree->insert(new_key, head_pos))
		return false;
	if (! m_kv_tree->remove(where_old, old_key))
	{
		kv_tree_lookup_type where_new;
		m_kv_tree->remove(where_new, new_key);
		return false;
	}
	return true;
}

inline bool KeyValueStore::put(const std::string& key, const std::string& value, bool overwrite)
{
	if (key.length() > KEY_MAX_SIZE)
		return false;
	assert(key.length() <= KEY_MAX_SIZE);

	Search result;
	bool present = find(key, result);

	block_type* head_block = NULL;
	bool do_allocate = true;

	if (present)
	{
		/* present */
		assert(result.valid());
		assert(result.found());

		if (! overwrite)
			return false;

		do_allocate = (value.length() > result.contents_size()) ? true : false;
	} else
	{
		/* not present */
		assert(! result.valid());
		assert(! result.found());
		do_allocate = true;
	}

	if (do_allocate)
	{
		// -- allocate a new place --
		head_block = NULL;
		result.contents_size(value.length());
		assert(result.envelope_size() == value.length() + sizeof(serialized_value_size_type));
		alloc_value_envelope(result.locator());
		if (! result.locator().valid())
			return false;
		assert(result.contents_size() == value.length());
	}

	// --  write value length and data --

	// DataLocator dst_pos(head_pos);

	// serialize value length
	if (! head_block)
		head_block = block_get(result.block_id());
	assert(head_block);
	char *dstp = head_block->data() + result.offset();
	size_t avail = static_cast<size_t>(BLOCKSIZE - result.offset());
	serialized_value_size_type v_value_length = static_cast<serialized_value_size_type>(value.length());
	seriously::Traits<serialized_value_size_type>::serialize(dstp, avail, v_value_length);

	// write value string
	SizedLocator contents_loc(result.contentsLocator());
	bool ok = write(value, contents_loc);

	// -- update the key-value map --

	if (do_allocate)
	{
		assert(m_kv_tree);
		if (present)
		{
			assert(overwrite);
			if (! m_kv_tree->update(key, result.headDataLocator()))
				return false;
		} else
		{
			if (! m_kv_tree->insert(key, result.headDataLocator()))
				return false;
		}
	}

	return ok;
}

inline bool KeyValueStore::find(const std::string& key, DataLocator& data_pos)
{
	assert(m_kv_tree);
	assert(m_kv_tree->isOpen());

	if (key.length() > KEY_MAX_SIZE)
	{
		data_pos.invalidate();
		return false;
	}
	assert(key.size() <= KEY_MAX_SIZE);

	// do we have this key?
	kv_tree_lookup_type where;
	if (m_kv_tree->search(where, key))
	{
		assert(where.found());

		kv_tree_node_type* node = where.node();
		assert(node);

		data_pos = node->value(where.pos());
		assert(data_pos.valid());
		return true;
	}

	data_pos.invalidate();
	return false;
}

inline bool KeyValueStore::find(const std::string& key, SizedLocator& sized_pos)
{
	if (key.length() > KEY_MAX_SIZE)
	{
		sized_pos.invalidate();
		return false;
	}
	assert(key.size() <= KEY_MAX_SIZE);

	assert(m_kv_tree);
	assert(m_kv_tree->isOpen());

	// do we have this key?
	kv_tree_lookup_type where;
	if (m_kv_tree->search(where, key))
	{
		assert(where.found());

		kv_tree_node_type* node = where.node();
		assert(node);

		sized_pos.dataLocator(node->value(where.pos()));
		sized_pos.envelope_size(0);
		assert(sized_pos.valid());
	} else
	{
		sized_pos.invalidate();
		return false;
	}

	assert(sized_pos.valid());

	block_type* block = block_get(sized_pos.block_id());
	assert(block);

	/*
	 * Key-Value Storage
	 * -----------------
	 *
	 * [ key-length | value-length | key | value ]
	 *
	 * key-length: (4 bytes) key length in bytes (serialized)
	 * value-length: (4 bytes) value length in bytes (serialized)
	 * key: key data (key-length bytes)
	 * value: value data (value-length bytes)
	 */

	/*
	 * Step 1 - Read key-length and value-length
	 */

	assert((sized_pos.offset() >= 0) && (sized_pos.offset() < static_cast<SizedLocator::offset_t>(BLOCKSIZE)));
	const char* block_data = block->data();
	const char* srcp = block_data + sized_pos.offset();
	size_t avail = BLOCKSIZE - sized_pos.offset();

#if 0
	uint32_t v_key_length = 0;
	uint32_t v_value_length = 0;

	assert(avail >= (2 * sizeof(uint32_t)));
	if (seriously::Traits<uint32_t>::deserialize(srcp, avail, v_key_length) < 0)
		return false;
	if (seriously::Traits<uint32_t>::deserialize(srcp, avail, v_value_length) < 0)
		return false;
#endif

#if 1
	serialized_value_size_type v_value_length = 0;

	assert(avail >= sizeof(serialized_value_size_type));
	if (seriously::Traits<serialized_value_size_type>::deserialize(srcp, avail, v_value_length) < 0)
	{
		sized_pos.invalidate();
		return false;
	}
#endif

	sized_pos.contents_size(static_cast<SizedLocator::size_type>(v_value_length));
	return true;
}

#define KV_FAST_BUFFER_SIZE    8192

inline bool KeyValueStore::read(std::string& dst, SizedLocator& location)
{
	assert(m_blockstorage);
	assert(isOpen());

	dst.clear();

	size_t length = location.envelope_size();

	char fast_data[KV_FAST_BUFFER_SIZE];
	char *dst_data;
	if ((length + 1) < sizeof(fast_data))
		dst_data = fast_data;
	else
		dst_data = new char[length + 1];
	char *dstp = dst_data;

	location.normalize();
	block_id_t  src_block_id = location.block_id();
	uint32_t    src_offset   = static_cast<uint32_t>(location.uoffset());
	size_t      src_rem      = length;
	block_type *src_block    = NULL;
	//uint32_t    dst_offset   = 0;
	size_t      nread        = 0;

	while (src_rem > 0)
	{
		assert(src_rem > 0);
		if ((! src_block) || (src_block->index() != src_block_id))
			src_block = block_get(src_block_id);
		assert(src_block);
		assert(src_block->index() == src_block_id);
		size_t src_block_avail = min(static_cast<size_t>(BLOCKSIZE - src_offset), src_rem);
		assert(src_block_avail <= src_rem);
		size_t amount = min(src_block_avail, src_rem);
		memcpy(dstp, src_block->data() + src_offset, amount);
		dstp       += amount;
		src_rem    -= amount;
		src_offset += amount;
		nread      += amount;
		if (src_offset >= BLOCKSIZE)
		{
			src_block_id++;
			src_block = NULL;
			src_offset = 0;
		}
	}
	*dstp = '\0';
	assert(src_rem == 0);

	assert(nread >= length);

	dst.reserve(nread);
	dst.resize(nread);
	dst.assign(dst_data, nread);
	if (dst_data != fast_data)
	{
		delete[] dst_data;
		dst_data = NULL;
	}

	location.delta(nread);
	location.shrink(nread);
	return (nread == length) ? true : false;
}

inline bool KeyValueStore::write(const std::string& src, SizedLocator& location)
{
	assert(m_blockstorage);
	assert(isOpen());

	size_t dst_avail = location.envelope_size();

	if (dst_avail < src.length())
		return false;

	assert(dst_avail >= src.length());

	location.normalize();
	block_id_t  dst_block_id = location.block_id();
	uint32_t    dst_offset   = static_cast<uint32_t>(location.uoffset());
	block_type *dst_block    = NULL;
	const char *srcp         = src.data();
	size_t      src_rem      = src.length();
	size_t      nwritten     = 0;

	while ((src_rem > 0) && (dst_avail > 0))
	{
		assert(src_rem > 0);
		assert(dst_avail > 0);

		if ((! dst_block) || (dst_block->index() != dst_block_id))
			dst_block = block_get(dst_block_id);
		assert(dst_block);
		assert(dst_block->index() == dst_block_id);

		size_t dst_block_avail = min(static_cast<size_t>(BLOCKSIZE - dst_offset), dst_avail);
		assert(dst_block_avail <= dst_avail);

		size_t amount = min(dst_block_avail, src_rem);
		assert(dst_offset + amount <= BLOCKSIZE);
		memcpy(dst_block->data() + dst_offset, srcp, amount);
		block_put(*dst_block);
		srcp       += amount;
		src_rem    -= amount;
		dst_offset += amount;
		dst_avail  -= amount;
		nwritten   += amount;
		if (dst_offset >= BLOCKSIZE)
		{
			dst_block_id++;
			dst_block = NULL;
			dst_offset = 0;
		}
	}
	assert(src_rem == 0);
	assert(dst_avail >= 0);

	location.delta(nwritten);
	location.shrink(nwritten);
	return (nwritten == src.length()) ? true : false;
}

inline bool KeyValueStore::alloc_value_envelope(SizedLocator& dst)
{
	size_t amount = dst.envelope_size();
	if ((! block_id_valid(m_current_block_id)) || (m_current_block_avail < amount))
	{
		size_t n_blocks = size_in_blocks(amount);
		assert((n_blocks * BLOCKSIZE) >= amount);
		m_current_block = NULL;
		m_current_block_id = block_alloc_id(n_blocks);
		if (! block_id_valid(m_current_block_id))
			return false;
		m_current_block_offset = 0;
		m_current_block_avail = n_blocks * BLOCKSIZE;
		if (! block_id_valid(m_first_block_id))
			m_first_block_id = m_current_block_id;
	}

	assert(block_id_valid(m_first_block_id));
	assert(block_id_valid(m_current_block_id));

	if (! m_current_block)
		m_current_block = block_get(m_current_block_id);

	assert(m_current_block);

	assert(m_current_block_avail >= amount);
	assert(m_current_block_offset >= 0);
	if (amount <= BLOCKSIZE)
	{
		assert(m_current_block_offset <= (BLOCKSIZE - amount));
	}

	// set dst SizedLocator to allocated space
	dst.block_id(m_current_block_id);
	dst.offset(m_current_block_offset);

	// compute next block id / avail
	block_id_t old_block_id = m_current_block_id;
	m_current_block_offset += (amount % BLOCKSIZE);
	m_current_block_id     += (amount / BLOCKSIZE);
	m_current_block_avail  -= (amount % BLOCKSIZE);
	if (m_current_block_id != old_block_id)
		m_current_block = NULL;
	// std::cerr << "m_current_block id:" << m_current_block_id << " offset:" << m_current_block_offset << " avail:" <<  m_current_block_avail << std::endl;
	return true;
}

inline size_t KeyValueStore::size_in_blocks(size_t size)
{
	return ((size + BLOCKSIZE - 1) / BLOCKSIZE);
}

/* -- Header I/O ----------------------------------------------- */

#define MAX_USER_HEADER 240

inline bool KeyValueStore::header_write()
{
	if (! isOpen())
		return false;
	assert(isOpen());
	assert(m_blockstorage);

	seriously::Packer<MAX_USER_HEADER> packer;
	packer << static_cast<uint32_t>(MAJOR_VERSION) << static_cast<uint32_t>(MINOR_VERSION) <<
	 	static_cast<uint32_t>(BLOCKSIZE) << static_cast<uint32_t>(B) <<
	 	static_cast<uint32_t>(KEY_MAX_SIZE);

	packer << m_first_block_id << m_current_block_id << m_current_block_offset << m_current_block_avail;

	std::string userHeader(packer.data(), packer.size());
	m_blockstorage->setUserHeader(m_kv_header_uid, userHeader);

	// std::cerr << "-> KV WRITE VER:" << MAJOR_VERSION << "." << MINOR_VERSION << " BLOCKSIZE:" << BLOCKSIZE <<
	// 	" B:" << B << std::endl;
	// std::cerr << "  first block id:" << m_first_block_id << "  current block id:" << m_current_block_id << " offset:" << m_current_block_offset << " avail:" << m_current_block_avail << std::endl;

//	std::cerr << "W KV userHeader[" << m_kv_header_uid << "]:" << std::endl;
//	std::cerr << s_hexdump(userHeader.data(), userHeader.size()) << std::endl;

	return true;
}

inline bool KeyValueStore::header_read()
{
	if (! isOpen())
		return false;
	assert(isOpen());
	assert(m_blockstorage);

	std::string userHeader = m_blockstorage->getUserHeader(m_kv_header_uid);

	seriously::Packer<MAX_USER_HEADER> packer(userHeader);

//	std::cerr << "R userHeader[" << m_kv_header_uid << "]:" << std::endl;
//	std::cerr << s_hexdump(userHeader.data(), userHeader.size()) << std::endl;

	uint32_t v_MAJOR, v_MINOR, v_BLOCKSIZE, v_B, v_KEY_MAX_SIZE;

	packer >> v_MAJOR >> v_MINOR >> v_BLOCKSIZE >> v_B >> v_KEY_MAX_SIZE;

	// std::cerr << "-> KV READ VER:" << v_MAJOR << "." << v_MINOR << " BLOCKSIZE:" << v_BLOCKSIZE <<
	// 	" B:" << v_B << std::endl;

	if (v_MAJOR > MAJOR_VERSION)
	{
		std::cerr << "ERROR: '" << m_blockstorage->pathname() << "' version not supported (found:" <<
			v_MAJOR << "." << v_MINOR <<
			" library:" << MAJOR_VERSION << "." << MINOR_VERSION << ")" << std::endl;
		return false;
	}

	if ((v_B != B) || (v_BLOCKSIZE != BLOCKSIZE))
	{
		std::cerr << "ERROR: '" << m_blockstorage->pathname() << "' doesn't match with kv btree properties (B/BLOCKSIZE)" << std::endl;
		return false;
	}

	assert(v_B == B);
	assert(v_BLOCKSIZE == BLOCKSIZE);
	assert(v_MAJOR <= MAJOR_VERSION);

	packer >> m_first_block_id >> m_current_block_id >> m_current_block_offset >> m_current_block_avail;
	// std::cerr << "  first block id:" << m_first_block_id << "  current block id:" << m_current_block_id << " offset:" << m_current_block_offset << " avail:" << m_current_block_avail << std::endl;

	return true;
}

} /* end of namespace milliways */

#endif /* MILLIWAYS_KEYVALUESTORE_IMPL_H */
