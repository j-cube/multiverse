/*
 * Seriously C++ serialization library.
 *
 * Copyright (C) 2014-2016 Marco Pantaleoni. All rights reserved.
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

#ifndef SERIOUSLY_H
#include "Seriously.h"
#endif

#ifndef SERIOUSLY_IMPL_HPP
//#define SERIOUSLY_IMPL_HPP

#include <functional>

#include <string.h>
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif /* HAVE_ARPA_INET_H */
#include <assert.h>

namespace seriously {

/* ----------------------------------------------------------------- *
 *   Helpers                                                         *
 * ----------------------------------------------------------------- */

#if !defined(HAVE_ARPA_INET_H)

#ifdef bswap16
#undef bswap16
#endif

#ifdef bswap32
#undef bswap32
#endif

static uint16_t bswap16(uint16_t x)
{
	return ((x << 8) & 0xff00) | ((x >> 8) & 0x00ff);
}

static inline uint32_t bswap32(uint32_t x)
{
	return	((x << 24) & 0xff000000 ) |
		((x <<  8) & 0x00ff0000 ) |
		((x >>  8) & 0x0000ff00 ) |
		((x >> 24) & 0x000000ff );
}

#ifndef HAVE_HTONS
#ifdef htons
#undef htons
#endif

static inline uint16_t htons(uint16_t v)
{
#if IS_BIG_ENDIAN
	return v;
#elif IS_LITTLE_ENDIAN
	return bswap16(v);
#else
	#error "Unkown endianness"
#endif
}

static inline uint16_t htons(int16_t v) { return htons(static_cast<uint16_t>(v)); }
#endif /* !defined(HAVE_HTONS) */

#ifndef HAVE_NTOHS
#ifdef ntohs
#undef ntohs
#endif

static inline uint16_t ntohs(uint16_t v)
{
#if IS_BIG_ENDIAN
	return v;
#elif IS_LITTLE_ENDIAN
	return bswap16(v);
#else
	#error "Unkown endianness"
#endif
}

static inline uint16_t ntohs(int16_t v) { return ntohs(static_cast<uint16_t>(v)); }
#endif /* !defined(HAVE_NTOHS) */

#ifndef HAVE_HTONL
#ifdef htonl
#undef htonl
#endif

static inline uint32_t htonl(uint32_t v)
{
#if IS_BIG_ENDIAN
	return v;
#elif IS_LITTLE_ENDIAN
	return bswap32(v);
#else
	#error "Unkown endianness"
#endif
}

static inline uint32_t htonl(int32_t v) { return htonl(static_cast<uint32_t>(v)); }
#endif /* !defined(HAVE_HTONL) */

#ifndef HAVE_NTOHL
#ifdef ntohl
#undef ntohl
#endif

static inline uint32_t ntohl(uint32_t v)
{
#if IS_BIG_ENDIAN
	return v;
#elif IS_LITTLE_ENDIAN
	return bswap32(v);
#else
	#error "Unkown endianness"
#endif
}

static inline uint32_t ntohl(int32_t v) { return ntohl(static_cast<uint32_t>(v)); }
#endif /* !defined(HAVE_NTOHL) */

#endif /* !defined(HAVE_ARPA_INET_H) */

#ifndef HAVE_HTONLL
// based on:
//   http://stackoverflow.com/questions/3022552/is-there-any-standard-htonl-like-function-for-64-bits-integers-in-c
//   http://stackoverflow.com/q/3022552
//   http://stackoverflow.com/a/28592202
static inline uint64_t htonll(uint64_t value)
{
	// The answer is 42
	static const int num = 42;

	// Check the endianness
	if (*reinterpret_cast<const char*>(&num) == num)
	{
		// Little Endian (needs swap for network order)
		uint32_t high_word = htonl(static_cast<uint32_t>(value & 0xFFFFFFFFLL));
		uint32_t low_word  = htonl(static_cast<uint32_t>(value >> 32));
		return (static_cast<uint64_t>(high_word) << 32) | static_cast<uint64_t>(low_word);
	} else
	{
		// Big Endian (Network byte order - RFC1700)
		return value;
	}
}
#endif /* !defined(HAVE_HTONLL) */

#ifndef HAVE_NTOHLL
static inline uint64_t ntohll(uint64_t value)
{
	// The answer is 42
	static const int num = 42;

	// Check the endianness
	if (*reinterpret_cast<const char*>(&num) == num)
	{
		// Little Endian (needs swap for network order)
		uint32_t high_word = htonl(static_cast<uint32_t>(value & 0xFFFFFFFFLL));
		uint32_t low_word  = htonl(static_cast<uint32_t>(value >> 32));
		return (static_cast<uint64_t>(high_word) << 32) | static_cast<uint64_t>(low_word);
	} else
	{
		// Big Endian (Network byte order - RFC1700)
		return value;
	}
}
#endif /* !defined(HAVE_NTOHLL) */

//#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
//#define ntohll(x) ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))

/* ----------------------------------------------------------------- *
 *   Traits                                                          *
 * ----------------------------------------------------------------- */

/* -- bool --------------------------------------------------------- */

inline ssize_t Traits<bool>::serialize(char*& dst, size_t& avail, const type& v)
{
	char* dstp = dst;
	size_t initial_avail = avail;

	if (sizeof(char) > avail)
		return -1;
	assert(avail >= sizeof(char));

	char n_value = v ? 't' : 'f';
	memcpy(dstp, &n_value, sizeof(char));
	dstp += sizeof(char);
	avail -= sizeof(char);

	if (avail > 0)
		*dstp = '\0';

	dst = dstp;
	return static_cast<ssize_t>(initial_avail - avail);
}

inline ssize_t Traits<bool>::deserialize(const char*& src, size_t& avail, type& v)
{
	const char* srcp = src;
	size_t initial_avail = avail;

	if (avail < sizeof(char))
		return -1;
	assert(avail >= sizeof(char));

	char n_value = '\0';
	memcpy(&n_value, srcp, sizeof(char));
	v = (n_value == 't') ? true : false;
	srcp += sizeof(char);
	avail -= sizeof(char);

	src = srcp;
	return static_cast<ssize_t>(initial_avail - avail);
}

/* -- int8_t ------------------------------------------------------- */

inline ssize_t Traits<int8_t>::serialize(char*& dst, size_t& avail, const type& v)
{
	char* dstp = dst;
	size_t initial_avail = avail;

	if (sizeof(type) > avail)
		return -1;
	assert(sizeof(type) <= avail);

	type n_value = v;
	memcpy(dstp, &n_value, sizeof(type));
	dstp += sizeof(type);
	avail -= sizeof(type);

	if (avail > 0)
		*dstp = '\0';

	dst = dstp;
	return static_cast<ssize_t>(initial_avail - avail);
}

inline ssize_t Traits<int8_t>::deserialize(const char*& src, size_t& avail, type& v)
{
	const char* srcp = src;
	size_t initial_avail = avail;

	if (avail < sizeof(type))
		return -1;
	assert(avail >= sizeof(type));

	type n_value = 0;
	memcpy(&n_value, srcp, sizeof(type));
	v = n_value;
	srcp += sizeof(type);
	avail -= sizeof(type);

	src = srcp;
	return static_cast<ssize_t>(initial_avail - avail);
}

/* -- uint8_t ------------------------------------------------------ */

inline ssize_t Traits<uint8_t>::serialize(char*& dst, size_t& avail, const type& v)
{
	char* dstp = dst;
	size_t initial_avail = avail;

	if (sizeof(type) > avail)
		return -1;
	assert(sizeof(type) <= avail);

	type n_value = v;
	memcpy(dstp, &n_value, sizeof(type));
	dstp += sizeof(type);
	avail -= sizeof(type);

	if (avail > 0)
		*dstp = '\0';

	dst = dstp;
	return static_cast<ssize_t>(initial_avail - avail);
}

inline ssize_t Traits<uint8_t>::deserialize(const char*& src, size_t& avail, type& v)
{
	const char* srcp = src;
	size_t initial_avail = avail;

	if (avail < sizeof(type))
		return -1;
	assert(avail >= sizeof(type));

	type n_value = 0;
	memcpy(&n_value, srcp, sizeof(type));
	v = n_value;
	srcp += sizeof(type);
	avail -= sizeof(type);

	src = srcp;
	return static_cast<ssize_t>(initial_avail - avail);
}

/* -- int16_t ------------------------------------------------------ */

inline ssize_t Traits<int16_t>::serialize(char*& dst, size_t& avail, const type& v)
{
	char* dstp = dst;
	size_t initial_avail = avail;

	if (sizeof(type) > avail)
		return -1;
	assert(sizeof(type) <= avail);

	type n_value = static_cast<type>(htons(static_cast<type>(v)));
	memcpy(dstp, &n_value, sizeof(type));
	dstp += sizeof(type);
	avail -= sizeof(type);

	if (avail > 0)
		*dstp = '\0';

	dst = dstp;
	return static_cast<ssize_t>(initial_avail - avail);
}

inline ssize_t Traits<int16_t>::deserialize(const char*& src, size_t& avail, type& v)
{
	const char* srcp = src;
	size_t initial_avail = avail;

	if (avail < sizeof(type))
		return -1;
	assert(avail >= sizeof(type));

	type n_value = 0;
	memcpy(&n_value, srcp, sizeof(type));
	v = static_cast<type>(ntohs(n_value));
	srcp += sizeof(type);
	avail -= sizeof(type);

	src = srcp;
	return static_cast<ssize_t>(initial_avail - avail);
}

/* -- uint16_t ----------------------------------------------------- */

inline ssize_t Traits<uint16_t>::serialize(char*& dst, size_t& avail, const type& v)
{
	char* dstp = dst;
	size_t initial_avail = avail;

	if (sizeof(type) > avail)
		return -1;
	assert(sizeof(type) <= avail);

	type n_value = htons(static_cast<type>(v));
	memcpy(dstp, &n_value, sizeof(type));
	dstp += sizeof(type);
	avail -= sizeof(type);

	if (avail > 0)
		*dstp = '\0';

	dst = dstp;
	return static_cast<ssize_t>(initial_avail - avail);
}

inline ssize_t Traits<uint16_t>::deserialize(const char*& src, size_t& avail, type& v)
{
	const char* srcp = src;
	size_t initial_avail = avail;

	if (avail < sizeof(type))
		return -1;
	assert(avail >= sizeof(type));

	type n_value = 0;
	memcpy(&n_value, srcp, sizeof(type));
	v = ntohs(n_value);
	srcp += sizeof(type);
	avail -= sizeof(type);

	src = srcp;
	return static_cast<ssize_t>(initial_avail - avail);
}

/* -- int32_t ------------------------------------------------------ */

inline ssize_t Traits<int32_t>::serialize(char*& dst, size_t& avail, const type& v)
{
	char* dstp = dst;
	size_t initial_avail = avail;

	if (sizeof(type) > avail)
		return -1;
	assert(sizeof(type) <= avail);

	type n_value = static_cast<type>(htonl(static_cast<type>(v)));
	memcpy(dstp, &n_value, sizeof(type));
	dstp += sizeof(type);
	avail -= sizeof(type);

	if (avail > 0)
		*dstp = '\0';

	dst = dstp;
	return static_cast<ssize_t>(initial_avail - avail);
}

inline ssize_t Traits<int32_t>::deserialize(const char*& src, size_t& avail, type& v)
{
	const char* srcp = src;
	size_t initial_avail = avail;

	if (avail < sizeof(type))
		return -1;
	assert(avail >= sizeof(type));

	type n_value = 0;
	memcpy(&n_value, srcp, sizeof(type));
	v = static_cast<type>(ntohl(n_value));
	srcp += sizeof(type);
	avail -= sizeof(type);

	src = srcp;
	return static_cast<ssize_t>(initial_avail - avail);
}

/* -- uint32_t ----------------------------------------------------- */

inline ssize_t Traits<uint32_t>::serialize(char*& dst, size_t& avail, const type& v)
{
	char* dstp = dst;
	size_t initial_avail = avail;

	if (sizeof(type) > avail)
		return -1;
	assert(sizeof(type) <= avail);

	type n_value = htonl(static_cast<type>(v));
	memcpy(dstp, &n_value, sizeof(type));
	dstp += sizeof(type);
	avail -= sizeof(type);

	if (avail > 0)
		*dstp = '\0';

	dst = dstp;
	return static_cast<ssize_t>(initial_avail - avail);
}

inline ssize_t Traits<uint32_t>::deserialize(const char*& src, size_t& avail, type& v)
{
	const char* srcp = src;
	size_t initial_avail = avail;

	if (avail < sizeof(type))
		return -1;
	assert(avail >= sizeof(type));

	type n_value = 0;
	memcpy(&n_value, srcp, sizeof(type));
	v = ntohl(n_value);
	srcp += sizeof(type);
	avail -= sizeof(type);

	src = srcp;
	return static_cast<ssize_t>(initial_avail - avail);
}

/* -- int64_t ------------------------------------------------------ */

inline ssize_t Traits<int64_t>::serialize(char*& dst, size_t& avail, const type& v)
{
	char* dstp = dst;
	size_t initial_avail = avail;

	if (sizeof(type) > avail)
		return -1;
	assert(sizeof(type) <= avail);

	type n_value = static_cast<type>(htonll(static_cast<uint64_t>(v)));
	memcpy(dstp, &n_value, sizeof(type));
	dstp += sizeof(type);
	avail -= sizeof(type);

	if (avail > 0)
		*dstp = '\0';

	dst = dstp;
	return static_cast<ssize_t>(initial_avail - avail);
}

inline ssize_t Traits<int64_t>::deserialize(const char*& src, size_t& avail, type& v)
{
	const char* srcp = src;
	size_t initial_avail = avail;

	if (avail < sizeof(type))
		return -1;
	assert(avail >= sizeof(type));

	type n_value = 0;
	memcpy(&n_value, srcp, sizeof(type));
	v = static_cast<type>(ntohll(static_cast<uint64_t>(n_value)));
	srcp += sizeof(type);
	avail -= sizeof(type);

	src = srcp;
	return static_cast<ssize_t>(initial_avail - avail);
}

/* -- uint64_t ----------------------------------------------------- */

inline ssize_t Traits<uint64_t>::serialize(char*& dst, size_t& avail, const type& v)
{
	char* dstp = dst;
	size_t initial_avail = avail;

	if (sizeof(type) > avail)
		return -1;
	assert(sizeof(type) <= avail);

	type n_value = htonll(v);
	memcpy(dstp, &n_value, sizeof(type));
	dstp += sizeof(type);
	avail -= sizeof(type);

	if (avail > 0)
		*dstp = '\0';

	dst = dstp;
	return static_cast<ssize_t>(initial_avail - avail);
}

inline ssize_t Traits<uint64_t>::deserialize(const char*& src, size_t& avail, type& v)
{
	const char* srcp = src;
	size_t initial_avail = avail;

	if (avail < sizeof(type))
		return -1;
	assert(avail >= sizeof(type));

	type n_value = 0;
	memcpy(&n_value, srcp, sizeof(type));
	v = ntohll(n_value);
	srcp += sizeof(type);
	avail -= sizeof(type);

	src = srcp;
	return static_cast<ssize_t>(initial_avail - avail);
}

#if ALLOWS_TEMPLATED_SIZE_T

/* -- size_t ------------------------------------------------------- */

inline ssize_t Traits<size_t>::serialize(char*& dst, size_t& avail, const type& v)
{
	typedef uint64_t serialized_type;

	char* dstp = dst;
	size_t initial_avail = avail;

	if (sizeof(serialized_type) > avail)
		return -1;
	assert(sizeof(serialized_type) <= avail);

	serialized_type n_value = htonll(static_cast<serialized_type>(v));
	memcpy(dstp, &n_value, sizeof(serialized_type));
	dstp += sizeof(serialized_type);
	avail -= sizeof(serialized_type);

	if (avail > 0)
		*dstp = '\0';

	dst = dstp;
	return static_cast<ssize_t>(initial_avail - avail);
}

inline ssize_t Traits<size_t>::deserialize(const char*& src, size_t& avail, type& v)
{
	typedef uint64_t serialized_type;

	const char* srcp = src;
	size_t initial_avail = avail;

	if (avail < sizeof(serialized_type))
		return -1;
	assert(avail >= sizeof(serialized_type));

	serialized_type n_value = 0;
	memcpy(&n_value, srcp, sizeof(serialized_type));
	v = static_cast<type>(ntohll(n_value));
	srcp += sizeof(serialized_type);
	avail -= sizeof(serialized_type);

	src = srcp;
	return static_cast<ssize_t>(initial_avail - avail);
}

#endif /* ALLOWS_TEMPLATED_SIZE_T */

#if ALLOWS_TEMPLATED_SSIZE_T

/* -- size_t ------------------------------------------------------- */

inline ssize_t Traits<ssize_t>::serialize(char*& dst, size_t& avail, const type& v)
{
	typedef int64_t serialized_type;

	char* dstp = dst;
	size_t initial_avail = avail;

	if (sizeof(serialized_type) > avail)
		return -1;
	assert(sizeof(serialized_type) <= avail);

	serialized_type n_value = static_cast<serialized_type>(htonll(static_cast<uint64_t>(v)));
	memcpy(dstp, &n_value, sizeof(serialized_type));
	dstp += sizeof(serialized_type);
	avail -= sizeof(serialized_type);

	if (avail > 0)
		*dstp = '\0';

	dst = dstp;
	return static_cast<ssize_t>(initial_avail - avail);
}

inline ssize_t Traits<ssize_t>::deserialize(const char*& src, size_t& avail, type& v)
{
	typedef int64_t serialized_type;

	const char* srcp = src;
	size_t initial_avail = avail;

	if (avail < sizeof(serialized_type))
		return -1;
	assert(avail >= sizeof(serialized_type));

	serialized_type n_value = 0;
	memcpy(&n_value, srcp, sizeof(serialized_type));
	v = static_cast<type>(ntohll(static_cast<uint64_t>(n_value)));
	srcp += sizeof(serialized_type);
	avail -= sizeof(serialized_type);

	src = srcp;
	return static_cast<ssize_t>(initial_avail - avail);
}

#endif /* ALLOWS_TEMPLATED_SSIZE_T */

/* -- std::string -------------------------------------------------- */

inline ssize_t Traits<std::string>::serialize(char*& dst, size_t& avail, const type& v)
{
	char* dstp = dst;
	size_t initial_avail = avail;

	uint32_t s_len = static_cast<uint32_t>(v.length());

	if ((4 + s_len) > avail)
		return -1;
	assert((4 + s_len) <= avail);

	Traits<uint32_t>::serialize(dstp, avail, s_len);

	memcpy(dstp, v.data(), s_len);
	dstp += s_len;
	avail -= s_len;

	if (avail > 0)
		*dstp = '\0';

	dst = dstp;
	return static_cast<ssize_t>(initial_avail - avail);
}

#define FAST_BUFFER_SIZE    4096

inline ssize_t Traits<std::string>::deserialize(const char*& src, size_t& avail, type& v)
{
	const char* srcp = src;
	size_t initial_avail = avail;

	uint32_t s_len = 0;
	if (Traits<uint32_t>::deserialize(srcp, avail, s_len) < 0)
		return -1;

	if (avail < s_len)
		return -1;
	assert(avail >= s_len);

	char fast_data[FAST_BUFFER_SIZE];
	char *data;
	if ((s_len + 1) < FAST_BUFFER_SIZE)
		data = fast_data;
	else
		data = new char[s_len + 1];
	memset(data, 0, s_len);
	memcpy(data, srcp, s_len);
	data[s_len] = '\0';
	v.resize(s_len);
	v.assign(data, s_len);
	srcp += s_len;
	avail -= s_len;
	if (data != fast_data)
	{
		delete[] data;
		data = NULL;
	}

	src = srcp;
	return static_cast<ssize_t>(initial_avail - avail);
}

} /* end of namespace seriously */

#endif /* SERIOUSLY_IMPL_HPP */
