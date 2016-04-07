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

// template <size_t SIZE, typename Key, typename T>
// LRUCache<SIZE, Key, T>::LRUCache() :
// 	m_l1_last(-1)
// {
// 	clear_l1();
// }

template <size_t SIZE, typename Key, typename T>
LRUCache<SIZE, Key, T>::LRUCache(const key_type& invalid) :
	m_l1_last(-1), m_invalid_key(invalid)
{
	clear_l1();
}

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
	for (int i = 0; i < L1_SIZE; i++)
		if (key == m_l1_key[i])
		{
			assert(m_l1_mapped[i]);
			dst = *m_l1_mapped[i];
			return true;
		}

	bool has_key = m_omap.has(key);
	// typename ordered_map<key_type, mapped_type>::const_iterator it = m_omap.find(key);

	if (has_key)
	{
		// fetch from its place...
		typename ordered_map<key_type, mapped_type>::value_type item = m_omap.pop(key);
		assert(item.first == key);

		// ...and re-insert at top
		m_omap[key] = item.second;

		mapped_type* mptr = &m_omap[key];

		m_l1_last = (m_l1_last + 1) % L1_SIZE;
		m_l1_key[m_l1_last] = key;
		m_l1_mapped[m_l1_last] = mptr;

		dst = *mptr;
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

			mapped_type* mptr = &m_omap[key];

			m_l1_last = (m_l1_last + 1) % L1_SIZE;
			m_l1_key[m_l1_last] = key;
			m_l1_mapped[m_l1_last] = mptr;

			return success;
		}
	}

	return false;
}

template <size_t SIZE, typename Key, typename T>
bool LRUCache<SIZE, Key, T>::set(key_type& key, mapped_type& value)
{
	for (int i = 0; i < L1_SIZE; i++)
		if (key == m_l1_key[i])
		{
			assert(m_l1_mapped[i]);
			(*m_l1_mapped[i]) = value;
			return true;
		}

	bool has_key = m_omap.has(key);
	// typename ordered_map<key_type, mapped_type>::const_iterator it = m_omap.find(key);

	if (has_key)
	{
		// remove existing from its place...
		typename ordered_map<key_type, mapped_type>::value_type item = m_omap.pop(key);
		assert(item.first == key);

		for (int i = 0; i < L1_SIZE; i++)
			if (item.first == m_l1_key[i])
			{
				invalidate_key(m_l1_key[i]);
				m_l1_mapped[i] = NULL;
				// break;
			}
	} else
	{
		if (m_omap.size() >= Size)
			evict();
		assert(m_omap.size() < Size);

		/* bool success = */ on_miss(op_set, key, value);
	}

	m_omap[key] = value;
	on_set(key, value);

	mapped_type* mptr = &m_omap[key];

	m_l1_last = (m_l1_last + 1) % L1_SIZE;
	m_l1_key[m_l1_last] = key;
	m_l1_mapped[m_l1_last] = mptr;

	return true;
}

template <size_t SIZE, typename Key, typename T>
bool LRUCache<SIZE, Key, T>::del(key_type& key)
{
	for (int i = 0; i < L1_SIZE; i++)
		if (key == m_l1_key[i])
		{
			invalidate_key(m_l1_key[i]);
			m_l1_mapped[i] = NULL;
			// break;
		}

	bool has_key = m_omap.has(key);
	// typename ordered_map<key_type, mapped_type>::const_iterator it = m_omap.find(key);

	if (has_key)
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
	for (int i = 0; i < L1_SIZE; i++)
		if (key == m_l1_key[i])
		{
			assert(m_l1_mapped[i]);
			return (*m_l1_mapped[i]);
		}

	bool has_key = m_omap.has(key);
	// typename ordered_map<key_type, mapped_type>::const_iterator it = m_omap.find(key);

	if (has_key)
	{
		// fetch from its place...
		typename ordered_map<key_type, mapped_type>::value_type item = m_omap.pop(key);
		assert(item.first == key);

		// ...and re-insert at top
		m_omap[key] = item.second;

		mapped_type* mptr = &m_omap[key];

		m_l1_last = (m_l1_last + 1) % L1_SIZE;
		m_l1_key[m_l1_last] = key;
		m_l1_mapped[m_l1_last] = mptr;

		return *mptr;
	} else
	{
		// miss - use overridable function

		if (m_omap.size() >= Size)
			evict();
		assert(m_omap.size() < Size);

		mapped_type value;

		/* bool success = */ on_miss(op_sub, key, value);

		m_omap[key] = value;

		mapped_type* mptr = &m_omap[key];

		m_l1_last = (m_l1_last + 1) % L1_SIZE;
		m_l1_key[m_l1_last] = key;
		m_l1_mapped[m_l1_last] = mptr;

		return *mptr;
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

		for (int i = 0; i < L1_SIZE; i++)
			if (item.first == m_l1_key[i])
			{
				invalidate_key(m_l1_key[i]);
				m_l1_mapped[i] = NULL;
				assert(! m_l1_mapped[i]);
				// break;
			}
	}
}

template <size_t SIZE, typename Key, typename T>
void LRUCache<SIZE, Key, T>::evict_all()
{
	clear_l1();

	while (m_omap.size() > 0)
		evict(/* force */ true);
}

template <size_t SIZE, typename Key, typename T>
void LRUCache<SIZE, Key, T>::clear_l1()
{
	for (int i = 0; i < L1_SIZE; i++)
	{
		invalidate_key(m_l1_key[i]);
		m_l1_mapped[i] = NULL;
	}
	m_l1_last = -1;
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

	for (int i = 0; i < L1_SIZE; i++)
		if (item.first == m_l1_key[i])
		{
			invalidate_key(m_l1_key[i]);
			m_l1_mapped[i] = NULL;
			// break;
		}

	return std::pair<Key, T>(item.first, item.second);
}

} /* end of namespace milliways */

#endif /* MILLIWAYS_LRUCACHE_IMPL_H */
