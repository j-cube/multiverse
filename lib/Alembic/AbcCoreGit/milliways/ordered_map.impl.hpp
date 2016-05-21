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

#ifndef MILLIWAYS_ORDERED_MAP_H
#include "ordered_map.h"
#endif

#ifndef MILLIWAYS_ORDERED_MAP_IMPL_H
#define MILLIWAYS_ORDERED_MAP_IMPL_H

#include <map>
#include <deque>
#include <algorithm>

#include <stdint.h>
#include <assert.h>

namespace milliways {

template <typename Key, typename T>
bool ordered_map<Key, T>::get(T & dst, const Key& key)
{
	return m_map.get(key, dst);
}

template <typename Key, typename T>
void ordered_map<Key, T>::set(const Key& key, const T & value)
{
	order_type pos = 0;
	if (! m_key2pos.get(key, pos)) {
		pos = m_tail++;
		m_key2pos.set(key, pos);
		m_pos2key.set(pos, key);
		// std::cerr << "OM::set[" << key << "] := " << value << "   new pos := " << pos << "\n";
	}
	// } else
	// 	std::cerr << "OM::set[" << key << "] := " << value << "   old pos: " << pos << "\n";
	m_map.set(key, value);
}

template <typename Key, typename T>
T & ordered_map<Key, T>::operator[](const Key& key)
{
	order_type pos = 0;
	if (! m_key2pos.get(key, pos)) {
		pos = m_tail++;
		m_key2pos.set(key, pos);
		m_pos2key.set(pos, key);
		// std::cerr << "OM::[" << key << "]   new pos := " << pos << "\n";
	}
	 // else
		// std::cerr << "OM::[" << key << "]   old pos: " << pos << "\n";
	return m_map[key];
}

template <typename Key, typename T>
std::pair<Key, T> ordered_map<Key, T>::pop()
{
	if ((m_tail > m_head) && (m_pos2key.size() > 0))
	{
		order_type pos = --m_tail;
		key_type key = m_pos2key[pos];
		T value = m_map[key];
		m_pos2key.erase(pos);
		m_key2pos.erase(key);
		m_map.erase(key);
		// std::cerr << "OM::pop()  key:" << key << " pos:" << pos << "\n";
		return std::pair<Key, T>(key, value);
	}

	assert(false);
	return std::pair<Key, T>();
}

template <typename Key, typename T>
std::pair<Key, T> ordered_map<Key, T>::pop_front()
{
	// std::cerr << "pop_front m_head:" << m_head << "  m_tail:" << m_tail << "\n";
	/* size_t size_ = m_pos2key.size(); */
	order_type pos;
	key_type key;
	bool has_pos = false;

	while (m_head < m_tail)
	{
		pos = m_head++;
		if (m_pos2key.get(pos, key))
		{
			has_pos = true;
			break;
		}
	}
	if (! has_pos)
	{
		assert(false);
		return std::pair<Key, T>();
	}

	assert(has_pos);

	T value = m_map[key];
	// std::cerr << "key:" << key << "  value:" << value << " pos:" << pos << "  now m_head(++):" << m_head << "\n";
	bool ok = m_pos2key.erase(pos);
	// if (! ok)
	// 	std::cerr << "m_pos2key not found pos:" << pos << "\n";
	assert(ok);
	ok = m_key2pos.erase(key);
	// if (! ok)
	// 	std::cerr << "m_key2pos not found key:" << key << "\n";
	assert(ok);
	ok = m_map.erase(key);
	// if (! ok)
	// 	std::cerr << "m_map not found key:" << key << "\n";
	assert(ok);
	return std::pair<Key, T>(key, value);
}

template <typename Key, typename T>
std::pair<Key, T> ordered_map<Key, T>::pop(const key_type& key)
{
	order_type pos = 0;
	if (m_key2pos.get(key, pos))
	{
		if (pos == m_head)
			m_head++;
		else if (pos == m_tail)
			m_tail--;
		T value = m_map[key];
		m_key2pos.erase(key);
		m_pos2key.erase(pos);
		m_map.erase(key);
		// std::cerr << "OM::pop_key(" << key << ")  key:" << key << " pos:" << pos << "\n";
		return std::pair<Key, T>(key, value);
	} else
		// std::cerr << "OM::pop_key(" << key << ")  key:" << key << " pos:--\n";

	return std::pair<Key, T>();
}

template <typename Key, typename T>
typename ordered_map<Key, T>::iterator ordered_map<Key, T>::find(const key_type& key)
{
	order_type pos = 0;
	if (m_key2pos.get(key, pos)) {
		return iterator(this, pos);
	}
	return iterator(this, static_cast<order_type>(-1));
}

template <typename Key, typename T>
typename ordered_map<Key, T>::const_iterator ordered_map<Key, T>::find(const key_type& key) const
{
	order_type pos = 0;
	if (m_key2pos.get(key, pos)) {
		return const_iterator(this, pos);
	}
	return const_iterator(this, static_cast<order_type>(-1));
}

} /* end of namespace milliways */

#endif /* MILLIWAYS_ORDERED_MAP_IMPL_H */
