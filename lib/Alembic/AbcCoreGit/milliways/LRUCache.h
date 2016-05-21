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

#ifndef MILLIWAYS_LRUCACHE_H
#define MILLIWAYS_LRUCACHE_H

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <unordered_map>
#include <deque>
#include <functional>

#include <stdint.h>
#include <assert.h>

#include "config.h"
#include "hashtable.h"

namespace milliways {

static const int LRUCACHE_L1_CACHE_SIZE = 12;

template <size_t SIZE, typename Key, typename T>
class LRUCache
{
public:
	typedef Key key_type;
	typedef T mapped_type;
	typedef std::pair<Key, T> value_type;
	typedef size_t size_type;

	typedef long age_t;
	typedef std::unordered_map<key_type, age_t> key_to_age_t;
	typedef std::unordered_map<key_type, mapped_type> map_t;
	typedef typename map_t::iterator map_iter_t;

	static const size_type Size = SIZE;
	static const int L1_SIZE = LRUCACHE_L1_CACHE_SIZE;

	typedef enum { op_get, op_set, op_sub } op_type;

	// LRUCache();
	LRUCache(const key_type& invalid);
	virtual ~LRUCache() { /* call evict_all() in final destructor */ evict_all(); }

	virtual bool on_miss(op_type op, const key_type& key, mapped_type& value);
	virtual bool on_set(const key_type& key, const mapped_type& value);
	virtual bool on_delete(const key_type& key);
	virtual bool on_eviction(const key_type& key, mapped_type& value);

	bool empty() const { return m_key2age.empty(); }
	size_type size() const { return m_key2age.size(); }
	size_type max_size() const { return Size; }

	void clear() { clear_l1(); }
	void clear_l1();

	bool has(const key_type& key) const;
	bool get(mapped_type& dst, const key_type& key);
	bool set(const key_type& key, mapped_type& value);
	bool del(key_type& key);

	size_type count(const key_type& key) const { return has(key) ? 1 : 0; }
	mapped_type& operator[](const key_type& key);

	bool evict(bool force = false);     // evict the LRU item
	void evict_all();

	value_type pop();                   // pop LRU item and return it

	key_type invalid_key() const { return m_invalid_key; }
	void invalid_key(const key_type& value) { m_invalid_key = value; }
	void invalidate_key(key_type& key) const { key = m_invalid_key; }

	void keys(std::vector<key_type>& dst) {
		dst.clear();
		typename std::map<age_t, key_type>::const_iterator it;
		for (it = m_age2key.begin(); it != m_age2key.end(); ++it)
			dst.push_back(it->second);
	}

	void values(std::vector<value_type>& dst) {
		dst.clear();
		typename std::map<age_t, key_type>::const_iterator it;
		for (it = m_age2key.begin(); it != m_age2key.end(); ++it)
		{
			/* age_t age = it->first; */
			key_type key = it->second;
			value_type& value = m_cache[key];
			dst.push_back(value_type(key, value));
		}
	}

private:
	LRUCache();
	LRUCache(const LRUCache<SIZE, Key, T>& other);
	LRUCache& operator= (const LRUCache<SIZE, Key, T>& rhs);

	bool oldest(age_t& age) {
		typename std::map<age_t, key_type>::const_iterator it = m_age2key.begin();
		if (it != m_age2key.end()) {
			age = it->first;
			return true;
		}
		return false;
	}

	mutable key_type m_l1_key[L1_SIZE];
	mutable map_iter_t m_l1_mapped[L1_SIZE];
	mutable int m_l1_last;

	mutable age_t                     m_current_age;
	mutable key_to_age_t              m_key2age;
	mutable std::map<age_t, key_type> m_age2key;
	mutable map_t                     m_cache;

	key_type m_invalid_key;
};

} /* end of namespace milliways */

#include "LRUCache.impl.hpp"

#endif /* MILLIWAYS_LRUCACHE_H */
