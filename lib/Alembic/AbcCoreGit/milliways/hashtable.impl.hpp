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

#ifndef MILLIWAYS_HASHTABLE_H
#include "hashtable.h"
#endif

#ifndef MILLIWAYS_HASHTABLE_IMPL_H
#define MILLIWAYS_HASHTABLE_IMPL_H

#include <algorithm>

#include <stdint.h>
#include <assert.h>

#include "config.h"

namespace milliways {

static long some_primes[] = {
    3, 31, 101, 199, 307, 503, 601, 997, 2003, 4001, 7993, 9001, 12007, 16001, 24001, 32003, 48017, 64007, 96001, 128021, 256019
};

/* ----------------------------------------------------------------- *
 *   hash functions                                                  *
 * ----------------------------------------------------------------- */

static inline long prime_lt(long value) {
	size_t nprimes = sizeof(some_primes) / sizeof(long);
	for (size_t i = 0; i < nprimes; ++i)
		if (some_primes[i] > value)
			return some_primes[i-1];
	return 3;
}

static inline long prime_gt(long value) {
	size_t nprimes = sizeof(some_primes) / sizeof(long);
	for (size_t i = 0; i < nprimes; ++i)
		if (some_primes[i] > value)
			return some_primes[i];
	return value;
}

/* from http://stackoverflow.com/a/12996028 */
static inline uint32_t u32hash(uint32_t x)
{
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x);
    return x;
}

static inline uint64_t u64hash(uint64_t x)
{
	return u32hash((uint32_t)(x >> 32)) ^ u32hash((uint32_t)(x & 0xFFFFFFFF));
}

/* from https://code.google.com/p/smhasher/wiki/MurmurHash3 */
static inline uint32_t u32hash2(uint32_t h)
{
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	return h;
}

static inline uint64_t u64hash2(uint64_t h)
{
	h ^= h >> 33;
	h *= 0xff51afd7ed558ccd;
	h ^= h >> 33;
	h *= 0xc4ceb9fe1a85ec53;
	h ^= h >> 33;
	return h;
}

/*
 * http://stackoverflow.com/a/7666577
 * http://www.cse.yorku.ca/~oz/hash.html
 * http://dmytry.blogspot.it/2009/11/horrible-hashes.html
 */
static inline uint32_t s_hash32(const char *str)
{
	const unsigned char *s = (const unsigned char *)str;
    uint32_t hash = 5381;
    unsigned char c;

    while ((c = *s++))
        hash = ((hash << 5) + hash) + static_cast<uint32_t>(c); /* hash * 33 + c */

    return hash;
}

static inline uint32_t s_hash32(const char *str, size_t len)
{
	const unsigned char *s = (const unsigned char *)str;
    uint32_t hash = 5381;

    while (len > 0) {
        hash = ((hash << 5) + hash) + static_cast<uint32_t>(*s++); /* hash * 33 + c */
        len--;
    }

    return hash;
}

static inline uint64_t s_hash64(const char *str)
{
	const unsigned char *s = (const unsigned char *)str;
    uint64_t hash = 5381;
    unsigned char c;

    while ((c = *s++))
        hash = ((hash << 5) + hash) ^ static_cast<uint64_t>(c); /* hash * 33 xor c */

    return hash;
}

static inline uint64_t s_hash64(const char *str, size_t len)
{
	const unsigned char *s = (const unsigned char *)str;
    uint64_t hash = 5381;

    while (len > 0) {
        hash = ((hash << 5) + hash) ^ static_cast<uint64_t>(*s++); /* hash * 33 xor c */
        len--;
    }

    return hash;
}

/* https://en.wikipedia.org/wiki/Jenkins_hash_function#one-at-a-time */
static inline uint32_t jenkins_one_at_a_time_hash(const char *key, size_t len)
{
    uint32_t hash, i;
    for (hash = i = 0; i < len; ++i)
    {
        hash += static_cast<uint32_t>(key[i]);
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

/* ----------------------------------------------------------------- *
 *   Hasher                                                          *
 * ----------------------------------------------------------------- */

template <>
struct Hasher<bool> {
	typedef bool key_type;
	inline static size_t hash(const key_type& key) { return /* primes */ (key ? 1231 : 8273); }
	static size_t hash2(const key_type& key)       { return /* primes */ (key ? 44651 : 17959); }
};

template <>
struct Hasher<uint32_t> {
	typedef uint32_t key_type;
	inline static size_t hash(const key_type& key) { return static_cast<size_t>(u32hash(static_cast<uint32_t>(key))); }
	static size_t hash2(const key_type& key)       { return static_cast<size_t>(u32hash2(static_cast<uint32_t>(key))); }
};

template <>
struct Hasher<int32_t> {
	typedef int32_t key_type;
	inline static size_t hash(const key_type& key) { return static_cast<size_t>(u32hash(static_cast<uint32_t>(key))); }
	static size_t hash2(const key_type& key)       { return static_cast<size_t>(u32hash2(static_cast<uint32_t>(key))); }
};

template <>
struct Hasher<uint64_t> {
	typedef uint64_t key_type;
	inline static size_t hash(const key_type& key) { return static_cast<size_t>(u64hash(static_cast<uint64_t>(key))); }
	static size_t hash2(const key_type& key)       { return static_cast<size_t>(u64hash2(static_cast<uint64_t>(key))); }
};

template <>
struct Hasher<int64_t> {
	typedef int64_t key_type;
	inline static size_t hash(const key_type& key) { return static_cast<size_t>(u64hash(static_cast<uint64_t>(key))); }
	static size_t hash2(const key_type& key)       { return static_cast<size_t>(u64hash2(static_cast<uint64_t>(key))); }
};

template <>
struct Hasher<std::string> {
	typedef std::string key_type;
	inline static size_t hash(const key_type& key) { return static_cast<size_t>(jenkins_one_at_a_time_hash(key.data(), key.size())); }
	static size_t hash2(const key_type& key)       { return static_cast<size_t>(s_hash64(key.data(), key.size())); }
};

/* Define to 1 if an explicit template for size_t is allowed even if all the uint*_t types are there */
#ifdef ALLOWS_TEMPLATED_SIZE_T
	template <>
	struct Hasher<size_t> {
		typedef size_t key_type;
		inline static size_t hash(const key_type& key) { return static_cast<size_t>(u64hash(static_cast<uint64_t>(key))); }
		static size_t hash2(const key_type& key)       { return static_cast<size_t>(u64hash2(static_cast<uint64_t>(key))); }
	};

	template <>
	struct Hasher<ssize_t> {
		typedef ssize_t key_type;
		inline static size_t hash(const key_type& key) { return static_cast<size_t>(u64hash(static_cast<uint64_t>(key))); }
		static size_t hash2(const key_type& key)       { return static_cast<size_t>(u64hash2(static_cast<uint64_t>(key))); }
	};
#endif

/* ----------------------------------------------------------------- *
 *   hashtable                                                       *
 * ----------------------------------------------------------------- */

template < typename Key, typename T, class KeyCompare >
hashtable<Key, T, KeyCompare>::hashtable() :
	m_buckets(NULL), m_capacity(0), m_size(0), m_prime_lt_capacity(0)
{
	size_type capacity_ = DEFAULT_CAPACITY;

	m_buckets = new bucket[capacity_];
	assert(m_buckets);
	m_capacity = capacity_;

	compute_prime_lt_capacity();
}

template < typename Key, typename T, class KeyCompare >
hashtable<Key, T, KeyCompare>::hashtable(size_type for_size) :
	m_buckets(NULL), m_capacity(0), m_size(0)
{
	size_type initial_capacity = capacity_for(for_size);

	m_buckets = new bucket[initial_capacity];
	assert(m_buckets);
	m_capacity = initial_capacity;

	compute_prime_lt_capacity();
}

template < typename Key, typename T, class KeyCompare >
hashtable<Key, T, KeyCompare>::~hashtable()
{
	if (m_buckets)
		delete[] m_buckets;
	m_buckets = NULL;
	m_capacity = 0;
	m_size = 0;
	m_prime_lt_capacity = 0;
}

template < typename Key, typename T, class KeyCompare >
typename hashtable<Key, T, KeyCompare>::hash_type hashtable<Key, T, KeyCompare>::compute_hash2(const key_type& /* key */) const
{
	return 3;
	// hash_type prime = static_cast<hash_type>(m_prime_lt_capacity);
	// assert(prime > 0);

	// return (prime - (Hasher<Key>::hash2(key) % prime)) + 1;		 the +1 is to avoid h2 to be zero (with nefarious consequences) 
}

template < typename Key, typename T, class KeyCompare >
bool hashtable<Key, T, KeyCompare>::has(const key_type& key) const
{
	return find_bucket(key) ? true : false;
}

template < typename Key, typename T, class KeyCompare >
bool hashtable<Key, T, KeyCompare>::get(const key_type& key, mapped_type& value) const
{
	const bucket* found = find_bucket(key);
	if (found) {
		value = found->value();
		return true;
	}

	return false;
}

template < typename Key, typename T, class KeyCompare >
hashtable<Key, T, KeyCompare>& hashtable<Key, T, KeyCompare>::set(const key_type& key, const mapped_type& value)
{
	/* bucket* bkp = */ set_(key, value);
	return *this;
}

template < typename Key, typename T, class KeyCompare >
typename hashtable<Key, T, KeyCompare>::bucket* hashtable<Key, T, KeyCompare>::set_(const key_type& key, const mapped_type& value)
{
	hash_type h1, h2, hf;
	size_t i;
	size_t pos;

	if (((m_size * 100) / m_capacity) >= hashtable::MAX_LOAD_FACTOR)
		expand(0);

	int n_expand_cycles = 0;

restart:
	i = 0;
	h1 = Hasher<Key>::hash(key);
	h2 = 0;
	hf = h1;
	pos = hf % m_capacity;

	while (m_buckets[pos].notFree() && (i < m_capacity))
	{
		if (KeyCompare()(key, m_buckets[pos].key())) {
			m_buckets[pos].value(value);
			m_buckets[pos].state(bucket::USED);
			return &m_buckets[pos];
		}

		if (i == 0) {
			h2 = compute_hash2(key);
			assert(h2 > 0);
		}

		i++;
		hf += h2;								/* hf = h1 + i * h2; */
		pos = hf % m_capacity;
	}

	/*
	 * it could happen that we have not found a free spot, for example
	 * because all buckets are deleted.
	 */
	if (i >= m_capacity)
	{
		assert(n_expand_cycles < 2);

		n_expand_cycles++;
		expand(m_size + 1);
		goto restart;
	}

	assert(i < m_capacity);

	bucket& bk = m_buckets[pos];

	if (bk.notFree())
	{
		expand();
		goto restart;
	}

	assert(bk.isFree());

	bk.key(key);
	bk.value(value);
	bk.state(bucket::USED);

	m_size++;
	return &bk;
}

template < typename Key, typename T, class KeyCompare >
bool hashtable<Key, T, KeyCompare>::erase(const key_type& key)
{
	bucket* found = find_bucket(key);
	if (found) {
		found->key(Key());
		found->value(T());
		found->state(bucket::DELETED);
		m_size--;
		return true;
	}

	return false;
}

template < typename Key, typename T, class KeyCompare >
void hashtable<Key, T, KeyCompare>::clear()
{
	for (size_t i = 0; i < m_capacity; i++)
	{
		bucket& bk = m_buckets[i];
		if (bk.notFree())
		{
			bk.key(Key());
			bk.value(T());
			bk.state(bucket::FREE);
			m_size--;
		}
	}
	assert(m_size == 0);
}

template < typename Key, typename T, class KeyCompare >
typename hashtable<Key, T, KeyCompare>::size_type hashtable<Key, T, KeyCompare>::capacity_for(size_type size_)
{
	size_t new_capacity = static_cast<size_t>(prime_gt(static_cast<long>(size_ * hashtable::EXPANSION_FACTOR + 37)));
	while ((((size_ + 1) * 100) / new_capacity) >= hashtable::MAX_LOAD_FACTOR)
		new_capacity += 23;

	return new_capacity;
}

template < typename Key, typename T, class KeyCompare >
bool hashtable<Key, T, KeyCompare>::expand(size_t for_size)
{
	if (for_size == 0)
		for_size = static_cast<size_t>( m_size * hashtable::EXPANSION_FACTOR + 1 );

	hashtable lamb(for_size);

	for (size_t i = 0; i < m_capacity; i++)
	{
		bucket& bk = m_buckets[i];
		if (bk.isUsed())
		{
			lamb.set(bk.key(), bk.value());
		}
	}

	/* surgery... */
	bucket* tmp_buckets = m_buckets;
	size_type tmp_capacity = m_capacity;
	size_type tmp_size = m_size;
	size_type tmp_prime_lt_capacity = m_prime_lt_capacity;

	m_buckets = lamb.m_buckets;
	m_capacity = lamb.m_capacity;
	m_size = lamb.m_size;
	m_prime_lt_capacity = lamb.m_prime_lt_capacity;

	lamb.m_buckets = tmp_buckets;
	lamb.m_capacity = tmp_capacity;
	lamb.m_size = tmp_size;
	lamb.m_prime_lt_capacity = tmp_prime_lt_capacity;

	return true;
}

template < typename Key, typename T, class KeyCompare >
const typename hashtable<Key, T, KeyCompare>::bucket* hashtable<Key, T, KeyCompare>::find_bucket(const key_type& key, ssize_t& pos_) const
{
	size_t i = 0;
	hash_type h1 = Hasher<Key>::hash(key);
	hash_type h2, hf = h1;
	size_t pos = h1 % m_capacity;

	while (m_buckets[pos].notFree() && (i < m_capacity))
	{
		if (m_buckets[pos].isUsed() && KeyCompare()(key, m_buckets[pos].key())) {
			pos_ = static_cast<ssize_t>(pos);
			return &m_buckets[pos];
		}

		if (i == 0)
			h2 = compute_hash2(key);

		hf += h2;								/* hf = h1 + i * h2; */
		pos = hf % m_capacity;
		i++;
	}

	pos_ = -1;
	return NULL;
}

template < typename Key, typename T, class KeyCompare >
const typename hashtable<Key, T, KeyCompare>::bucket* hashtable<Key, T, KeyCompare>::find_bucket(const key_type& key) const
{
	size_t i = 0;
	hash_type h1 = Hasher<Key>::hash(key);
	hash_type h2, hf = h1;
	size_t pos = h1 % m_capacity;

	while (m_buckets[pos].notFree() && (i < m_capacity))
	{
		if (m_buckets[pos].isUsed() && KeyCompare()(key, m_buckets[pos].key())) {
			return &m_buckets[pos];
		}

		if (i == 0)
			h2 = compute_hash2(key);

		hf += h2;								/* hf = h1 + i * h2; */
		pos = hf % m_capacity;
		i++;
	}

	return NULL;
}

template < typename Key, typename T, class KeyCompare >
typename hashtable<Key, T, KeyCompare>::bucket* hashtable<Key, T, KeyCompare>::find_bucket(const key_type& key, ssize_t& pos_)
{
	size_t i = 0;
	hash_type h1 = Hasher<Key>::hash(key);
	hash_type h2, hf = h1;
	size_t pos = h1 % m_capacity;

	while (m_buckets[pos].notFree() && (i < m_capacity))
	{
		if (m_buckets[pos].isUsed() && KeyCompare()(key, m_buckets[pos].key())) {
			pos_ = static_cast<ssize_t>(pos);
			return &m_buckets[pos];
		}

		if (i == 0)
			h2 = compute_hash2(key);

		hf += h2;								/* hf = h1 + i * h2; */
		pos = hf % m_capacity;
		i++;
	}

	pos_ = -1;
	return NULL;
}

template < typename Key, typename T, class KeyCompare >
typename hashtable<Key, T, KeyCompare>::bucket* hashtable<Key, T, KeyCompare>::find_bucket(const key_type& key)
{
	size_t i = 0;
	hash_type h1 = Hasher<Key>::hash(key);
	hash_type h2, hf = h1;
	size_t pos = h1 % m_capacity;

	while (m_buckets[pos].notFree() && (i < m_capacity))
	{
		if (m_buckets[pos].isUsed() && KeyCompare()(key, m_buckets[pos].key())) {
			return &m_buckets[pos];
		}

		if (i == 0)
			h2 = compute_hash2(key);

		hf += h2;								/* hf = h1 + i * h2; */
		pos = hf % m_capacity;
		i++;
	}

	return NULL;
}

template < typename Key, typename T, class KeyCompare >
void hashtable<Key, T, KeyCompare>::compute_prime_lt_capacity()
{
	if (m_capacity > 0)
	{
		m_prime_lt_capacity = static_cast<size_type>(prime_lt(static_cast<long>(m_capacity)));
		if (m_prime_lt_capacity > m_capacity)
			m_prime_lt_capacity = m_capacity - 1;
	} else
		m_prime_lt_capacity = 0;
}

} /* end of namespace milliways */

#endif /* MILLIWAYS_HASHTABLE_IMPL_H */
