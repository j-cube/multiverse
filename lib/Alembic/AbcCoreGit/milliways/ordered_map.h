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
#define MILLIWAYS_ORDERED_MAP_H

#include <map>
#include <unordered_map>
#include <deque>

#include <stdint.h>
#include <assert.h>

namespace milliways {

template <typename Key, typename T>
class ordered_map
{
public:
	typedef Key key_type;
	typedef T mapped_type;
	typedef std::pair<Key, T> value_type;
	typedef std::unordered_map<key_type, mapped_type> key_value_map_t;
	typedef std::deque<key_type> key_vector_t;
	typedef typename key_value_map_t::size_type size_type;

	class iterator;
	class const_iterator;

	ordered_map() {}
	ordered_map(const ordered_map& other) :
		m_map(other.m_map), m_keys(other.m_keys) {}
	ordered_map& operator= (const ordered_map& other)
		{ m_map = other.m_map; m_keys = other.m_keys; }

	bool empty() const { return m_map.empty(); }
	size_type size() const { return m_map.size(); }
	size_type max_size() const { return m_map.max_size(); }

	void clear() { m_map.clear(); m_keys.clear(); }

	bool has(const key_type& key) const { return m_map.count(key); }
	bool get(mapped_type& dst, const key_type& key);
	void set(const key_type& key, const mapped_type& value);

	size_type count(const key_type& key) const { return m_map.count(key); }
	mapped_type& operator[] (const key_type& key);

	value_type pop();            // remove last (LIFO)
	value_type pop_front();      // remove first (FIFO)

	value_type pop(const key_type& key);

	iterator begin() { return iterator(this, m_keys.begin()); }
	iterator end() { return iterator(this, m_keys.end()); }

	iterator find(const key_type& key);

	class iterator
	{
	public:
		typedef iterator self_type;
		typedef std::pair<const Key, T> value_type;
		typedef std::pair<const Key, T>& reference;
		typedef std::pair<const Key, T>* pointer;
		typedef std::forward_iterator_tag iterator_category;
		typedef int difference_type;

		iterator() { }
		iterator(ordered_map* parent, typename key_vector_t::iterator it) : m_parent(parent), m_key_it(it) { }
		iterator(const iterator& other) : m_parent(other.m_parent), m_key_it(other.m_key_it), m_map_it(other.m_map_it) { }
		iterator& operator= (const iterator& other) { m_parent = other.m_parent; m_key_it = other.m_key_it; m_map_it = other.m_map_it; return *this; }

		self_type operator++() { self_type i = *this; m_key_it++; return i; }
		self_type operator++(int junk) { m_key_it++; return *this; }
		reference operator*() { m_map_it = m_parent->m_map.find((*m_key_it)); return (*m_map_it); }
		pointer operator->() { m_map_it = m_parent->m_map.find((*m_key_it)); return &(*m_map_it); }
		bool operator==(const self_type& rhs) { return (m_parent == rhs.m_parent) && (m_key_it == rhs.m_key_it); }
		bool operator!=(const self_type& rhs) { return (m_parent != rhs.m_parent) || (m_key_it != rhs.m_key_it); }

		operator bool() const { return m_key_it != m_parent->m_keys.end(); }

	private:
		ordered_map* m_parent;
		typename key_vector_t::iterator m_key_it;
		typename key_value_map_t::iterator m_map_it;
		friend class const_iterator;
	};

	class const_iterator
	{
	public:
		typedef const_iterator self_type;
		typedef std::pair<const Key, T> value_type;
		typedef std::pair<const Key, T>& reference;
		typedef const std::pair<const Key, T>& const_reference;
		typedef std::pair<const Key, T>* pointer;
		typedef const std::pair<const Key, T>* const_pointer;
		typedef std::forward_iterator_tag iterator_category;
		typedef int difference_type;

		const_iterator() { }
		const_iterator(ordered_map* parent, typename key_vector_t::iterator it) : m_parent(parent), m_key_it(it) { }
		const_iterator(const const_iterator& other) : m_parent(other.m_parent), m_key_it(other.m_key_it), m_map_it(other.m_map_it) { }
		const_iterator(const iterator& other) : m_parent(other.m_parent), m_key_it(other.m_key_it), m_map_it(other.m_map_it) { }
		const_iterator& operator= (const const_iterator& other) { m_parent = other.m_parent; m_key_it = other.m_key_it; m_map_it = other.m_map_it; return *this; }

		self_type operator++() { self_type i = *this; m_key_it++; return i; }
		self_type operator++(int junk) { m_key_it++; return *this; }
		const_reference operator*() { m_map_it = m_parent->m_map.find((*m_key_it)); return (*m_map_it); }
		const_pointer operator->() { m_map_it = m_parent->m_map.find((*m_key_it)); return &(*m_map_it); }
		bool operator==(const self_type& rhs) { return (m_parent == rhs.m_parent) && (m_key_it == rhs.m_key_it); }
		bool operator!=(const self_type& rhs) { return (m_parent != rhs.m_parent) || (m_key_it != rhs.m_key_it); }

		operator bool() const { return m_key_it != m_parent->m_keys.end(); }

	private:
		const ordered_map* m_parent;
		typename key_vector_t::const_iterator m_key_it;
		typename key_value_map_t::const_iterator m_map_it;
		friend class iterator;
	};

private:
	key_value_map_t m_map;
	key_vector_t m_keys;
};

} /* end of namespace milliways */

#include "ordered_map.impl.hpp"

#endif /* MILLIWAYS_ORDERED_MAP_H */
