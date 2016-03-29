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
#include <deque>
#include <functional>

#include <stdint.h>
#include <assert.h>

#include "ordered_map.h"

namespace milliways {

template <size_t SIZE, typename Key, typename T>
class LRUCache
{
public:
	typedef Key key_type;
	typedef T mapped_type;
	typedef std::pair<Key, T> value_type;
	typedef ordered_map<key_type, mapped_type> ordered_map_type;
	typedef typename ordered_map<key_type, mapped_type>::size_type size_type;

	static const size_type Size = SIZE;

	typedef enum { op_get, op_set, op_sub } op_type;

	LRUCache() {}
	virtual ~LRUCache() { /* call evict_all() in final destructor */ evict_all(); }

	virtual bool on_miss(op_type op, const key_type& key, mapped_type& value);
	virtual bool on_set(const key_type& key, const mapped_type& value);
	virtual bool on_delete(const key_type& key);
	virtual bool on_eviction(const key_type& key, mapped_type& value);

	bool empty() const { return m_omap.empty(); }
	size_type size() const { return m_omap.size(); }
	size_type max_size() const { return Size; }

	void clear() { m_omap.clear(); }

	bool has(key_type& key) const { return m_omap.has(key); }
	bool get(mapped_type& dst, key_type& key);
	bool set(key_type& key, mapped_type& value);
	bool del(key_type& key);

	size_type count(const key_type& key) const { return m_omap.count(key); }
	mapped_type& operator[](const key_type& key);

	void evict(bool force = false);     // evict the LRU item
	void evict_all();

	value_type pop();                   // pop LRU item and return it

private:
	LRUCache(const LRUCache<SIZE, Key, T>& other);
	LRUCache& operator= (const LRUCache<SIZE, Key, T>& rhs);

	ordered_map<key_type, mapped_type> m_omap;
};

} /* end of namespace milliways */

#include "LRUCache.impl.hpp"

#endif /* MILLIWAYS_LRUCACHE_H */
