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

#include "lz4.h"

//#undef MILLIWAYS_DISABLE_COMPRESSION
//#define MILLIWAYS_DISABLE_COMPRESSION 1

#ifndef MILLIWAYS_KEYVALUESTORE_IMPL_H
//#define MILLIWAYS_KEYVALUESTORE_IMPL_H

/* ----------------------------------------------------------------- *
 *   ::seriously::Traits<milliways::kv_stream_pos_t>                     *
 * ----------------------------------------------------------------- */

namespace seriously {

inline ssize_t Traits<milliways::kv_stream_pos_t>::serialize(char*& dst, size_t& avail, const type& v)
{
	char* dstp = dst;
	size_t initial_avail = avail;

	if (sizeof(serialized_data_pos_type) > avail)
		return -1;

	assert(sizeof(serialized_data_pos_type) <= avail);

	serialized_data_pos_type v_pos = static_cast<serialized_data_pos_type>(v.pos());

	Traits<serialized_data_pos_type>::serialize(dstp, avail, v_pos);

	if (avail > 0)
		*dstp = '\0';

	dst = dstp;
	return static_cast<ssize_t>(initial_avail - avail);
}

inline ssize_t Traits<milliways::kv_stream_pos_t>::deserialize(const char*& src, size_t& avail, type& v)
{
	if (avail < sizeof(serialized_data_pos_type))
		return -1;

	const char* srcp = src;
	size_t initial_avail = avail;

	serialized_data_pos_type v_pos = static_cast<serialized_data_pos_type>(-1);

	if (Traits<serialized_data_pos_type>::deserialize(srcp, avail, v_pos) < 0)
		return -1;

	v.pos(static_cast<XTYPENAME type::offset_t>(v_pos));

	src = srcp;
	return static_cast<ssize_t>(initial_avail - avail);
}

inline ssize_t Traits<milliways::kv_stream_sized_pos_t>::serialize(char*& dst, size_t& avail, const type& v)
{
	char* dstp = dst;
	size_t initial_avail = avail;

	if ((sizeof(serialized_data_pos_type) + sizeof(serialized_size_type)) > avail)
		return -1;

	assert((sizeof(serialized_data_pos_type) + sizeof(serialized_size_type)) <= avail);

	serialized_data_pos_type v_pos = static_cast<serialized_data_pos_type>(v.pos());
	serialized_size_type v_size = static_cast<serialized_size_type>(v.size());

	Traits<serialized_data_pos_type>::serialize(dstp, avail, v_pos);
	Traits<serialized_size_type>::serialize(dstp, avail, v_size);

	if (avail > 0)
		*dstp = '\0';

	dst = dstp;
	return static_cast<ssize_t>(initial_avail - avail);
}

inline ssize_t Traits<milliways::kv_stream_sized_pos_t>::deserialize(const char*& src, size_t& avail, type& v)
{
	if (avail < (sizeof(serialized_data_pos_type) + sizeof(serialized_size_type)))
		return -1;

	const char* srcp = src;
	size_t initial_avail = avail;

	serialized_data_pos_type v_pos = static_cast<serialized_data_pos_type>(-1);
	serialized_size_type v_size = 0;

	if (Traits<serialized_data_pos_type>::deserialize(srcp, avail, v_pos) < 0)
		return -1;
	if (Traits<serialized_size_type>::deserialize(srcp, avail, v_size) < 0)
		return -1;

	v.pos(static_cast<XTYPENAME type::offset_t>(v_pos));
	v.size(static_cast<XTYPENAME type::size_type>(v_size));

	src = srcp;
	return static_cast<ssize_t>(initial_avail - avail);
}

} /* end of namespace seriously */


namespace milliways {


static const size_t KV_COMPRESSION_MIN_LENGTH = KV_BLOCKSIZE;

static const size_t LZ4_BLOCK_BYTES = 1024 * 8;
static const int N_LZ4_BUFFERS = 2;
static const int LZ4_ACCELERATION = 1;

/* ----------------------------------------------------------------- *
 *   KeyValueStore                                                   *
 * ----------------------------------------------------------------- */

inline KeyValueStore::KeyValueStore(block_storage_type* blockstorage) :
	m_blockstorage(blockstorage), m_storage(NULL), m_kv_tree(NULL),
	m_first_block_id(BLOCK_ID_INVALID), m_next_location(),
	m_kv_header_uid(-1)
{
#ifdef NDEBUG
#else
	int max_B = BTreeFileStorage_Compute_Max_B< BLOCKSIZE, KEY_MAX_SIZE + 4, mapped_traits >();
	assert(B <= max_B);
#endif

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
	kv_stream_pos_t head_pos;
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

		MW_SHPTR<kv_tree_node_type> node( where.node() );
		assert(node);

		// we store only the position inside the btree
		// so we need to read the size separately
		result.dataLocator(node->value(where.pos()));

		result.full_size(0);
		assert(result.locator().valid());
		assert(result.valid());
	} else
	{
		result.invalidate();
		return false;
	}

	assert(result.valid());

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

	// we store only the position inside the btree
	// so we need to read the size separately
	// To do that, preliminarily set the size to the "envelope"/header
	// size only, to be able to use the various reading
	// methods
	// NOTE: the envelope is the FullLocator one!
	result.full_size(FullLocator::ENVELOPE_SIZE);

	// ok, now we can use the various read stream methods...
	serialized_value_size_type v_value_length = 0;
	serialized_value_size_type v_compressed_length = 0;

	read_stream_t rs(m_blockstorage, result.locator().sizedLocator());
	rs >> v_value_length >> v_compressed_length;
	assert(! rs.fail());
	assert(rs.nread() == FullLocator::ENVELOPE_SIZE);

	if (v_compressed_length == 0) {
		result.set_uncompressed(static_cast<kv_stream_sized_pos_t::size_type>(v_value_length));
		assert(! result.isCompressed());
	} else {

		result.set_compressed(static_cast<kv_stream_sized_pos_t::size_type>(v_compressed_length), static_cast<kv_stream_sized_pos_t::size_type>(v_value_length));
		assert(result.isCompressed());
	}

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
    kv_stream_sized_pos_t payload_loc(result.payloadLocator());
    size_t payload_size = payload_loc.size();
	size_t compressed_size = 0;
	bool ok;

	read_stream_t rs(m_blockstorage, payload_loc);
	if (result.isCompressed()) {
		bool pre_compression_ok = false;
		ssize_t pre_compressed_size = -1;
		if (result.contents_size() < Lz4Packer::Size) {
			std::string buffer;
			IOExtString dst(buffer, result.compressed_size());
			rs >> dst;
			ok = !rs.fail();
			assert(rs.nread() == result.compressed_size());

			Lz4Packer lz4packer(buffer);
			if (lz4packer.lz4_read(value, result.contents_size(), pre_compressed_size) >= 0) {
				pre_compression_ok = true;
				ok = true;
			} else
			{
				assert(false);
				ok = false;
				// TODO: implement ReadStream::seek()
				// rs.seek(0, seek_start);
			}
		}

		if (! pre_compression_ok)
			ok = read_lz4(rs, value, result.contents_size(), compressed_size);
	} else {
		IOExtString dst(value, result.contents_size());
		rs >> dst;
		ok = !rs.fail();
	}
	assert(! rs.fail());
#ifdef NDEBUG
	UNUSED(payload_size);
#else
	assert(rs.nread() == payload_size);
#endif
	result.locator().consume(rs.nread());		// move and shrink
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

    kv_stream_sized_pos_t payload_loc(result.payloadLocator());
    size_t payload_size = payload_loc.size();
	bool ok;

	// if (partial > 0)
	// 	payload_loc.size(static_cast<FullLocator::size_type>(partial));
	// else
	// 	partial = (ssize_t) payload_size;

	read_stream_t rs(m_blockstorage, payload_loc);
	if (result.isCompressed())
	{
		ssize_t compressed_size   = (ssize_t) result.compressed_size();
		ssize_t uncompressed_size = (ssize_t) result.uncompressed_size();
		size_t r_compressed_size = 0;

		if (partial > 0)
			payload_loc.size(static_cast<FullLocator::size_type>(partial));
		else
			partial = (ssize_t) uncompressed_size;

		bool pre_compression_ok = false;
		ssize_t pre_compressed_size = -1;
		if (result.contents_size() < Lz4Packer::Size) {
			std::string buffer;
			IOExtString dst(buffer, result.compressed_size());
			rs >> dst;
			ok = !rs.fail();
			assert(rs.nread() == result.compressed_size());

			Lz4Packer lz4packer(buffer);
			if (lz4packer.lz4_read(value, result.contents_size(), pre_compressed_size) >= 0) {
				pre_compression_ok = true;
				ok = true;
			} else
			{
				assert(false);
				ok = false;
				// TODO: implement ReadStream::seek()
				// rs.seek(0, seek_start);
			}
		}

		// std::cerr << "payload_loc: " << payload_loc << " payload_size:" << payload_size << " partial:" << partial << "\n";
		if (! pre_compression_ok)
			ok = read_lz4(rs, value, (size_t) partial, r_compressed_size);
		// std::cerr << "read_lz4 ok: " << (ok ? "OK" : "NO") << " r-compressed_size:" << r_compressed_size << "\n";

		if (! ok) return false;

		assert(! rs.fail());
		// if ((ssize_t) rs.nread() != compressed_size) {
		// 	std::cerr << "(ssize_t) rs.nread():" << (rs.nread()) << " != compressed_size:" << compressed_size << "  payload_size:" << payload_size << "\n";
		// }
		assert((ssize_t) rs.nread() == compressed_size);
	} else {
		if (partial > 0)
			payload_loc.size(static_cast<FullLocator::size_type>(partial));
		else
			partial = (ssize_t) payload_size;

		IOExtString dst(value, (size_t) partial);
		rs >> dst;
		ok = !rs.fail();

		assert(! rs.fail());
		// if ((ssize_t) rs.nread() != partial) {
		// 	std::cerr << "(ssize_t) rs.nread():" << (rs.nread()) << " != partial:" << partial << "  payload_size:" << payload_size << "\n";
		// }
		assert((ssize_t) rs.nread() == partial);
	}
	result.locator().consume(rs.nread());		// move and shrink
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

	kv_stream_pos_t head_pos;
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

	bool do_allocate = true;

	FullLocator locator;

	bool do_compress = (value.length() >= KV_COMPRESSION_MIN_LENGTH);
#if MILLIWAYS_DISABLE_COMPRESSION
	do_compress = false;
#endif

	bool pre_compression_ok = false;
	ssize_t pre_compressed_size = -1;
	Lz4Packer lz4packer;

	size_t payload_size = value.length();

	if (do_compress && (value.length() < Lz4Packer::Size)) {
		if (lz4packer.lz4_write(value, pre_compressed_size) >= 0) {
			bool has_compressed = ((size_t)pre_compressed_size < value.length()) ? true : false;

			if (has_compressed) {
				pre_compression_ok = true;
				payload_size = (size_t)pre_compressed_size;
			} else {
				pre_compression_ok = false;
				do_compress = false;
			}
		} else {
			pre_compression_ok = false;
		}
	} else {
	}

	if (present)
	{
		/* present */
		assert(result.valid());
		assert(result.found());

		if (! overwrite)
			return false;

		// TODO: handle allocation decision when using compression
		if (result.isCompressed())
		{
			do_allocate = ((! pre_compression_ok) || ((size_t)pre_compressed_size > result.payload_size()) || ((value.length() > result.payload_size()))) ? true : false;
		} else
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
		// result.contents_size(value.length());
		// allocate for uncompressed size, then later we'll mark
		// residual space as free (TODO)
		kv_stream_sized_pos_t fresh;
		size_t amount = 0;
		if (do_compress)
		{
			if (pre_compression_ok)
				amount = pre_compressed_size + FullLocator::ENVELOPE_SIZE;
			else
			{
				amount = value.length(); /* value.length() + FullLocator::ENVELOPE_SIZE; */
				size_t n_lz4_blocks = (amount + LZ4_BLOCK_BYTES - 1) / LZ4_BLOCK_BYTES;
				size_t lz4_amount = n_lz4_blocks * (sizeof(uint32_t) + LZ4_COMPRESSBOUND(LZ4_BLOCK_BYTES));
				amount = lz4_amount + FullLocator::ENVELOPE_SIZE; /* lz4_amount */
			}
		} else
			amount = value.length() + FullLocator::ENVELOPE_SIZE;
		if (! alloc_space(fresh, amount))
			return false;

		// locator.set_uncompressed(value.length());
		locator = fresh;
		assert(locator.valid());
		assert(locator.payload_size() >= payload_size);
		assert(locator.contents_size() >= payload_size);
		assert(! locator.isCompressed());
		assert(locator.full_size() >= payload_size + FullLocator::ENVELOPE_SIZE);

		// result.locator() = fresh;
		// assert(result.locator().valid());
		// assert(result.payload_size() >= value.length());
		// assert(result.contents_size() == value.length());
		// assert(! result.isCompressed());
		// assert(result.full_size() == value.length() + (2 * sizeof(serialized_value_size_type)));
		// std::cerr << "allocated " << result.locator() << "\n";
	} else
	{
		locator = result.locator();
	}

	// --  write value length and data --

	// kv_stream_pos_t dst_pos(head_pos);

	bool ok = false;
	bool done = false;
	size_t compressed_size = 0;

	// FullLocator head_loc(result.headLocator());
	kv_stream_sized_pos_t head_loc(locator.headLocator());
	write_stream_t ws(m_blockstorage, head_loc);

	// write uncompressed value length and
	// preliminary compressed value length (to be rewritten later)
	serialized_value_size_type v_value_length = static_cast<serialized_value_size_type>(value.length());
	serialized_value_size_type v_compressed_length = 0;
	if (pre_compression_ok)
		v_compressed_length = static_cast<serialized_value_size_type>(pre_compressed_size);

	if (pre_compression_ok)
	{
		compressed_size = static_cast<size_t>(pre_compressed_size);

		// update head block with compressed size information
		result.locator() = locator;
		result.set_compressed(compressed_size, value.length());
		assert(result.isCompressed());

		v_compressed_length = static_cast<serialized_value_size_type>(compressed_size);
		bool s_ok = ws.seek(0, seek_start);
#ifdef NDEBUG
		UNUSED(s_ok);
#else
		assert(s_ok);
#endif
		ws << v_value_length << v_compressed_length;

		ws.write(lz4packer.data(), lz4packer.size());
		done = true;
	} else
	{
		ws << v_value_length << v_compressed_length;
	}

	// write value string

	if ((! done) && do_compress) {
		ok = write_lz4(ws, value, compressed_size);
		if (ok) {
			bool has_compressed = (compressed_size < value.length()) ? true : false;

			if (! has_compressed) {
				do_compress = false;
				done = false;
			} else {
				done = true;
			}
		} else {
			done = false;
		}

		if (done) {
			// update head block with compressed size information
			result.locator() = locator;
			result.set_compressed(compressed_size, value.length());
			assert(result.isCompressed());

			v_compressed_length = static_cast<serialized_value_size_type>(compressed_size);
			bool s_ok = ws.seek(0, seek_start);
#ifdef NDEBUG
			UNUSED(s_ok);
#else
			assert(s_ok);
#endif
			ws << v_value_length << v_compressed_length;
			// std::cerr << "value length:" << v_value_length << " compressed length: " << v_compressed_length << "\n";
		} else {
			// /* compression failed, retry without compression */
			// ws.seek(0, seek_start);
			// ws << v_value_length << v_compressed_length;
		}
	} else {
	}

	if (! done) {
		// no compression or compression failed (retry without compression)
		/* compression failed, retry without compression */
		bool s_ok = ws.seek(0, seek_start);
#ifdef NDEBUG
		UNUSED(s_ok);
#else
		assert(s_ok);
#endif
		ws << v_value_length << v_compressed_length;

		ws << value;
		ok = !ws.fail();

		if (ok) {
			done = true;
		} else {
			std::cerr << "FAIL" << std::endl;
		}
		result.locator() = locator;
		result.set_uncompressed(value.length());
	}

	ws.flush();
	ok = done && !ws.fail();

	if (! ok) {
		// everything failed!
		std::cerr << "WARNING: write FAILED" << std::endl;
		assert(false);
		return false;
	}

	assert(do_compress || (ws.nwritten() == static_cast<size_t>(head_loc.size())));

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

		// TODO: if allocated more space than used, release excess space here
	}

	return ok;
}

inline bool KeyValueStore::find(const std::string& key, kv_stream_pos_t& data_pos)
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

		MW_SHPTR<kv_tree_node_type> node( where.node() );
		assert(node);

		data_pos = node->value(where.pos());
		assert(data_pos.valid());
		return true;
	}

	data_pos.invalidate();
	return false;
}

inline bool KeyValueStore::extend_allocated_space(kv_stream_sized_pos_t& dst, size_t amount)
{
	if (! m_next_location.follows(dst))
		return false;

	FullLocator extra;
	if (! alloc_space(extra, amount))
		return false;

	return dst.merge(extra);
}

inline bool KeyValueStore::alloc_space(kv_stream_sized_pos_t& dst, size_t amount)
{
	// size_t amount = dst.size();
	if ((! m_next_location.valid()) || (m_next_location.size() < amount))
	{
		size_t n_blocks = size_in_blocks(amount);
		assert((n_blocks * BLOCKSIZE) >= amount);
		block_id_t first_block_id = block_alloc_id(static_cast<int>(n_blocks));
		if (! block_id_valid(first_block_id))
			return false;
		m_next_location.from_block_offset(first_block_id, 0);
		m_next_location.size(n_blocks * BLOCKSIZE);
		if (! block_id_valid(m_first_block_id))
			m_first_block_id = m_next_location.block_id();
	}

	assert(block_id_valid(m_first_block_id));
	assert(m_next_location.valid());

	assert(m_next_location.size() >= amount);
	assert(m_next_location.offset() >= 0);
	if (amount <= BLOCKSIZE)
	{
		assert(static_cast<ssize_t>(m_next_location.offset()) <= static_cast<ssize_t>((BLOCKSIZE - amount)));
	}

	// set dst kv_stream_sized_pos_t to allocated space
	dst.size(amount);
	dst.pos(m_next_location.pos());

	// compute next block id / avail
	m_next_location.consume(amount);		// move and shrink
	/* assert(m_next_location.full_size() >= 0); */
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
	std::string headerPrefix("KEYVALUEDIRECT");
	packer << headerPrefix <<
		static_cast<uint32_t>(MAJOR_VERSION) << static_cast<uint32_t>(MINOR_VERSION) <<
	 	static_cast<uint32_t>(BLOCKSIZE) << static_cast<uint32_t>(B) <<
	 	static_cast<uint32_t>(KEY_MAX_SIZE);

	packer << m_first_block_id <<
		static_cast<kv_stream_pos_t::offset_t>(m_next_location.pos()) <<
		static_cast<size_t>(m_next_location.size());

	std::string userHeader(packer.data(), packer.size());
	m_blockstorage->setUserHeader(m_kv_header_uid, userHeader);

	// std::cerr << "-> KV WRITE VER:" << MAJOR_VERSION << "." << MINOR_VERSION << " BLOCKSIZE:" << BLOCKSIZE <<
	// 	" B:" << B << std::endl;

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

	std::string headerPrefix;
	uint32_t v_MAJOR, v_MINOR, v_BLOCKSIZE, v_B, v_KEY_MAX_SIZE;

	packer >> headerPrefix;
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

	if ((static_cast<int>(v_B) != B) || (static_cast<size_t>(v_BLOCKSIZE) != BLOCKSIZE))
	{
		std::cerr << "ERROR: '" << m_blockstorage->pathname() << "' doesn't match with kv btree properties (B/BLOCKSIZE)" << std::endl;
		return false;
	}

	assert(static_cast<int>(v_B) == B);
	assert(static_cast<size_t>(v_BLOCKSIZE) == BLOCKSIZE);
	assert(v_MAJOR <= MAJOR_VERSION);

	kv_stream_pos_t::offset_t v_next_pos;
	size_t v_next_avail;
	packer >> m_first_block_id >> v_next_pos >> v_next_avail;
	m_next_location.pos(v_next_pos);
	m_next_location.size(v_next_avail);

	return true;
}


} /* end of namespace milliways */

#endif /* MILLIWAYS_KEYVALUESTORE_IMPL_H */
