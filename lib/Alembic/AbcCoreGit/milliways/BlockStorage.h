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

#include <map>
#include <unordered_map>
#include <memory>

#include <stdint.h>
#include <assert.h>

#include "LRUCache.h"
#include "Utils.h"

namespace milliways {

typedef uint32_t block_id_t;

static const block_id_t BLOCK_ID_INVALID = static_cast<block_id_t>(-1);

inline bool block_id_valid(block_id_t block_id) { return (block_id != BLOCK_ID_INVALID); }

template <size_t BLOCKSIZE>
class BlockManager;

template <size_t BLOCKSIZE>
class Block
{
public:
	static const size_t BlockSize = BLOCKSIZE;

	typedef size_t size_type;

	// Block(block_id_t index_) :
	// 		m_index(index_), m_dirty(false) { memset(m_data, 0, sizeof(m_data)); }
	// Block(const Block<BLOCKSIZE>& other) : m_index(other.m_index), m_data(other.m_data), m_dirty(other.m_dirty) { }
	// Block& operator= (const Block<BLOCKSIZE>& rhs) { assert(this != &rhs); m_index = rhs.index(); memcpy(m_data, rhs.m_data, sizeof(m_data)); m_dirty = rhs.m_dirty; return *this; }
	// ~Block() {}

	Block& operator= (const Block<BLOCKSIZE>& rhs) { assert(this != &rhs); m_index = rhs.index(); memcpy(m_data, rhs.m_data, sizeof(m_data)); m_dirty = rhs.m_dirty; return *this; }

	block_id_t index() const { return m_index; }
	block_id_t index(block_id_t value) { block_id_t old = m_index; m_index = value; return old; }

	char* data() { return m_data; }
	const char* data() const { return m_data; }

	size_type size() const { return BlockSize; }

	bool valid() const { return m_index != BLOCK_ID_INVALID; }

	bool dirty() const { return m_dirty; }
	bool dirty(bool value) { bool old = m_dirty; m_dirty = value; return old; }

private:
	/* lifetime managed by BlockManager */
	Block() {}
	Block(block_id_t index_) :
			m_index(index_), m_dirty(false) { memset(m_data, 0, sizeof(m_data)); }
	Block(const Block<BLOCKSIZE>& other) : m_index(other.m_index), m_data(other.m_data), m_dirty(other.m_dirty) { }
	~Block() {}

	friend class BlockManager<BLOCKSIZE>;
	friend class BlockManager<BLOCKSIZE>::block_deleter;
	template < size_t BLOCKSIZE_, int B_, typename KeyTraits, typename TTraits, class Compare >
		friend class BTreeFileStorage;

	block_id_t m_index;
	char m_data[BlockSize];
	bool m_dirty;
};

/* ----------------------------------------------------------------- *
 *   BlockManager                                                    *
 * ----------------------------------------------------------------- */

/*
 * Based on http://stackoverflow.com/a/15708286
 *   http://stackoverflow.com/questions/15707991/good-design-pattern-for-manager-handler
 */
template <size_t BLOCKSIZE>
class BlockManager
{
public:
	typedef Block<BLOCKSIZE> block_type;
	typedef BlockManager<BLOCKSIZE> handler_type;
	typedef std::unordered_map< block_id_t, std::weak_ptr<block_type> > weak_map_t;
	typedef typename weak_map_t::iterator weak_map_iter;
	typedef typename weak_map_t::const_iterator weak_map_citer;

	BlockManager() : m_objects() {}
	~BlockManager() {
		weak_map_iter it;
		for (it = m_objects.begin(); it != m_objects.end(); ++it) {
			std::weak_ptr<block_type> wp = it->second;
			if ((wp.use_count() > 0) && wp.expired()) {
				std::cerr << "WARNING: block weak pointer expired BUT use count not zero for managed block! (in use:" << wp.use_count() << ")" << std::endl;
			} else if (wp.use_count() > 0) {
				std::cerr << "WARNING: block use count not zero for managed block (in use:" << wp.use_count() << ")" << std::endl;
			}
		}
		m_objects.clear();
	}

	std::shared_ptr<block_type> get_object(block_id_t id, bool createIfNotFound = true)
	{
		weak_map_citer it = m_objects.find(id);
		if (it != m_objects.end()) {
			assert(it->first == id);
			return it->second.lock();
		} else if (createIfNotFound) {
			return make_object(id);
		} else
			return std::shared_ptr<block_type>();
	}

	bool has(block_type id) {
		weak_map_citer it = m_objects.find(id);
		return (it != m_objects.end()) ? true : false;
	}

	size_t count() const { return m_objects.size(); }

private:
	friend class Block<BLOCKSIZE>;

	class block_deleter;
	friend class block_deleter;

	std::shared_ptr<block_type> make_object(block_id_t id)
	{
		assert(m_objects.count(id) == 0);
		std::shared_ptr<block_type> sp(new block_type(id), block_deleter(this, id));

		m_objects[id] = sp;

		return sp;
	}

	/* custom block deleter */
	class block_deleter
	{
	public:
		block_deleter(handler_type* handler, block_id_t id) :
			m_handler(handler), m_id(id) {}

		void operator()(block_type* p) {
			assert(m_handler);
			assert(p);
			assert(p->index() == m_id);
			m_handler->m_objects.erase(m_id);
			delete p;
		}
	private:
		handler_type* m_handler;
		block_id_t m_id;
	};

	weak_map_t m_objects;
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
	typedef BlockManager<BLOCKSIZE> manager_type;

	BlockStorage() :
		m_header_block_id(BLOCK_ID_INVALID), m_user_header(), m_manager() {}
	virtual ~BlockStorage() { /* call close() from the most derived class, and BEFORE destruction  */ }

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

	int allocUserHeader() { int uid = static_cast<int>(m_user_header.size()); m_user_header.push_back(""); return uid; }
	void setUserHeader(int uid, const std::string& userHeader) { m_user_header[static_cast<size_type>(uid)] = userHeader; }
	std::string getUserHeader(int uid) { return m_user_header[static_cast<size_type>(uid)]; }

	/* -- Block I/O ------------------------------------------------ */

	virtual bool hasId(block_id_t block_id) = 0;

	virtual block_id_t allocId(int n_blocks = 1) = 0;
	virtual block_id_t firstId() = 0;
	block_id_t allocBlock(block_t& dst);

	virtual bool dispose(block_id_t block_id, int count = 1) = 0;
	bool dispose(block_t& block);

	virtual bool read(block_t& dst) = 0;
	virtual bool write(block_t& src) = 0;

	/* -- Node Manager --------------------------------------------- */

	manager_type& manager() { return m_manager; }

private:
	BlockStorage(const BlockStorage& other);
	BlockStorage& operator= (const BlockStorage& other);

	block_id_t m_header_block_id;
	std::vector<std::string> m_user_header;
	manager_type m_manager;
};

template <size_t BLOCKSIZE, size_t CACHESIZE>
class LRUBlockCache : public LRUCache< CACHESIZE, block_id_t, MW_SHPTR< Block<BLOCKSIZE> > >
{
public:
	typedef block_id_t key_type;
	typedef Block<BLOCKSIZE> block_type;
	typedef MW_SHPTR<block_type> block_ptr_type;
	typedef block_ptr_type mapped_type;
	typedef std::pair<key_type, mapped_type> value_type;
	typedef LRUCache< CACHESIZE, block_id_t, MW_SHPTR<block_type> > base_type;
	typedef typename base_type::size_type size_type;

	typedef BlockStorage<BLOCKSIZE>* storage_ptr_type;

	static const size_type Size = CACHESIZE;
	static const size_type BlockSize = BLOCKSIZE;
	static const block_id_t InvalidCacheKey = BLOCK_ID_INVALID;

	LRUBlockCache(storage_ptr_type storage) :
		base_type(LRUBlockCache::InvalidCacheKey), m_storage(storage) {}

	bool on_miss(typename base_type::op_type op, const key_type& key, mapped_type& value)
	{
		// std::cerr << "block miss id:" << key << " op:" << (int)op << "\n";
		block_id_t block_id = key;
		if (m_storage->hasId(block_id)) {
			/* allocate block object and read block data from disk */
			MW_SHPTR<block_type> block_ptr;
			// if (! block) return false;
			bool rv = false;
			switch (op)
			{
			case base_type::op_get:
				block_ptr = m_storage->manager().get_object(block_id);
				assert(block_ptr && (block_ptr->index() == block_id));
				if (! block_ptr) return false;
				rv = m_storage->read(*block_ptr);
				assert(rv || block_ptr->dirty());
				value = block_ptr;
				return rv;
				// if (! m_storage->read(*block)) return false;
				// block->dirty(false);
				// value.reset(block);
				break;
			case base_type::op_set:
				// *block = *value;
				rv = true;
				break;
			case base_type::op_sub:
				block_ptr = m_storage->manager().get_object(block_id);
				assert(block_ptr && (block_ptr->index() == block_id));
				if (! block_ptr) return false;
				//assert(value);
				rv = m_storage->read(*block_ptr);
				assert(rv || block_ptr->dirty());
				value = block_ptr;
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
		/* block_id_t block_id = key; */
		block_type* block = value.get();
		if (block)
		{
			if (block->valid())
			{
#ifdef NDEBUG
				m_storage->write(*block);
#else
				bool ok = m_storage->write(*block);
				assert(ok);
#endif
				// if (ok) block->dirty(false);
			}
			// block->dirty(true);
			// block->index(BLOCK_ID_INVALID);
			// value.reset();
		}
		return true;
	}

private:
	LRUBlockCache(const LRUBlockCache&) {}
	LRUBlockCache& operator= (const LRUBlockCache&) {}

	storage_ptr_type m_storage;
};

template <size_t BLOCKSIZE>
struct StreamPos;

template <size_t BLOCKSIZE>
struct StreamSizedPos;

template <size_t BLOCKSIZE, int CACHE_SIZE>
class WriteStream;

template <size_t BLOCKSIZE, int CACHE_SIZE>
class ReadStream;

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

	typedef StreamPos<BLOCKSIZE> stream_pos_t;
	typedef StreamSizedPos<BLOCKSIZE> stream_sized_pos_t;
	typedef WriteStream<BLOCKSIZE, CACHE_SIZE> write_stream_t;
	typedef ReadStream<BLOCKSIZE, CACHE_SIZE> read_stream_t;

	FileBlockStorage(const std::string& pathname_) :
		BlockStorage<BLOCKSIZE>(),
		m_pathname(pathname_), m_stream(), m_created(false), m_count(-1), m_next_block_id(BLOCK_ID_INVALID), m_lru(this) {}
	~FileBlockStorage(); 	/* call close() before destruction! */

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
	MW_SHPTR<block_t> get(block_id_t block_id, bool createIfNotFound = true);
	bool put(const block_t& src);

	/* -- Streaming I/O -------------------------------------------- */

	/* streaming read */
	bool read_lz4(read_stream_t& rs, char*& dstp, size_t nbytes, size_t& compressed_size);
	bool read_lz4(read_stream_t& rs, std::string& dst, size_t nbytes, size_t& compressed_size);

	/* streaming write */
	bool write_lz4(write_stream_t& ws, const char*& srcp, size_t nbytes, size_t& compressed_size);
	bool write_lz4(write_stream_t& ws, const std::string& src, size_t& compressed_size) { const char *srcp = src.data(); return write_lz4(ws, srcp, src.length(), compressed_size); }

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

/* ----------------------------------------------------------------- *
 *   Streaming                                                       *
 * ----------------------------------------------------------------- */

/* ----------------------------------------------------------------- *
 *   StreamPos                                                       *
 * ----------------------------------------------------------------- */

template <size_t BLOCKSIZE>
struct StreamPos
{
public:
	static const size_t BlockSize = BLOCKSIZE;

	typedef ssize_t offset_t;
	typedef uint32_t block_offset_t;

	StreamPos() :
		m_pos(-1) {}
	StreamPos(block_id_t block_id_, block_offset_t offset_) :
		m_pos(0) { from_block_offset(block_id_, offset_); }
	StreamPos(offset_t linear_pos_) :
		m_pos(linear_pos_) {}
	StreamPos(size_t linear_pos_) :
		m_pos(static_cast<offset_t>(linear_pos_)) {}
	StreamPos(const StreamPos& other) :
		m_pos(other.m_pos) {}
	StreamPos(const StreamPos& other, offset_t delta_) :
			m_pos(other.m_pos) { delta(delta_); }
	~StreamPos() {}
	StreamPos& operator= (const StreamPos& other) { m_pos = other.m_pos; return *this; }

	bool operator== (const StreamPos& rhs) const { return (((m_pos < 0) && (rhs.m_pos < 0)) || (m_pos == rhs.m_pos)); }
	bool operator!= (const StreamPos& rhs) const { return (! (*this == rhs)); }
	bool operator< (const StreamPos& rhs) const {
		return m_pos < rhs.m_pos;
	}
	StreamPos& operator+= (const offset_t& delta_) { return delta(delta_); }
	StreamPos& operator-= (const offset_t& delta_) { return delta(-delta_); }
	// StreamPos operator+ (const offset_t& delta_) { StreamPos n(*this); n += delta_; return n; }
	// StreamPos operator- (const offset_t& delta_) { StreamPos n(*this); n -= delta_; return n; }

	operator bool() const { return valid(); }

	bool valid() const { return (m_pos >= 0); }

	StreamPos& invalidate() { m_pos = -1; return *this; }

	offset_t pos() const { return m_pos; }
	offset_t pos(offset_t value) { offset_t old = m_pos; m_pos = value; return old; }

	block_id_t block_id() const { return static_cast<block_id_t>(m_pos / static_cast<offset_t>(BlockSize)); }
	block_offset_t offset() const { return static_cast<block_offset_t>(m_pos % static_cast<offset_t>(BlockSize)); }

	StreamPos& from_block_offset(block_id_t block_id_, block_offset_t offset_) {
		m_pos = static_cast<offset_t>(block_id_) * static_cast<offset_t>(BlockSize) + static_cast<offset_t>(offset_);
		return *this;
	}

	StreamPos& delta(offset_t delta_) { m_pos += delta_; return *this; }

protected:
	offset_t m_pos;
};

template <size_t BLOCKSIZE>
inline std::ostream& operator<< (std::ostream& out, const StreamPos<BLOCKSIZE>& value)
{
	if (value.valid())
		out << "<StreamPos pos:" << value.pos() << " (block:" << value.block_id() << " offset:" << (int)value.offset() << ")>";
	else
		out << "<StreamPos invalid>";
	return out;
}

/* ----------------------------------------------------------------- *
 *   StreamSizedPos                                                  *
 *     size + position                                               *
 * ----------------------------------------------------------------- */

template <size_t BLOCKSIZE>
struct StreamSizedPos : public StreamPos<BLOCKSIZE>
{
public:
	typedef StreamPos<BLOCKSIZE> base_type;
	typedef StreamPos<BLOCKSIZE> data_locator_t;
	typedef XTYPENAME StreamPos<BLOCKSIZE>::offset_t offset_t;
	typedef size_t size_type;

	StreamSizedPos() : base_type(), m_full_size(0) {}
	StreamSizedPos(offset_t linear_pos_, size_t size_) :
		base_type(linear_pos_), m_full_size(size_) {}
	StreamSizedPos(size_t linear_pos_, size_t size_) :
		base_type(linear_pos_), m_full_size(size_) {}
	StreamSizedPos(const StreamSizedPos& other) :
		base_type(other.block_id(), other.offset()), m_full_size(other.m_full_size) {}
	StreamSizedPos(const data_locator_t& dataLocator_, size_t size_) :
		base_type(dataLocator_), m_full_size(size_) {}
	StreamSizedPos(const StreamSizedPos& other, offset_t delta_) :
		base_type(other.block_id(), other.offset()), m_full_size(other.m_full_size) { delta(delta_); }
	StreamSizedPos& operator=(const StreamSizedPos& other) { this->m_pos = other.m_pos; m_full_size = other.m_full_size; return *this; }
	StreamSizedPos& operator=(const data_locator_t& dl) { this->m_pos = dl.pos(); return *this; }

	bool operator==(const StreamSizedPos& rhs) const
	{
		return (((this->m_pos < 0) && (rhs.m_pos < 0)) ||
		        ((this->m_pos == rhs.m_pos) && (m_full_size == rhs.m_full_size)));
	}
	bool operator!=(const StreamSizedPos& rhs) const { return (!(*this == rhs)); }
	bool operator<(const StreamSizedPos& rhs) const {
		if ((this->m_pos < 0) && (rhs.m_pos < 0))
			return false;
		if (this->m_pos < rhs.m_pos)
			return true;
		else if (this->m_pos == rhs.m_pos)
			return (m_full_size < rhs.m_full_size);
		return false;
	}

	offset_t end_pos() const {
		return this->m_pos + static_cast<offset_t>(m_full_size) - 1;
	}

	data_locator_t end() const { return data_locator_t(end_pos()); }

	bool follows(const StreamSizedPos& other) const {
		/* check if strictly follows 'other', without holes */
		return ((other.end_pos() + 1) == this->pos());
	}
	bool precedes(const StreamSizedPos& other) const { return other.follows(*this); }

	bool merge(StreamSizedPos& following) {
		if (! precedes(following))
			return false;
		size_type amount = following.size();
		grow(amount);
		following.shrink(amount);
		assert(following.size() == 0);
		return true;
	}

	data_locator_t dataLocator() const { return data_locator_t(this->m_pos); }
	StreamSizedPos& dataLocator(const data_locator_t& value) { this->pos(value.pos()); return *this; }

	size_type size() const { return m_full_size; }
	size_type size(size_type value) { size_type old = m_full_size; m_full_size = value; return old; }

	StreamSizedPos& shrink(size_type value) { m_full_size -= value; return *this; }
	StreamSizedPos& grow(size_type value) { m_full_size += value; return *this; }

	StreamSizedPos& consume(size_type value) { shrink(value); this->delta(static_cast<offset_t>(value)); return *this; }

protected:
	size_type m_full_size;				// enveloped size: size + payload
};

template <size_t BLOCKSIZE>
inline std::ostream& operator<< (std::ostream& out, const StreamSizedPos<BLOCKSIZE>& value)
{
	if (value.valid())
		out << "<StreamSizedPos pos:" << value.pos() << " (block:" << value.block_id() << " offset:" << (int)value.offset() << ") size:" << value.size() << ">";
	else
		out << "<StreamSizedPos invalid>";
	return out;
}

enum seek_t { seek_start, seek_current, seek_end };

class IOBuffer
{
public:
	IOBuffer(char *bufp, size_t len) :
		m_bufp(bufp), m_len(len) {}
	~IOBuffer() {}

	char *data()             { return m_bufp; }
	const char *data() const { return m_bufp; }
	size_t length() const    { return m_len; }

private:
	IOBuffer();
	IOBuffer(const IOBuffer& other);

	char   *m_bufp;
	size_t  m_len;
};

class IOExtString
{
public:
	IOExtString(std::string& str, size_t amount_) :
		m_str(str), m_amount(amount_) {}
	~IOExtString() {}

	std::string& string()             { return m_str; }
	const std::string& string() const { return m_str; }
	size_t amount() const             { return m_amount; }

private:
	IOExtString();
	IOExtString(const IOExtString& other);

	std::string& m_str;
	size_t       m_amount;
};

template <size_t BLOCKSIZE, int CACHE_SIZE>
class WriteStream
{
public:
	typedef StreamPos<BLOCKSIZE> data_locator_t;
	typedef StreamSizedPos<BLOCKSIZE> sized_locator_t;
	typedef XTYPENAME sized_locator_t::offset_t offset_t;
	typedef XTYPENAME StreamPos<BLOCKSIZE>::block_offset_t block_offset_t;
	typedef FileBlockStorage<BLOCKSIZE, CACHE_SIZE> block_storage_t;
	typedef XTYPENAME block_storage_t::block_t block_type;

	WriteStream(block_storage_t* bs, const sized_locator_t& location_) :
		m_bs(bs), m_location_start(location_), m_location(location_),
		m_dstp(NULL), m_dst_block(),
		m_nwritten(0), m_fail(false) {}
	~WriteStream() { flush(); m_dst_block.reset(); }

	const sized_locator_t& location_start() const { return m_location_start; }

	const sized_locator_t& location() const { return m_location; }
	sized_locator_t& location() { return m_location; }
	WriteStream& location(sized_locator_t& location_) { m_location = location_; return *this; }

	size_t  nwritten() const { return m_nwritten; }
	bool    eof() const { return (m_location.size() <= 0); }
	bool    fail() const { return m_fail; }
	bool    fail(bool value) { bool old = m_fail; m_fail = value; assert(!m_fail); return old; }
	size_t  avail() const { return m_location.size(); }

	void    clearfail() { m_fail = false; }

	bool    seek(ssize_t off, seek_t seek_type);

	ssize_t write(const char *srcp, size_t src_length);
	ssize_t write(const std::string& src) { return write(src.data(), src.length()); }
	void    flush() {
		if (m_dst_block) {
			m_bs->put(*m_dst_block);
			m_dst_block.reset();
		}
	}

	template<typename T>
	ssize_t write(const T value) {
		char buffer[sizeof(T)];
		char* dstp = buffer;
		size_t bufsize = sizeof(buffer);
		memset(buffer, 0, sizeof(buffer));
		ssize_t rc = seriously::Traits<T>::serialize(dstp, bufsize, value);
		if (rc < 0) {
			fail(true);
			return rc;
		}
		return this->write(buffer, sizeof(T));
	}

private:
	WriteStream();
	WriteStream(const WriteStream&);
	WriteStream& operator= (const WriteStream&) { return *this; }

	bool fetch_block() {
		if ((! m_dst_block) || (m_dst_block->index() != m_location.block_id())) {
			if (m_dst_block) {
				m_bs->put(*m_dst_block);
			}
			m_dst_block = m_bs->get(m_location.block_id());
			m_dstp = m_dst_block->data() + m_location.offset();
			if (! m_dst_block)
				fail(true);
			return m_dst_block ? true : false;
		}
		return true;
	}

	block_id_t     dst_block_id() const    { return m_location.block_id(); }
	block_offset_t dst_offset() const      { return m_location.offset(); }
	MW_SHPTR<block_type>& dst_block()      { return m_dst_block; }

	block_storage_t*     m_bs;
	sized_locator_t      m_location_start;
	sized_locator_t      m_location;
	char*                m_dstp;
	MW_SHPTR<block_type> m_dst_block;
	size_t               m_nwritten;
	bool                 m_fail;
};

/*
 * function template partial specialization is forbidden in C++, so we
 * use a functor as an helper class
 */
template <typename T, size_t BLOCKSIZE, int CACHE_SIZE>
struct WriteStreamOutFunctor {
	static WriteStream<BLOCKSIZE, CACHE_SIZE>& write(WriteStream<BLOCKSIZE, CACHE_SIZE>& out, const T& value) {
#ifdef NDEBUG
		out.write(value);
#else
		ssize_t nwritten = out.write(value);
		assert(nwritten == sizeof(T));
#endif
		return out;
	}
};

/* partial specialization for std::string */
template <size_t BLOCKSIZE, int CACHE_SIZE>
struct WriteStreamOutFunctor<std::string, BLOCKSIZE, CACHE_SIZE> {
	static WriteStream<BLOCKSIZE, CACHE_SIZE>& write(WriteStream<BLOCKSIZE, CACHE_SIZE>& out, const std::string& src) {
#ifdef NDEBUG
		out.write(src);
#else
		ssize_t nwritten = out.write(src);
		assert(nwritten == static_cast<ssize_t>(src.length()));
#endif
		return out;
	}
};

/* partial specialization for IOBuffer */
template <size_t BLOCKSIZE, int CACHE_SIZE>
struct WriteStreamOutFunctor<IOBuffer, BLOCKSIZE, CACHE_SIZE> {
	static WriteStream<BLOCKSIZE, CACHE_SIZE>& write(WriteStream<BLOCKSIZE, CACHE_SIZE>& out, const IOBuffer& src) {
#ifdef NDEBUG
		out.write(src.data(), src.length());
#else
		ssize_t nwritten = out.write(src.data(), src.length());
		assert(nwritten == static_cast<ssize_t>(src.length()));
#endif
		return out;
	}
};

/* partial specialization for IOExtString */
template <size_t BLOCKSIZE, int CACHE_SIZE>
struct WriteStreamOutFunctor<IOExtString, BLOCKSIZE, CACHE_SIZE> {
	static WriteStream<BLOCKSIZE, CACHE_SIZE>& write(WriteStream<BLOCKSIZE, CACHE_SIZE>& out, const IOExtString& src) {
#ifdef NDEBUG
		out.write(src.string(), src.amount());
#else
		ssize_t nwritten = out.write(src.string(), src.amount());
		assert(nwritten == static_cast<ssize_t>(src.amount()));
#endif
		return out;
	}
};

template <typename T, size_t BLOCKSIZE, int CACHE_SIZE>
inline WriteStream<BLOCKSIZE, CACHE_SIZE>& operator<< (WriteStream<BLOCKSIZE, CACHE_SIZE>& out, const T& value)
{
	return WriteStreamOutFunctor<T, BLOCKSIZE, CACHE_SIZE>::write(out, value);
}


template <size_t BLOCKSIZE, int CACHE_SIZE>
class ReadStream
{
private:
	static const size_t RS_FAST_BUFFER_SIZE = 8192;

public:
	typedef StreamPos<BLOCKSIZE> data_locator_t;
	typedef StreamSizedPos<BLOCKSIZE> sized_locator_t;
	typedef XTYPENAME sized_locator_t::offset_t offset_t;
	typedef XTYPENAME StreamPos<BLOCKSIZE>::block_offset_t block_offset_t;
	typedef FileBlockStorage<BLOCKSIZE, CACHE_SIZE> block_storage_t;
	typedef XTYPENAME block_storage_t::block_t block_type;

	ReadStream(block_storage_t* bs, const sized_locator_t& location_) :
		m_bs(bs), m_location_start(location_), m_location(location_),
		m_srcp(NULL), m_src_block(),
		m_nread(0), m_fail(false), m_fast_data() {}
	~ReadStream() { m_src_block.reset(); }

	const sized_locator_t& location_start() const { return m_location_start; }

	const sized_locator_t& location() const { return m_location; }
	sized_locator_t& location() { return m_location; }
	ReadStream& location(sized_locator_t& location_) { m_location = location_; return *this; }

	size_t  nread() const { return m_nread; }
	bool    eof() const { return (m_location.size() <= 0); }
	bool    fail() const { return m_fail; }
	bool    fail(bool value) { bool old = m_fail; m_fail = value; return old; }
	size_t  avail() const { return m_location.size(); }

	void    clearfail() { m_fail = false; }

	ssize_t read(char *dstp, size_t dst_length);
	ssize_t read(std::string& dst, size_t dst_length);
	ssize_t read(std::string& dst) { return read(dst, avail()); }

	template<typename T>
	ssize_t read(T& value) {
		char buffer[sizeof(T)];
		const char* srcp = buffer;
		size_t bufsize = sizeof(buffer);
		T tmp;
		memset(buffer, 0, sizeof(buffer));
		ssize_t rc = this->read(buffer, sizeof(T));
		if (rc < static_cast<ssize_t>(sizeof(T))) {
			fail(true);
			return -1;
		}
		if (seriously::Traits<T>::deserialize(srcp, bufsize, tmp) < 0) {
			fail(true);
			return -1;
		}
		value = tmp;
		return rc;
	}

private:
	ReadStream();
	ReadStream(const ReadStream&);
	ReadStream& operator= (const ReadStream&) { return *this; }

	bool fetch_block() {
		if ((! m_src_block) || (m_src_block->index() != m_location.block_id())) {
			m_src_block = m_bs->get(m_location.block_id());
			m_srcp = m_src_block->data() + m_location.offset();
			if (! m_src_block)
				fail(true);
			return m_src_block ? true : false;
		}
		return true;
	}

	block_id_t     src_block_id() const    { return m_location.block_id(); }
	block_offset_t src_offset() const      { return m_location.offset(); }
	MW_SHPTR<block_type>& src_block()      { return m_src_block; }

	block_storage_t*     m_bs;
	sized_locator_t      m_location_start;
	sized_locator_t      m_location;
	char*                m_srcp;
	MW_SHPTR<block_type> m_src_block;
	size_t               m_nread;
	bool                 m_fail;

	char m_fast_data[RS_FAST_BUFFER_SIZE];
};

/*
 * function template partial specialization is forbidden in C++, so we
 * use a functor as an helper class
 */
template <typename T, size_t BLOCKSIZE, int CACHE_SIZE>
struct ReadStreamInFunctor {
	static ReadStream<BLOCKSIZE, CACHE_SIZE>& read(ReadStream<BLOCKSIZE, CACHE_SIZE>& in, T& value) {
		in.read(value);
		return in;
	}
};

/* partial specialization for std::string */
template <size_t BLOCKSIZE, int CACHE_SIZE>
struct ReadStreamInFunctor<std::string, BLOCKSIZE, CACHE_SIZE> {
	static ReadStream<BLOCKSIZE, CACHE_SIZE>& read(ReadStream<BLOCKSIZE, CACHE_SIZE>& in, std::string& dst) {
		in.read(dst, in.avail());
		return in;
	}
};

/* partial specialization for IOBuffer */
template <size_t BLOCKSIZE, int CACHE_SIZE>
struct ReadStreamInFunctor<IOBuffer, BLOCKSIZE, CACHE_SIZE> {
	static ReadStream<BLOCKSIZE, CACHE_SIZE>& read(ReadStream<BLOCKSIZE, CACHE_SIZE>& in, IOBuffer& dst) {
		in.read(dst.data(), dst.length());
		return in;
	}
};

/* partial specialization for IOBuffer */
template <size_t BLOCKSIZE, int CACHE_SIZE>
struct ReadStreamInFunctor<IOExtString, BLOCKSIZE, CACHE_SIZE> {
	static ReadStream<BLOCKSIZE, CACHE_SIZE>& read(ReadStream<BLOCKSIZE, CACHE_SIZE>& in, IOExtString& dst) {
		in.read(dst.string(), dst.amount());
		return in;
	}
};

template <typename T, size_t BLOCKSIZE, int CACHE_SIZE>
inline ReadStream<BLOCKSIZE, CACHE_SIZE>& operator>> (ReadStream<BLOCKSIZE, CACHE_SIZE>& in, T& value)
{
	return ReadStreamInFunctor<T, BLOCKSIZE, CACHE_SIZE>::read(in, value);
	// in.read(value);
	// return in;
}

} /* end of namespace milliways */

#include "BlockStorage.impl.hpp"

#endif /* MILLIWAYS_BLOCKSTORAGE_H */
