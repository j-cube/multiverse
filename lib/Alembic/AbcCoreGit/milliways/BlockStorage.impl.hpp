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

#ifndef MILLIWAYS_BLOCKSTORAGE_IMPL_H
//#define MILLIWAYS_BLOCKSTORAGE_IMPL_H

namespace milliways {

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
	block_t headerBlock(m_header_block_id);
	if (! read(headerBlock))
		return false;
//	std::cerr << "read header block " << m_header_block_id << " dump:" << std::endl << s_hexdump(headerBlock.data(), 256);

	// deserialize header

	seriously::Packer<BLOCKSIZE> packer(headerBlock.data(), headerBlock.size());
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
	block_t headerBlock(m_header_block_id);

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

	assert(packer.size() <= headerBlock.size());
	memcpy(headerBlock.data(), packer.data(), packer.size());

	if (! write(headerBlock))
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
 *   FileBlockStorage                                                *
 * ----------------------------------------------------------------- */

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
	if (! isOpen())
		return;

	assert(m_stream.is_open());

	m_stream.seekg(0, std::ios_base::end);
	std::ifstream::pos_type pos = m_stream.tellg();
	assert(pos != static_cast<std::ifstream::pos_type>(-1));
	assert((pos % BlockSize) == 0);
	m_count = static_cast<ssize_t>(pos / BlockSize);
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
	m_next_block_id = block_id + n_blocks;
	return block_id;
}

template <size_t BLOCKSIZE, int CACHE_SIZE>
bool FileBlockStorage<BLOCKSIZE, CACHE_SIZE>::dispose(block_id_t block_id, int count)
{
	if (block_id == BLOCK_ID_INVALID)
		return false;

	assert(block_id != BLOCK_ID_INVALID);

	nextId();	// force update of m_next_block_id if necessary
	if (m_next_block_id == (block_id + count))
		m_next_block_id -= count;

	return true;
}

template <size_t BLOCKSIZE, int CACHE_SIZE>
bool FileBlockStorage<BLOCKSIZE, CACHE_SIZE>::read(block_t& dst)
{
	// std::cerr << "FBS::read(" << dst.index() << ")" << std::endl;
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
	// std::cerr << "FBS::write(" << src.index() << ")" << std::endl;
	assert(src.index() != BLOCK_ID_INVALID);

	size_t pos = src.index() * BlockSize;

	try {
		m_stream.seekp(static_cast<std::streamoff>(pos));
	} catch (std::ios::failure) {
		assert(false);
		return false;
	}

	try {
		m_stream.write(src.data(), BlockSize);
	} catch (std::ios_base::failure& e) {
		std::cerr << "error reading block " << src.index() << ":" << e.what() << std::endl;
		assert(false);
		return false;
	}

	if (m_stream.fail())
	{
		std::cerr << "stream fail writing block " << src.index() << ", error: " << strerror(errno) << std::endl;
		assert(false);
		return false;
	}

	assert(! m_stream.fail());

	pos += BlockSize;
	if (pos >= (m_count * BlockSize))
		m_count = static_cast<ssize_t>(pos / BlockSize);

	// std::cerr << "FBS::write(" << src.index() << ") block dump:" << std::endl << s_hexdump(src.data(), 128) << std::endl;
	return true;
}

/* cached I/O */

template <size_t BLOCKSIZE, int CACHE_SIZE>
typename FileBlockStorage<BLOCKSIZE, CACHE_SIZE>::block_t* FileBlockStorage<BLOCKSIZE, CACHE_SIZE>::get(block_id_t block_id)
{
	return m_lru[block_id];
}

template <size_t BLOCKSIZE, int CACHE_SIZE>
bool FileBlockStorage<BLOCKSIZE, CACHE_SIZE>::put(const block_t& src)
{
	block_t *cached = m_lru[src.index()];
	if (cached)
	{
		assert(cached->index() == src.index());
		*cached = src;
		return true;
	}
	return false;
}

} /* end of namespace milliways */

#endif /* MILLIWAYS_BLOCKSTORAGE_IMPL_H */
