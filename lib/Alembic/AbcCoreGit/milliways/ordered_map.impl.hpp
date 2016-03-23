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
	typename key_value_map_t::const_iterator it = m_map.find(key);
	if (it != m_map.end())
	{
		dst = it->second;
		return true;
	}

	return false;
}

template <typename Key, typename T>
void ordered_map<Key, T>::set(const Key& key, const T & value)
{
	typename key_value_map_t::const_iterator it = m_map.find(key);
	if (it == m_map.end())
		m_keys.push_back(key);
	m_map[key] = value;
}

template <typename Key, typename T>
T & ordered_map<Key, T>::operator[](const Key& key)
{
	typename key_value_map_t::const_iterator it = m_map.find(key);
	if (it == m_map.end())
		m_keys.push_back(key);
	return m_map[key];
}

template <typename Key, typename T>
std::pair<Key, T> ordered_map<Key, T>::pop()
{
	Key key = m_keys.back();
	m_keys.pop_back();

	typename key_value_map_t::iterator it = m_map.find(key);
	T value = it->second;

	m_map.erase(it);

	return std::pair<Key, T>(key, value);
}

template <typename Key, typename T>
std::pair<Key, T> ordered_map<Key, T>::pop_front()
{
	Key key = m_keys.front();
	m_keys.pop_front();

	typename key_value_map_t::iterator it = m_map.find(key);
	T value = it->second;

	m_map.erase(it);

	return std::pair<Key, T>(key, value);
}

template <typename Key, typename T>
std::pair<Key, T> ordered_map<Key, T>::pop(const key_type& key)
{
	typename key_vector_t::iterator k_it = std::find(m_keys.begin(), m_keys.end(), key);

	if (k_it != m_keys.end())
	{
		m_keys.erase(k_it);

		typename key_value_map_t::iterator it = m_map.find(key);
		T value = it->second;

		m_map.erase(it);

		return std::pair<Key, T>(key, value);
	}

	return std::pair<Key, T>();
}

template <typename Key, typename T>
typename ordered_map<Key, T>::iterator ordered_map<Key, T>::find(const key_type& key)
{
	typename key_vector_t::iterator k_it = std::find(m_keys.begin(), m_keys.end(), key);

	return iterator(this, k_it);
}

} /* end of namespace milliways */

#endif /* MILLIWAYS_ORDERED_MAP_IMPL_H */
