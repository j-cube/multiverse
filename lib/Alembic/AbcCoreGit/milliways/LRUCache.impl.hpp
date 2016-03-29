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
#include "LRUCache.h"
#endif

#ifndef MILLIWAYS_LRUCACHE_IMPL_H
//#define MILLIWAYS_LRUCACHE_IMPL_H

namespace milliways {

template <size_t SIZE, typename Key, typename T>
bool LRUCache<SIZE, Key, T>::on_miss(op_type op, const key_type& key, mapped_type& value)
{
	return true;
}

template <size_t SIZE, typename Key, typename T>
bool LRUCache<SIZE, Key, T>::on_set(const key_type& key, const mapped_type& value)
{
	return true;
}

template <size_t SIZE, typename Key, typename T>
bool LRUCache<SIZE, Key, T>::on_delete(const key_type& key)
{
	return true;
}

template <size_t SIZE, typename Key, typename T>
bool LRUCache<SIZE, Key, T>::on_eviction(const key_type& key, mapped_type& value)
{
	return true;
}

template <size_t SIZE, typename Key, typename T>
bool LRUCache<SIZE, Key, T>::get(mapped_type& dst, key_type& key)
{
	typename ordered_map<key_type, mapped_type>::const_iterator it = m_omap.find(key);

	if (it != m_omap.end())
	{
		// fetch from its place...
		typename ordered_map<key_type, mapped_type>::value_type item = m_omap.pop(key);

		// ...and re-insert at top
		m_omap[key] = item.second;

		dst = item.second;
		return true;
	} else
	{
		// miss - use overridable function

		if (m_omap.size() >= Size)
			evict();
		assert(m_omap.size() < Size);

		mapped_type value;
		bool success = on_miss(op_get, key, value);
		if (success)
		{
			m_omap[key] = value;
			return success;
		}
	}

	return false;
}

template <size_t SIZE, typename Key, typename T>
bool LRUCache<SIZE, Key, T>::set(key_type& key, mapped_type& value)
{
	typename ordered_map<key_type, mapped_type>::const_iterator it = m_omap.find(key);

	if (it != m_omap.end())
	{
		// remove existing from its place...
		/* typename ordered_map<key_type, mapped_type>::value_type item = */ m_omap.pop(key);
	} else
	{
		if (m_omap.size() >= Size)
			evict();
		assert(m_omap.size() < Size);

		/* bool success = */ on_miss(op_set, key, value);
	}

	m_omap[key] = value;
	on_set(key, value);

	return true;
}

template <size_t SIZE, typename Key, typename T>
bool LRUCache<SIZE, Key, T>::del(key_type& key)
{
	typename ordered_map<key_type, mapped_type>::const_iterator it = m_omap.find(key);

	if (it != m_omap.end())
	{
		// remove existing from its place...
		/* typename ordered_map<key_type, mapped_type>::value_type item = */ m_omap.pop(key);
		on_delete(key);
		return true;
	}

	return false;
}

template <size_t SIZE, typename Key, typename T>
T& LRUCache<SIZE, Key, T>::operator[](const key_type& key)
{
	typename ordered_map<key_type, mapped_type>::const_iterator it = m_omap.find(key);

	if (it != m_omap.end())
	{
		// fetch from its place...
		typename ordered_map<key_type, mapped_type>::value_type item = m_omap.pop(key);

		// ...and re-insert at top
		m_omap[key] = item.second;

		return m_omap[key];
	} else
	{
		// miss - use overridable function

		if (m_omap.size() >= Size)
			evict();
		assert(m_omap.size() < Size);

		mapped_type value;

		/* bool success = */ on_miss(op_sub, key, value);

		m_omap[key] = value;
		return m_omap[key];
	}
}

template <size_t SIZE, typename Key, typename T>
void LRUCache<SIZE, Key, T>::evict(bool force)
{
	if (m_omap.size() <= 0)
		return;

	assert(m_omap.size() > 0);

	if ((m_omap.size() >= Size) || force)
	{
		// remove oldest (FIFO)
		typename ordered_map<key_type, mapped_type>::value_type item = m_omap.pop_front();

		on_eviction(item.first, item.second);
	}
}

template <size_t SIZE, typename Key, typename T>
void LRUCache<SIZE, Key, T>::evict_all()
{
	while (m_omap.size() > 0)
		evict(/* force */ true);
}

template <size_t SIZE, typename Key, typename T>
typename std::pair<Key, T> LRUCache<SIZE, Key, T>::pop()
{
	if (m_omap.size() <= 0)
		return std::pair<Key, T>();

	assert(m_omap.size() > 0);

	// remove oldest (FIFO)
	typename ordered_map<key_type, mapped_type>::value_type item = m_omap.pop_front();

	on_eviction(item.first, item.second);

	return std::pair<Key, T>(item.first, item.second);
}

} /* end of namespace milliways */

#endif /* MILLIWAYS_LRUCACHE_IMPL_H */
