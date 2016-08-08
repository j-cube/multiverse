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

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include "lz4.h"

#ifndef MILLIWAYS_LZ4PACKER_IMPL_H
//#define MILLIWAYS_LZ4PACKER_IMPL_H

namespace milliways {

static const size_t MEM_LZ4_BLOCK_SIZE = 1024 * 1024 * 8;
static const int MEM_N_LZ4_BLOCK_BUFFERS = 2;
static const int MEM_LZ4_ACCELERATION = MILLIWAYS_LZ4_ACCELERATION;

static const int LZ4_PACKER_USER_SIZE = MEM_LZ4_BLOCK_SIZE;
static const int LZ4_PACKER_EFFECTIVE_SIZE = LZ4_COMPRESSBOUND(LZ4_PACKER_USER_SIZE);

template <typename T>
inline T lz4p_min(const T& a, const T& b)
{
	return (a <= b) ? a : b;
}

class Lz4Packer
{
public:
	static const size_t CMPBUF_SIZE = LZ4_COMPRESSBOUND(MEM_LZ4_BLOCK_SIZE);
	static const size_t IOBUF_EL_SIZE = MEM_LZ4_BLOCK_SIZE;

	static const int Size = LZ4_PACKER_USER_SIZE;
	static const int EffectiveSize = LZ4_PACKER_EFFECTIVE_SIZE;
	typedef seriously::Packer<LZ4_PACKER_EFFECTIVE_SIZE> Packer;
	typedef Lz4Packer self_type;

	Lz4Packer() :
		m_buffer(NULL), m_dstp(NULL), m_dst_avail(0),
		m_srcp(NULL), m_src_avail(0),
		m_length(0), m_error(false), m_cmpBuf(NULL), m_ioBuf() { setup(); }
	Lz4Packer(std::string data_) :
		m_buffer(NULL), m_dstp(NULL), m_dst_avail(0),
		m_srcp(NULL), m_src_avail(0),
		m_length(0), m_error(false), m_cmpBuf(NULL), m_ioBuf() { setup(); data(data_); }
	Lz4Packer(const char *data_, size_t size_) :
		m_buffer(NULL), m_dstp(NULL), m_dst_avail(0),
		m_srcp(NULL), m_src_avail(0),
		m_length(0), m_error(false), m_cmpBuf(NULL), m_ioBuf() { setup(); data(data_, size_); }
	Lz4Packer(const Lz4Packer& other) :
		m_buffer(NULL), m_dstp(NULL), m_dst_avail(0),
		m_srcp(NULL), m_src_avail(0),
		m_length(0), m_error(false), m_cmpBuf(NULL), m_ioBuf() { setup(); data(other.data(), other.size()); }
	~Lz4Packer() {
		char *space = m_buffer;
		if (space)
			delete[] space;
		m_buffer = NULL;
		m_dstp = NULL;
		m_srcp = NULL;
		m_ioBuf[0] = NULL;
		m_ioBuf[1] = NULL;
		m_cmpBuf = NULL;
	}

	void setup() {
		char *space = new char[EffectiveSize + CMPBUF_SIZE + (MEM_N_LZ4_BLOCK_BUFFERS * IOBUF_EL_SIZE)];
		assert(space);
		m_buffer = space;
		m_dstp = m_buffer;
		m_dst_avail = EffectiveSize;
		m_srcp = m_buffer;
		m_src_avail = 0;

		m_cmpBuf = space + EffectiveSize;
		char *iob = space + EffectiveSize + CMPBUF_SIZE;
		m_ioBuf[0] = &iob[0];
		m_ioBuf[1] = &iob[IOBUF_EL_SIZE];
	}

	const char* data() const { return m_buffer; }
	bool data(const char *data_, size_t size_) {
		if (size_ > EffectiveSize)
			return false;
		memcpy(m_buffer, data_, size_);
		if (size_ < EffectiveSize)
			m_buffer[size_] = '\0';
		m_length = size_;
		m_dstp = m_buffer;
		m_dst_avail = EffectiveSize - m_length;
		m_srcp = m_buffer;
		m_src_avail = m_length;
		m_error = false;
		return true;
	}
	bool data(const std::string& data_) {
		return data(data_.data(), data_.size());
	}

	size_t size() const { return m_length; }
	size_t maxsize() const { return EffectiveSize; }

	size_t packing_avail() const { return m_dst_avail; }
	size_t unpacking_avail() const { return m_src_avail; }
	bool error() const { return m_error; }

	Lz4Packer& rewind() { m_dstp = m_buffer; m_dst_avail = EffectiveSize; m_length = 0; m_srcp = m_buffer; m_src_avail = 0; m_error = false; return *this; }
	Lz4Packer& unpacking_rewind() { m_srcp = m_buffer; m_src_avail = m_length; return *this; }

	template <typename T>
	ssize_t put(const T& value)
	{
		if (m_dst_avail < seriously::Traits<T>::serializedsize(value))
		{
			seterror();
			return -1;
		}
		ssize_t used = seriously::Traits<T>::serialize(m_dstp, m_dst_avail, value);
		if (used >= 0) {
			m_length += static_cast<size_t>(used);
			m_src_avail += static_cast<size_t>(used);
		} else
			seterror();
		return used;
	}

	ssize_t put(const char* srcp, size_t nbytes)
	{
		if (m_dst_avail < nbytes)
		{
			seterror();
			return -1;
		}
		memcpy(m_dstp, srcp, nbytes);
		m_dstp += nbytes;
		m_dst_avail -= nbytes;
		m_length += nbytes;
		m_src_avail += nbytes;
		return nbytes;
	}

	template <typename T>
	ssize_t get(T& value)
	{
		if (m_src_avail < seriously::Traits<T>::serializedsize(value))
		{
			seterror();
			return -1;
		}
		ssize_t consumed = seriously::Traits<T>::deserialize(m_srcp, m_src_avail, value);
		if (consumed < 0)
			seterror();
		return consumed;
	}

	ssize_t get(char* dstp, size_t nbytes)
	{
		if (m_src_avail < nbytes)
		{
			seterror();
			return -1;
		}
		memcpy(dstp, m_srcp, nbytes);
		m_srcp += nbytes;
		m_src_avail -= nbytes;
		return nbytes;
	}

	ssize_t lz4_read(std::string& dst, size_t nbytes, ssize_t& compressed_size)
	{
		const size_t BS_C_FAST_BUFFER_SIZE = 8192;

		char fast_data[BS_C_FAST_BUFFER_SIZE];
		char *dst_data;
		if ((nbytes + 1) < sizeof(fast_data))
			dst_data = fast_data;
		else
			dst_data = new char[nbytes + 1];
		char *dstp = dst_data;

		ssize_t nread_u = lz4_read(dstp, nbytes, compressed_size);
		if (nread_u >= 0)
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

		return nread_u;
	}

	ssize_t lz4_read(char*& dstp, size_t nbytes, ssize_t& compressed_size)
	{
	    LZ4_streamDecode_t lz4StreamDecode_body;
	    LZ4_streamDecode_t* lz4StreamDecode = &lz4StreamDecode_body;
	    int  decBufIndex = 0;

	    LZ4_setStreamDecode(lz4StreamDecode, NULL, 0);

		size_t  to_read = nbytes;
		size_t  nread   = 0;
		size_t  nread_u = 0;
		ssize_t nr;

		memset(m_cmpBuf, 0, CMPBUF_SIZE);
		memset(m_ioBuf[0], 0, IOBUF_EL_SIZE);
		memset(m_ioBuf[1], 0, IOBUF_EL_SIZE);

		while (to_read > 0)
		{
			assert(to_read > 0);

			uint32_t cmpBytes = 0;

			nr = get(cmpBytes);
	        if (nr < 0)
	            break;			/* failure */
			nread += static_cast<size_t>(nr);

			nr = get(m_cmpBuf, cmpBytes);
			if (nr < 0)
	            break;			/* failure */
	        assert(nr == (ssize_t)cmpBytes);
			nread += static_cast<size_t>(cmpBytes);

		    char* const decPtr = m_ioBuf[decBufIndex];
		    const int decBytes = LZ4_decompress_safe_continue(
		        lz4StreamDecode, m_cmpBuf, decPtr, (int) cmpBytes, IOBUF_EL_SIZE);
		    if (decBytes <= 0) {
				std::cerr << "FAIL 3: decBytes=" << decBytes << "  cmpBytes=" << cmpBytes << "  MEM_LZ4_BLOCK_SIZE:" << MEM_LZ4_BLOCK_SIZE << std::endl;
				seterror();
		        break;		/* failure */
		    }

		    size_t to_copy = lz4p_min((size_t) to_read, (size_t) decBytes);
			assert(to_copy <= to_read);
		    memcpy(dstp, decPtr, to_copy);
			dstp    += to_copy; // (size_t) decBytes;
			nread_u += to_copy; // (size_t) decBytes;
			to_read -= to_copy; // (ssize_t) decBytes;

			decBufIndex = (decBufIndex + 1) % MEM_N_LZ4_BLOCK_BUFFERS;
		}

		if (to_read != 0) {
			/* failure */
			seterror();
			std::cerr << "ERROR: to_read (" << to_read << ") != 0 - nbytes:" << nbytes << " nread:" << nread << " nread_u:" << nread_u << "\n";
			return -1;
		}

		assert(to_read == 0);
		assert(nread_u == nbytes);

		compressed_size = static_cast<ssize_t>(nread);
		return static_cast<ssize_t>(nread_u);
	}

	ssize_t lz4_write(const std::string& src, ssize_t& compressed_size) { const char *srcp = src.data(); return lz4_write(srcp, src.length(), compressed_size); }

	ssize_t lz4_write(const char*& srcp, size_t nbytes, ssize_t& compressed_size)
	{
	    LZ4_stream_t lz4Stream_body;
	    LZ4_stream_t* lz4Stream = &lz4Stream_body;
	    int  inpBufIndex = 0;

		compressed_size = -1;

	    LZ4_resetStream(lz4Stream);

		size_t  to_write   = nbytes;
		size_t  nwritten_u = 0;
		size_t  nwritten   = 0;
		ssize_t nw;

		memset(m_cmpBuf, 0, CMPBUF_SIZE);
		memset(m_ioBuf[0], 0, IOBUF_EL_SIZE);
		memset(m_ioBuf[1], 0, IOBUF_EL_SIZE);

		while ((to_write > 0) && (packing_avail() > 0))
		{
			assert(to_write > 0);

			size_t amount = lz4p_min(MEM_LZ4_BLOCK_SIZE, to_write);

			// compress 'amount' bytes from srcp - using our double buffer
	        char* const inpPtr = m_ioBuf[inpBufIndex];
	        assert(amount <= MEM_LZ4_BLOCK_SIZE);
	        memcpy(inpPtr, srcp, amount);
	        srcp += amount;

	        const int cmpBytes = LZ4_compress_fast_continue(
	            lz4Stream, inpPtr, m_cmpBuf, static_cast<int>(amount), CMPBUF_SIZE, /* acceleration */ MEM_LZ4_ACCELERATION);
	        if (cmpBytes <= 0) {
	        	std::cerr << "Lz4Packer::lz4_write FAIL {1}" << std::endl;
				seterror();
	            break;			/* failure */
	        }

			nw = put(static_cast<uint32_t>(cmpBytes));
	        if (nw < 0) {
	        	std::cerr << "Lz4Packer::lz4_write FAIL {3}" << std::endl;
	            break;			/* failure */
	        }
			nwritten += static_cast<size_t>(nw);

			nw = put(m_cmpBuf, cmpBytes);
			if (nw < 0)
			{
	        	std::cerr << "Lz4Packer::lz4_write FAIL {4}" << std::endl;
	            break;			/* failure */
			}
			assert(nw == cmpBytes);
			nwritten   += static_cast<size_t>(cmpBytes);
			nwritten_u += amount;

	        inpBufIndex = (inpBufIndex + 1) % MEM_N_LZ4_BLOCK_BUFFERS;

			to_write -= amount;
		}

		if (to_write > 0) {
			/* failure */
			seterror();
			std::cerr << "Lz4Packer FAILED lz4_write() (to_write:" << to_write << " nwritten:" << nwritten << " nwritten_u:" << nwritten_u << ")" << "\n";
			return -1;
		}

		assert(to_write == 0);
		assert(nwritten_u == nbytes);
		/* assert(dst_avail >= 0); */

		compressed_size = static_cast<ssize_t>(nwritten);
		return static_cast<ssize_t>(nwritten_u);
	}

protected:
	bool seterror(bool value = true) { bool old = m_error; m_error = value; return old; }

	char*& dstp()                           { return m_dstp; }
	size_t& dst_avail()                     { return m_dst_avail; }
	char* dstp_advance(size_t amount)       { m_dstp += amount; m_dst_avail -= amount; m_length += amount; return m_dstp; }

	const char*& srcp()                     { return m_srcp; }
	size_t& src_avail()                     { return m_src_avail; }
	const char* srcp_advance(size_t amount) { m_srcp += amount; m_src_avail -= amount; return m_srcp; }

private:
	Lz4Packer& operator= (const Lz4Packer& other);

	char *m_buffer;
	char* m_dstp;
	size_t m_dst_avail;
	const char* m_srcp;
	size_t m_src_avail;
	size_t m_length;
	bool m_error;

	char *m_cmpBuf;
    char *m_ioBuf[MEM_N_LZ4_BLOCK_BUFFERS];

	template <typename T>
	friend inline Lz4Packer& operator<< (Lz4Packer& os, const T& value)
	{
		os.put(value);
		return os;
	}

	template <typename T>
	friend inline Lz4Packer& operator>> (Lz4Packer& os, T& value)
	{
		os.get(value);
		return os;
	}
};


} /* end of namespace milliways */

#endif /* MILLIWAYS_LZ4PACKER_IMPL_H */
