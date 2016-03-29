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

#ifndef MILLIWAYS_BLOCKSTORAGE_H
#define MILLIWAYS_BLOCKSTORAGE_H

#include <iostream>
#include <fstream>
#include <string>
#include <array>
#include <vector>
#include <functional>

#include <stdint.h>
#include <assert.h>

#include "LRUCache.h"
#include "Utils.h"

namespace milliways {

typedef uint32_t block_id_t;

static const block_id_t BLOCK_ID_INVALID = static_cast<block_id_t>(-1);

inline bool block_id_valid(block_id_t block_id) { return (block_id != BLOCK_ID_INVALID); }

template <size_t BLOCKSIZE>
class Block
{
public:
	static const size_t BlockSize = BLOCKSIZE;

	typedef size_t size_type;

	Block(block_id_t index) :
			m_index(index), m_dirty(false) { memset(m_data, 0, sizeof(m_data)); }
	Block(const Block<BLOCKSIZE>& other) { m_index = other.m_index; m_data = other.m_data; m_dirty = other.m_dirty; }
	Block& operator= (const Block<BLOCKSIZE>& rhs) { m_index = rhs.index(); memcpy(m_data, rhs.m_data, sizeof(m_data)); m_dirty = rhs.m_dirty; return *this; }

	virtual ~Block() {}

	block_id_t index() const { return m_index; }
	block_id_t index(block_id_t value) { block_id_t old = m_index; m_index = value; return old; }

	char* data() { return m_data; }
	const char* data() const { return m_data; }

	size_type size() const { return BlockSize; }

	bool valid() const { return m_index != BLOCK_ID_INVALID; }

	bool dirty() const { return m_dirty; }
	bool dirty(bool value) { bool old = m_dirty; m_dirty = value; return old; }

private:
	Block();

	block_id_t m_index;
	char m_data[BlockSize];
	bool m_dirty;
};

template <size_t BLOCKSIZE>
class BlockStorage
{
public:
	static const int MAJOR_VERSION = 0;
	static const int MINOR_VERSION = 1;
	static const size_t MAX_USER_HEADER_LEN = 240;

	static const size_t BlockSize = BLOCKSIZE;
	typedef Block<BLOCKSIZE> block_t;
	typedef size_t size_type;

	BlockStorage() :
		m_header_block_id(BLOCK_ID_INVALID) {}
	virtual ~BlockStorage() { /* cal close() from the most derived class */ }

	/* -- General I/O ---------------------------------------------- */

	virtual bool isOpen() const = 0;
	virtual bool open();
	virtual bool close();
	virtual bool flush() = 0;
	virtual bool created() const = 0;

	virtual bool openHelper() = 0;
	virtual bool closeHelper() = 0;

	/* -- Misc ----------------------------------------------------- */

	virtual size_type count() = 0;

	/* -- Header --------------------------------------------------- */

	virtual bool readHeader();
	virtual bool writeHeader();

	int allocUserHeader() { int uid = m_user_header.size(); m_user_header.push_back(""); return uid; }
	void setUserHeader(int uid, const std::string& userHeader) { m_user_header[uid] = userHeader; }
	std::string getUserHeader(int uid) { return m_user_header[uid]; }

	/* -- Block I/O ------------------------------------------------ */

	virtual bool hasId(block_id_t block_id) = 0;

	virtual block_id_t allocId(int n_blocks = 1) = 0;
	virtual block_id_t firstId() = 0;
	block_id_t allocBlock(block_t& dst);

	virtual bool dispose(block_id_t block_id, int count = 1) = 0;
	bool dispose(block_t& block);

	virtual bool read(block_t& dst) = 0;
	virtual bool write(block_t& src) = 0;

private:
	BlockStorage(const BlockStorage& other);
	BlockStorage& operator= (const BlockStorage& other);

	block_id_t m_header_block_id;
	std::vector<std::string> m_user_header;
};

template <size_t BLOCKSIZE, size_t CACHESIZE>
class LRUBlockCache : public LRUCache< CACHESIZE, block_id_t, shptr< Block<BLOCKSIZE> > >
{
public:
	typedef block_id_t key_type;
	typedef Block<BLOCKSIZE> block_type;
	typedef shptr<block_type> block_ptr_type;
	typedef block_ptr_type mapped_type;
	typedef std::pair<key_type, mapped_type> value_type;
	typedef ordered_map<key_type, mapped_type> ordered_map_type;
	typedef LRUCache< CACHESIZE, block_id_t, shptr<block_type> > base_type;
	typedef typename base_type::size_type size_type;

	typedef BlockStorage<BLOCKSIZE>* storage_ptr_type;

	static const size_type Size = CACHESIZE;
	static const size_type BlockSize = BLOCKSIZE;

	LRUBlockCache(storage_ptr_type storage) :
		base_type(), m_storage(storage) {}

	bool on_miss(typename base_type::op_type op, const key_type& key, mapped_type& value)
	{
		// std::cerr << "block miss id:" << key << " op:" << (int)op << "\n";
		block_id_t block_id = key;
		if (m_storage->hasId(block_id)) {
			/* allocate block object and read block data from disk */
			block_type* block = new block_type(block_id);
			if (! block) return false;
			switch (op)
			{
			case base_type::op_get:
				if (! m_storage->read(*block)) return false;
				block->dirty(false);
				value.reset(block);
				break;
			case base_type::op_set:
				assert(value);
				*block = *value;
				break;
			case base_type::op_sub:
				//assert(value);
				bool rv = m_storage->read(*block);
				assert(rv || block->dirty());
				value.reset(block);
				return rv;
				break;
			}
			return true;
		}
		return false;
	}
	bool on_set(const key_type& key, const mapped_type& value)
	{
		return true;
	}
	//bool on_delete(const key_type& key);
	bool on_eviction(const key_type& key, mapped_type& value)
	{
		/* write back block */
		/* block_id_t block_id = key; */
		block_type* block = value.get();
		if (block)
		{
			if (block->valid())
				m_storage->write(*block);
			block->dirty(true);
			block->index(BLOCK_ID_INVALID);
			value.reset();
		}
		return true;
	}

private:
	storage_ptr_type m_storage;
};

template <size_t BLOCKSIZE, int CACHE_SIZE>
class FileBlockStorage : public BlockStorage<BLOCKSIZE>
{
public:
	static const size_t BlockSize = BLOCKSIZE;
	static const int CacheSize = CACHE_SIZE;

	typedef Block<BLOCKSIZE> block_t;
	typedef size_t size_type;
	typedef ssize_t ssize_type;
	typedef BlockStorage<BLOCKSIZE> base_type;

	typedef LRUBlockCache<BLOCKSIZE, CACHE_SIZE> cache_t;

	FileBlockStorage(const std::string& pathname) :
		BlockStorage<BLOCKSIZE>(),
		m_pathname(pathname), m_created(false), m_count(-1), m_next_block_id(BLOCK_ID_INVALID), m_lru(this) {}
	~FileBlockStorage() {
		close();
	}

	/* -- General I/O ---------------------------------------------- */

	bool isOpen() const { return m_stream.is_open(); }
	bool open() { return base_type::open(); }
	bool close() { return base_type::close(); }
	bool openHelper();
	bool closeHelper();
	bool flush();

	bool created() const { return m_created; }

	/* -- Misc ----------------------------------------------------- */

	size_type count();

	const std::string& pathname() const { return m_pathname; }

	/* -- Block I/O ------------------------------------------------ */

	bool hasId(block_id_t block_id) { return (block_id != BLOCK_ID_INVALID) && (block_id < nextId()); }

	block_id_t nextId();
	block_id_t allocId(int n_blocks = 1);
	block_id_t firstId() { return 0; }

	bool dispose(block_id_t block_id, int count = 1);

	bool read(block_t& dst);
	bool write(block_t& src);

	/* cached I/O */
	shptr<block_t> get(block_id_t block_id);
	bool put(const block_t& src);

protected:
	void _updateCount();

private:
	FileBlockStorage();
	FileBlockStorage(const FileBlockStorage& other);
	FileBlockStorage& operator= (const FileBlockStorage& other);

	std::string m_pathname;
	std::fstream m_stream;
	bool m_created;
	ssize_t m_count;
	block_id_t m_next_block_id;

	cache_t m_lru;
};

} /* end of namespace milliways */

#include "BlockStorage.impl.hpp"

#endif /* MILLIWAYS_BLOCKSTORAGE_H */
