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
#include "BlockStorage.h"
#endif

#include "Seriously.h"
#include "Utils.h"

#include "lz4packer.impl.hpp"

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include "lz4.h"

#ifndef MILLIWAYS_BLOCKSTORAGE_IMPL_H
//#define MILLIWAYS_BLOCKSTORAGE_IMPL_H

namespace milliways {

static const size_t BS_LZ4_BLOCK_BYTES = 1024 * 8;
static const int BS_N_LZ4_BUFFERS = 2;
static const int BS_LZ4_ACCELERATION = MILLIWAYS_LZ4_ACCELERATION;

/* ----------------------------------------------------------------- *
 *   BlockStorage                                                    *
 * ----------------------------------------------------------------- */

template <size_t BLOCKSIZE> const int BlockStorage<BLOCKSIZE>::MAJOR_VERSION;
template <size_t BLOCKSIZE> const int BlockStorage<BLOCKSIZE>::MINOR_VERSION;
template <size_t BLOCKSIZE> const size_t BlockStorage<BLOCKSIZE>::MAX_USER_HEADER_LEN;

template <size_t BLOCKSIZE>
bool BlockStorage<BLOCKSIZE>::open()
{
	if (isOpen())
		return true;

	assert(! isOpen());
	if (! openHelper())
		return false;

	assert(isOpen());

	if (created())
		m_header_block_id = allocId();
	else
		m_header_block_id = firstId();

	if (! created())
		return readHeader() && isOpen();
	return isOpen();
}

template <size_t BLOCKSIZE>
bool BlockStorage<BLOCKSIZE>::close()
{
	if (! isOpen())
		return true;

	// TODO: flush cache

	assert(isOpen());

	return writeHeader() && closeHelper();
}

/* -- Header --------------------------------------------------- */

template <typename T>
T min(const T& a, const T& b)
{
	return (a <= b) ? a : b;
}

template <size_t BLOCKSIZE>
bool BlockStorage<BLOCKSIZE>::readHeader()
{
	assert(m_header_block_id != BLOCK_ID_INVALID);
	// block_t headerBlock(m_header_block_id);
	MW_SHPTR<block_t> headerBlock( manager().get_object(m_header_block_id) );
	assert(headerBlock);
	if (! read(*headerBlock))
		return false;
//	std::cerr << "read header block " << m_header_block_id << " dump:" << std::endl << s_hexdump(headerBlock.data(), 256);

	// deserialize header

	seriously::Packer<BLOCKSIZE> packer(headerBlock->data(), headerBlock->size());
	int32_t v_major, v_minor, v_n_user_headers;
	packer >> v_major >> v_minor >> v_n_user_headers;
	m_user_header.clear();
	// std::cerr << "reading " << v_n_user_headers << " user headers" << std::endl;
	for (int uid = 0; uid < v_n_user_headers; uid++)
	{
		int32_t v_uid;
		std::string v_user_header;

		packer >> v_uid;
		packer >> v_user_header;
		assert(! packer.error());
		if (packer.error())
			return false;
		if (v_uid != uid)
			return false;

//		std::cerr << "userHeader[" << v_uid << "] len:" << v_user_header.size() << " data:" << v_user_header << std::endl;
		m_user_header.push_back(v_user_header);
	}
	assert(! packer.error());

	return true;
}

template <size_t BLOCKSIZE>
bool BlockStorage<BLOCKSIZE>::writeHeader()
{
	assert(m_header_block_id != BLOCK_ID_INVALID);
	// block_t headerBlock(m_header_block_id);
	MW_SHPTR<block_t> headerBlock( manager().get_object(m_header_block_id) );
	assert(headerBlock);

	// serialize header

	seriously::Packer<BLOCKSIZE> packer;

	packer << static_cast<int32_t>(MAJOR_VERSION) <<
			static_cast<int32_t>(MINOR_VERSION) <<
			static_cast<int32_t>(m_user_header.size());

	std::vector<std::string>::const_iterator it;
	int uid = 0;
	for (it = m_user_header.begin(); it != m_user_header.end(); ++it)
	{
		const std::string& userHeader = *it;

		if (userHeader.length() > MAX_USER_HEADER_LEN)
			return false;

		packer << static_cast<int32_t>(uid) << userHeader;
		assert(! packer.error());
		if (packer.error())
			return false;

		uid++;
	}
	assert(! packer.error());
	if (packer.error())
		return false;

	assert(packer.size() <= headerBlock->size());
	memcpy(headerBlock->data(), packer.data(), packer.size());

	if (! write(*headerBlock))
		return false;
//	std::cerr << "wrote header block " << headerBlock.index() << " dump:" << std::endl << s_hexdump(headerBlock.data(), 256);

	return true;
}

/* -- Block I/O ------------------------------------------------ */

template <size_t BLOCKSIZE>
block_id_t BlockStorage<BLOCKSIZE>::allocBlock(block_t& dst)
{
	block_id_t block_id = allocId();

	assert(dst.index() == BLOCK_ID_INVALID);
	dst.index(block_id);
	return block_id;
}

template <size_t BLOCKSIZE>
bool BlockStorage<BLOCKSIZE>::dispose(block_t& block)
{
	block_id_t block_id = block.index();

	if (block_id == BLOCK_ID_INVALID)
		return false;
	if (! dispose(block_id))
		return false;
	block.index(BLOCK_ID_INVALID);
	return true;
}

/* ----------------------------------------------------------------- *
 *   LRUBlockCache                                                   *
 * ----------------------------------------------------------------- */

template < size_t BLOCKSIZE, size_t CACHESIZE >
const typename LRUBlockCache<BLOCKSIZE, CACHESIZE>::size_type LRUBlockCache<BLOCKSIZE, CACHESIZE>::Size;

template < size_t BLOCKSIZE, size_t CACHESIZE >
const typename LRUBlockCache<BLOCKSIZE, CACHESIZE>::size_type LRUBlockCache<BLOCKSIZE, CACHESIZE>::BlockSize;

template < size_t BLOCKSIZE, size_t CACHESIZE >
const block_id_t LRUBlockCache<BLOCKSIZE, CACHESIZE>::InvalidCacheKey;

/* ----------------------------------------------------------------- *
 *   FileBlockStorage                                                *
 * ----------------------------------------------------------------- */

template <size_t BLOCKSIZE, int CACHE_SIZE>
FileBlockStorage<BLOCKSIZE, CACHE_SIZE>::~FileBlockStorage()
{
	/* call close() from the most derived class */
	if (isOpen())
	{
		std::cerr << std::endl << "WARNING: FileBlockStorage still open at destruction time. Call close() *BEFORE* destruction!." << std::endl << std::endl;
		close();
	}
	assert(! isOpen());
}

template <size_t BLOCKSIZE, int CACHE_SIZE>
bool FileBlockStorage<BLOCKSIZE, CACHE_SIZE>::openHelper()
{
	if (isOpen())
		return true;

	assert(! isOpen());
	m_stream.open(m_pathname.c_str(), std::fstream::binary | std::fstream::in | std::fstream::out);
	if (m_stream.is_open())
	{
		m_created = false;
	} else if (! m_stream.is_open())
	{
		std::cerr << "file '" << m_pathname << "' doesn't exist. Creating..." << std::endl;
		m_stream.open(m_pathname.c_str(), std::fstream::binary | std::fstream::in | std::fstream::out | std::fstream::trunc);
		m_created = true;
	}

	assert(m_stream.is_open());

	m_count = -1;

	return isOpen();
}

template <size_t BLOCKSIZE, int CACHE_SIZE>
bool FileBlockStorage<BLOCKSIZE, CACHE_SIZE>::closeHelper()
{
	// std::cerr << "FBS::closeHelper()" << std::endl;
	assert(isOpen());

	m_lru.evict_all();

	m_stream.close();

	m_created = false;
	m_count = -1;

	return true;
}

template <size_t BLOCKSIZE, int CACHE_SIZE>
bool FileBlockStorage<BLOCKSIZE, CACHE_SIZE>::flush()
{
	// TODO: flush cache

	return true;
}

template <size_t BLOCKSIZE, int CACHE_SIZE>
typename FileBlockStorage<BLOCKSIZE, CACHE_SIZE>::size_type FileBlockStorage<BLOCKSIZE, CACHE_SIZE>::count()
{
	if (! isOpen())
		return 0;

	if (m_count < 0)
		_updateCount();

	assert(m_count >= 0);
	return static_cast<size_type>(m_count);
}

template <size_t BLOCKSIZE, int CACHE_SIZE>
void FileBlockStorage<BLOCKSIZE, CACHE_SIZE>::_updateCount()
{
#if STACK_PROTECTOR
	char dummy[8]; UNUSED(dummy);	/* for -fstack-protector -Wstack-protector */
#endif

	if (! isOpen())
		return;

	assert(m_stream.is_open());

	m_stream.seekg(0, std::ios_base::end);
	std::ifstream::pos_type pos = m_stream.tellg();
	assert(pos != static_cast<std::ifstream::pos_type>(-1));
	assert((pos % static_cast<std::ifstream::pos_type>(BlockSize)) == 0);
	m_count = static_cast<ssize_t>(pos / static_cast<std::ifstream::pos_type>(BlockSize));
//	std::cout << "block count:" << m_count << std::endl;

	if (m_next_block_id == BLOCK_ID_INVALID)
		m_next_block_id = static_cast<block_id_t>(m_count);
}

template <size_t BLOCKSIZE, int CACHE_SIZE>
block_id_t FileBlockStorage<BLOCKSIZE, CACHE_SIZE>::nextId()
{
	if (! isOpen())
		return 0;

	if (m_next_block_id == BLOCK_ID_INVALID)
		_updateCount();

	assert(m_next_block_id != BLOCK_ID_INVALID);
	return m_next_block_id;
}

template <size_t BLOCKSIZE, int CACHE_SIZE>
block_id_t FileBlockStorage<BLOCKSIZE, CACHE_SIZE>::allocId(int n_blocks)
{
	// std::cerr << "FBS::allocId(" << n_blocks << ")" << std::endl;
	// block_id_t block_id = m_next_block_id;
	// if (block_id == BLOCK_ID_INVALID)
	// 	block_id = static_cast<block_id_t>(count());
	block_id_t block_id = nextId();
	m_next_block_id = block_id + static_cast<block_id_t>(n_blocks);
	return block_id;
}

template <size_t BLOCKSIZE, int CACHE_SIZE>
bool FileBlockStorage<BLOCKSIZE, CACHE_SIZE>::dispose(block_id_t block_id, int count_)
{
	if (block_id == BLOCK_ID_INVALID)
		return false;

	assert(block_id != BLOCK_ID_INVALID);

	nextId();	// force update of m_next_block_id if necessary
	if (m_next_block_id == (block_id + static_cast<block_id_t>(count_)))
		m_next_block_id -= static_cast<block_id_t>(count_);

	return true;
}

template <size_t BLOCKSIZE, int CACHE_SIZE>
bool FileBlockStorage<BLOCKSIZE, CACHE_SIZE>::read(block_t& dst)
{
#if STACK_PROTECTOR
	char dummy[8]; UNUSED(dummy);	/* for -fstack-protector -Wstack-protector */
#endif

	// std::cerr << "bs.read(" << dst.index() << ")" << std::endl;
	assert(dst.index() != BLOCK_ID_INVALID);

	size_t pos = dst.index() * BlockSize;

	try {
		m_stream.seekg(static_cast<std::streamoff>(pos));
	} catch (std::ios::failure) {
		assert(false);
		return false;
	}

	try {
		m_stream.read(dst.data(), BlockSize);
	} catch (std::ios_base::failure& e) {
		std::cerr << "error reading block " << dst.index() << ":" << e.what() << std::endl;
		assert(false);
		return false;
	}

	if (m_stream.fail())
	{
		// std::cerr << "can't read block " << dst.index() << "\n";
		dst.dirty(true);
		m_stream.clear();
		// std::cerr << "FBS::read() failure" << std::endl;
		return false;
	} else
		dst.dirty(false);

	// std::cerr << "FBS::read(" << dst.index() << ") block dump:" << std::endl << s_hexdump(dst.data(), 128) << std::endl;

//	if (m_stream.fail())
//	{
//		std::cerr << "stream fail reading block " << dst.index() << ", error: " << strerror(errno) << std::endl;
//		assert(false);
//		return false;
//	}

	assert(! m_stream.fail());
	return true;
}

template <size_t BLOCKSIZE, int CACHE_SIZE>
bool FileBlockStorage<BLOCKSIZE, CACHE_SIZE>::write(block_t& src)
{
#if STACK_PROTECTOR
	char dummy[8]; UNUSED(dummy);	/* for -fstack-protector -Wstack-protector */
#endif

	// std::cerr << "bs.write(" << src.index() << ")" << std::endl;
	assert(src.index() != BLOCK_ID_INVALID);

	size_t pos = src.index() * BlockSize;

	try {
		m_stream.seekp(static_cast<std::streamoff>(pos));
	} catch (std::ios::failure& e) {
		std::cerr << "error writing (seeking) block " << src.index() << ":" << e.what() << std::endl;
		src.dirty(true);
		assert(false);
		return false;
	}

	try {
		m_stream.write(src.data(), BlockSize);
	} catch (std::ios_base::failure& e) {
		std::cerr << "error writing block " << src.index() << ":" << e.what() << std::endl;
		src.dirty(true);
		assert(false);
		return false;
	}

	if (m_stream.fail())
	{
		std::cerr << "stream fail writing block " << src.index() << ", error: " << strerror(errno) << std::endl;
		src.dirty(true);
		assert(false);
		return false;
	} else
		src.dirty(false);

	assert(! m_stream.fail());

	pos += BlockSize;
	if (pos >= (static_cast<size_t>(m_count) * BlockSize))
		m_count = static_cast<ssize_t>(pos / BlockSize);

	// std::cerr << "FBS::write(" << src.index() << ") block dump:" << std::endl << s_hexdump(src.data(), 128) << std::endl;
	return true;
}

/* cached I/O */

template <size_t BLOCKSIZE, int CACHE_SIZE>
MW_SHPTR<typename FileBlockStorage<BLOCKSIZE, CACHE_SIZE>::block_t> FileBlockStorage<BLOCKSIZE, CACHE_SIZE>::get(block_id_t block_id, bool createIfNotFound)
{
	MW_SHPTR<block_t> cached_ptr;
	if (m_lru.get(cached_ptr, block_id))
		return cached_ptr;
	else {
		if (createIfNotFound) {
			cached_ptr = this->manager().get_object(block_id);
			assert(cached_ptr);
			m_lru.set(block_id, cached_ptr);
		}
		return cached_ptr;

	}
}

template <size_t BLOCKSIZE, int CACHE_SIZE>
bool FileBlockStorage<BLOCKSIZE, CACHE_SIZE>::put(const block_t& src)
{
	MW_SHPTR<block_t> cached_ptr;
	if (m_lru.get(cached_ptr, src.index()))
	{
		if (cached_ptr.get() != &src)
		{
			assert(cached_ptr->index() == src.index());
			*cached_ptr = src;
			assert(src.dirty() == (*cached_ptr).dirty());
		}
		return true;
	} else
	{
		block_id_t bid = src.index();
		MW_SHPTR<block_t> src_ptr( this->manager().get_object(bid) );
		assert(src_ptr);
		*src_ptr = src;
		return m_lru.set(bid, src_ptr) ? true : false;
	}

	return false;
}

/* -- Streaming I/O -------------------------------------------- */

	/* streaming read */

template <size_t BLOCKSIZE, int CACHE_SIZE>
inline bool FileBlockStorage<BLOCKSIZE, CACHE_SIZE>::read_lz4(read_stream_t& rs, char*& dstp, size_t nbytes, size_t& compressed_size)
{
    LZ4_streamDecode_t lz4StreamDecode_body;
    LZ4_streamDecode_t* lz4StreamDecode = &lz4StreamDecode_body;
	char cmpBuf[LZ4_COMPRESSBOUND(BS_LZ4_BLOCK_BYTES)];

    char decBuf[BS_N_LZ4_BUFFERS][BS_LZ4_BLOCK_BYTES];
    int  decBufIndex = 0;

    LZ4_setStreamDecode(lz4StreamDecode, NULL, 0);

	size_t  to_read = nbytes;
	size_t  nread   = 0;
	size_t  nread_u = 0;
	ssize_t nr;

	while (to_read > 0)
	{
		assert(to_read > 0);

		uint32_t cmpBytes = 0;

		nr = rs.read(cmpBytes);
		if (nr < 0)
			break;		/* failure */
		nread += static_cast<size_t>(nr);

		nr = rs.read(cmpBuf, static_cast<size_t>(cmpBytes));
		if (nr < 0)
			break;		/* failure */
		nread += static_cast<size_t>(nr);

	    char* const decPtr = decBuf[decBufIndex];
	    const int decBytes = LZ4_decompress_safe_continue(
	        lz4StreamDecode, cmpBuf, decPtr, (int) cmpBytes, BS_LZ4_BLOCK_BYTES);
	    if (decBytes <= 0)
	        break;		/* failure */

	    size_t to_copy = min((size_t) to_read, (size_t) decBytes);
		assert(to_copy <= to_read);
	    memcpy(dstp, decPtr, to_copy);
		dstp    += to_copy; // (size_t) decBytes;
		nread_u += to_copy; // (size_t) decBytes;
		to_read -= to_copy; // (ssize_t) decBytes;

		decBufIndex = (decBufIndex + 1) % BS_N_LZ4_BUFFERS;
	}
	if (to_read != 0)
		std::cerr << "ERROR: to_read != 0 - nbytes:" << nbytes << " nread:" << nread << " nread_u:" << nread_u << "\n";
	assert(to_read == 0);

	assert(rs.nread() == nread);

	compressed_size = nread;
	return (nread_u == nbytes);
}

template <size_t BLOCKSIZE, int CACHE_SIZE>
inline bool FileBlockStorage<BLOCKSIZE, CACHE_SIZE>::read_lz4(read_stream_t& rs, std::string& dst, size_t nbytes, size_t& compressed_size)
{
	const size_t BS_C_FAST_BUFFER_SIZE = 8192;

	char fast_data[BS_C_FAST_BUFFER_SIZE];
	char *dst_data;
	if ((nbytes + 1) < sizeof(fast_data))
		dst_data = fast_data;
	else
		dst_data = new char[nbytes + 1];
	char *dstp = dst_data;

	bool ok = read_lz4(rs, dstp, nbytes, compressed_size);
	if (ok)
	{
		*dstp = '\0';

		dst.clear();
		dst.reserve(nbytes);
		dst.resize(nbytes);
		dst.assign(dst_data, nbytes);
	}

	if (dst_data != fast_data)
	{
		delete[] dst_data;
		dst_data = NULL;
	}

	return ok;
}

	/* streaming write */

template <size_t BLOCKSIZE, int CACHE_SIZE>
inline bool FileBlockStorage<BLOCKSIZE, CACHE_SIZE>::write_lz4(write_stream_t& ws, const char*& srcp, size_t nbytes, size_t& compressed_size)
{
    LZ4_stream_t lz4Stream_body;
    LZ4_stream_t* lz4Stream = &lz4Stream_body;
	char cmpBuf[LZ4_COMPRESSBOUND(BS_LZ4_BLOCK_BYTES)];

    char inpBuf[BS_N_LZ4_BUFFERS][BS_LZ4_BLOCK_BYTES];
    int  inpBufIndex = 0;

	compressed_size = 0;

    LZ4_resetStream(lz4Stream);

	size_t  to_write   = nbytes;
	size_t  nwritten_u = 0;
	size_t  nwritten   = 0;
	ssize_t nw;

	while ((to_write > 0) && (ws.avail() > 0))
	{
		assert(to_write > 0);

		size_t amount = min(BS_LZ4_BLOCK_BYTES, to_write);

		// compress 'amount' bytes from srcp - using our double buffer
        char* const inpPtr = inpBuf[inpBufIndex];
        assert(amount <= BS_LZ4_BLOCK_BYTES);
        memcpy(inpPtr, srcp, amount);
        srcp     += amount;

        const int cmpBytes = LZ4_compress_fast_continue(
            lz4Stream, inpPtr, cmpBuf, static_cast<int>(amount), sizeof(cmpBuf), /* acceleration */ BS_LZ4_ACCELERATION);
        if (cmpBytes <= 0)
            break;			/* failure */

        nw = ws.write(static_cast<uint32_t>(cmpBytes));
        if (nw < 0)
            break;			/* failure */
		nwritten += static_cast<size_t>(nw);

        nw = ws.write(cmpBuf, static_cast<uint32_t>(cmpBytes));
        if (nw < 0)
            break;			/* failure */
		nwritten   += static_cast<size_t>(nw);
		nwritten_u += amount;

        inpBufIndex = (inpBufIndex + 1) % BS_N_LZ4_BUFFERS;

		to_write -= amount;
	}
	if (to_write > 0) {
		/* failure */
		std::cerr << "FAILED write_lz4()" << "\n";
		return false;
	}
	assert(to_write == 0);
	/* assert(dst_avail >= 0); */

	compressed_size = nwritten;
	return (nwritten_u == nbytes);
}


/* ----------------------------------------------------------------- *
 *   Streaming                                                       *
 * ----------------------------------------------------------------- */

template <size_t BLOCKSIZE, int CACHE_SIZE>
inline bool WriteStream<BLOCKSIZE, CACHE_SIZE>::seek(ssize_t off, seek_t seek_type)
{
	if ((off == 0) && (seek_type == seek_current))
		return true;

	sized_locator_t new_location(m_location_start);
	ssize_t      new_avail = 0;

	switch (seek_type)
	{
	case seek_start:
		if ((off < 0) || (off > static_cast<ssize_t>(m_location_start.size()))) {
			fail(true);
			return false;
		}
		new_location = m_location_start;
		// new_location.normalize();
		new_avail = static_cast<ssize_t>(new_location.size());
		new_location.delta(static_cast<offset_t>(off));
		new_avail -= off;
		m_nwritten = off;
		break;

	case seek_current:
		new_location = m_location;
		// new_location.normalize();
		new_avail = (ssize_t) m_location.size();
		new_location.delta(static_cast<offset_t>(off));
		new_avail -= off;
		m_nwritten -= off;
		break;

	case seek_end:
		{
			if ((off > 0) || (off < -static_cast<ssize_t>(m_location_start.size()))) {
				fail(true);
				return false;
			}
			new_location = m_location_start;
			// new_location.normalize();
			ssize_t initial_size = static_cast<ssize_t>(new_location.size());
			new_avail = 0;
			new_location.delta(static_cast<offset_t>(initial_size + off));
			new_avail -= off;
			m_nwritten = initial_size + off;
		}
		break;

	default:
		assert(false);
		break;
	}

	if (new_avail < 0) {
		fail(true);
		return false;
	}

	if (m_dst_block)
		m_bs->put(*m_dst_block);
	m_dst_block.reset();

	m_location = new_location;
	m_dstp     = NULL;
	return true;
}

template <size_t BLOCKSIZE, int CACHE_SIZE>
inline ssize_t WriteStream<BLOCKSIZE, CACHE_SIZE>::write(const char *srcp, size_t src_length)
{
	assert(m_bs);
	assert(m_bs->isOpen());

	if (m_location.size() < src_length) {
		std::cerr << "WARNING: location size:" << m_location.size() << " < src len:" << src_length << "\n";
		fail(true);
		return -1;
	}

	assert(m_location.size() >= src_length);

	size_t  src_rem  = src_length;
	ssize_t nwritten_ = 0;

	while ((src_rem > 0) && (m_location.size() > 0))
	{
		assert(src_rem > 0);
		assert(m_location.size() > 0);

		fetch_block();
		assert(m_dst_block);
		assert(m_dst_block->index() == m_location.block_id());

		size_t dst_block_avail = min(static_cast<size_t>(BLOCKSIZE - m_location.offset()), m_location.size());
		assert(dst_block_avail <= m_location.size());

		size_t amount = min(dst_block_avail, src_rem);
		assert(m_location.offset() + amount <= BLOCKSIZE);
		assert(m_location.size() >= amount);
		memcpy(m_dstp /* m_dst_block->data() + m_dst_offset */, srcp, amount);
		// block_put(*dst_block);
		m_dstp       += amount;
		srcp         += amount;
		src_rem      -= amount;
		m_location.consume(amount);		// move and shrink
		nwritten_    += static_cast<ssize_t>(amount);
	}
	assert(src_rem == 0);
	/* assert(dst_avail >= 0); */

	m_nwritten += static_cast<size_t>(nwritten_);
	if (nwritten_ != static_cast<ssize_t>(src_length))
		fail(true);
	return nwritten_;
}

template <size_t BLOCKSIZE, int CACHE_SIZE>
inline ssize_t ReadStream<BLOCKSIZE, CACHE_SIZE>::read(std::string& dst, size_t dst_length)
{
	char *dst_data;
	if ((dst_length + 1) < sizeof(m_fast_data))
		dst_data = m_fast_data;
	else
		dst_data = new char[dst_length + 1];
	char *dstp = dst_data;

	ssize_t nread_ = read(dstp, dst_length);
	dstp[dst_length] = '\0';

	if (nread_ >= 0)
	{
		assert(nread_ >= static_cast<ssize_t>(dst_length));

		dst.clear();
		dst.reserve(static_cast<size_t>(nread_));
		dst.resize(static_cast<size_t>(nread_));
		dst.assign(dst_data, static_cast<size_t>(nread_));
		if (dst_data != m_fast_data)
		{
			delete[] dst_data;
			dst_data = NULL;
		}
	}

	if (nread_ != static_cast<ssize_t>(dst_length))
		fail(true);
	return nread_;
}

template <size_t BLOCKSIZE, int CACHE_SIZE>
inline ssize_t ReadStream<BLOCKSIZE, CACHE_SIZE>::read(char *dstp, size_t dst_length)
{
	assert(m_bs);
	assert(m_bs->isOpen());

	if (m_location.size() < dst_length) {
		std::cerr << "NO SPACE: location-size:" << m_location.size() << " dst-len:" << dst_length << "\n";
		fail(true);
		return -1;
	}

	assert(m_location.size() >= dst_length);

	size_t  length = dst_length;
	ssize_t nread_  = 0;
	assert(length <= m_location.size());

	while (length > 0)
	{
		assert(m_location.size() > 0);
		assert(length > 0);

		fetch_block();
		assert(m_src_block);
		assert(m_src_block->index() == m_location.block_id());

		size_t src_block_avail = min(static_cast<size_t>(BLOCKSIZE - m_location.offset()), m_location.size());
		assert(src_block_avail <= m_location.size());

		size_t amount = min(src_block_avail, length);
		assert(m_location.offset() + amount <= BLOCKSIZE);
		assert(m_location.size() >= amount);
		memcpy(dstp, m_srcp /* m_src_block->data() + m_src_offset */, amount);
		// block_put(*dst_block);
		dstp         += amount;
		length       -= amount;
		m_srcp       += amount;
		m_location.consume(amount);		// move and shrink
		nread_        += static_cast<ssize_t>(amount);
	}
	assert(length == 0);
	/* assert(src_avail >= 0); */

	m_nread += static_cast<size_t>(nread_);
	if (nread_ != static_cast<ssize_t>(dst_length))
		fail(true);
	return nread_;
}

} /* end of namespace milliways */

#endif /* MILLIWAYS_BLOCKSTORAGE_IMPL_H */
