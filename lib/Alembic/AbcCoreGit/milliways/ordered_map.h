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
// #if defined(USE_STD_UNORDERED_MAP)
// #include <unordered_map>
// #elif defined(USE_TR1_UNORDERED_MAP)
// #include <tr1/unordered_map>
// #elif defined(USE_BOOST_UNORDERED_MAP)
// #include <boost/unordered_map.hpp>
// #endif
// #include <deque>

#include <stdint.h>
#include <assert.h>

#include "hashtable.h"

namespace milliways {

template <typename Key, typename T>
class ordered_map
{
public:
	typedef Key key_type;
	typedef T mapped_type;
	typedef std::pair<Key, T> value_type;
	typedef size_t order_type;
	typedef hashtable<key_type, mapped_type> key_value_map_t;
	typedef hashtable<key_type, order_type> key_to_order_t;
	typedef hashtable<order_type, key_type> order_to_key_t;
	typedef ITYPENAME key_value_map_t::size_type size_type;

	class iterator;
	class const_iterator;

	ordered_map() : m_head(0), m_tail(0) {}
	ordered_map(size_t for_size) : m_map(for_size), m_key2pos(for_size), m_pos2key(for_size), m_head(0), m_tail(0) {}
	ordered_map(const ordered_map& other) :
		m_map(other.m_map), m_key2pos(other.m_key2pos), m_pos2key(other.m_pos2key), m_head(0), m_tail(0) {}
	ordered_map& operator= (const ordered_map& other)
		{ m_map = other.m_map; m_key2pos = other.m_key2pos; m_pos2key = other.m_pos2key; m_head = other.m_head; m_tail = other.m_tail; }

	bool empty() const { return m_map.empty(); }
	size_type size() const { return m_map.size(); }
	size_type max_size() const { return m_map.max_size(); }

	void clear() { m_map.clear(); m_key2pos.clear(); }

	bool has(const key_type& key) const { return m_map.count(key); }
	bool get(mapped_type& dst, const key_type& key);
	void set(const key_type& key, const mapped_type& value);

	size_type count(const key_type& key) const { return m_map.count(key); }
	mapped_type& operator[] (const key_type& key);

	value_type pop();            // remove last (LIFO)
	value_type pop_front();      // remove first (FIFO)

	value_type pop(const key_type& key);

	iterator begin() { return iterator(this, m_head); }
	iterator end() { return iterator(this, static_cast<order_type>(-1)); }

	const_iterator begin() const { return const_iterator(this, m_head); }
	const_iterator end() const { return const_iterator(this, static_cast<order_type>(-1)); }

	iterator find(const key_type& key);
	const_iterator find(const key_type& key) const;

	class iterator
	{
	public:
		typedef iterator self_type;
		typedef std::pair<Key, T> value_type;
		typedef std::pair<Key, T>& reference;
		typedef std::pair<Key, T>* pointer;
		typedef std::forward_iterator_tag iterator_category;
		typedef int difference_type;

		iterator() : m_parent(NULL), m_pos(static_cast<order_type>(-1)), m_value(), m_has_value(false) { }
		iterator(ordered_map* parent, order_type pos) : m_parent(parent), m_pos(pos), m_value(), m_has_value(false) { }
		iterator(const iterator& other) : m_parent(other.m_parent), m_pos(other.m_pos), m_value(), m_has_value(false) { }
		iterator(const const_iterator& other) : m_parent(other.m_parent), m_pos(other.m_pos), m_value(), m_has_value(false) { }
		iterator& operator= (const iterator& other) { m_parent = other.m_parent; m_pos = other.m_pos; m_has_value = false; return *this; }

		self_type& operator++() { next_(); m_has_value = false; return *this; }									/* prefix  */
		self_type operator++(int) { self_type i = *this; next_(); m_has_value = false; return i; }				/* postfix */
		reference operator*() { update_value(); return m_value; }
		pointer operator->() { update_value(); return &m_value; }
		bool operator==(const self_type& rhs) {
			if (! m_parent)
				return m_parent == rhs.m_parent;
			if (at_end())
				return rhs.at_end();
			return (m_parent == rhs.m_parent) && (m_pos == rhs.m_pos);
		}
		bool operator!=(const self_type& rhs) { return !(*this == rhs); }

		operator bool() const { return (! at_end()); }
		bool at_end() const { return (! m_parent) || (m_pos >= m_parent->m_pos2key.size()) || (m_pos == static_cast<order_type>(-1)); }

	private:
		void next_() {
			m_has_value = false;
			if (! m_parent) return;
			size_type size = m_parent->m_pos2key.size();
			do {
				++m_pos;
			} while ((m_pos < size) && (! m_parent->m_pos2key.has(m_pos)));
		}
		void update_value() { if (m_has_value) return; assert(m_parent); key_type key; if (m_parent->m_pos2key.get(m_pos, key)) { T value; m_parent->m_map.get(key, value); m_value = value_type(key, value); m_has_value = true; }; }

		ordered_map* m_parent;
		order_type m_pos;
		value_type m_value;
		bool m_has_value;
		friend class const_iterator;
	};

	class const_iterator
	{
	public:
		typedef const_iterator self_type;
		typedef std::pair<Key, T> value_type;
		typedef std::pair<Key, T>& reference;
		typedef const std::pair<Key, T>& const_reference;
		typedef std::pair<Key, T>* pointer;
		typedef const std::pair<Key, T>* const_pointer;
		typedef std::forward_iterator_tag iterator_category;
		typedef int difference_type;

		const_iterator() : m_parent(NULL), m_pos(static_cast<order_type>(-1)), m_value(), m_has_value(false) { }
		const_iterator(ordered_map* parent, order_type pos) : m_parent(parent), m_pos(pos), m_value(), m_has_value(false) { }
		const_iterator(const const_iterator& other) : m_parent(other.m_parent), m_pos(other.m_pos), m_value(), m_has_value(false) { }
		const_iterator(const iterator& other) : m_parent(other.m_parent), m_pos(other.m_pos), m_value(), m_has_value(false) { }
		const_iterator& operator= (const const_iterator& other) { m_parent = other.m_parent; m_pos = other.m_pos; m_has_value = false; return *this; }

		self_type& operator++() { next_(); m_has_value = false; return *this; }									/* prefix  */
		self_type operator++(int) { self_type i = *this; next_(); m_has_value = false; return i; }				/* postfix */
		const_reference operator*() { update_value(); return m_value; }
		const_pointer operator->() { update_value(); return &m_value; }
		bool operator==(const self_type& rhs) {
			if (! m_parent)
				return m_parent == rhs.m_parent;
			if (at_end())
				return rhs.at_end();
			return (m_parent == rhs.m_parent) && (m_pos == rhs.m_pos);
		}
		bool operator!=(const self_type& rhs) { return !(*this == rhs); }

		operator bool() const { return (! at_end()); }
		bool at_end() const { return (! m_parent) || (m_pos >= m_parent->m_pos2key.size()) || (m_pos == static_cast<order_type>(-1)); }

	private:
		void next_() {
			m_has_value = false;
			if (! m_parent) return;
			size_type size = m_parent->m_pos2key.size();
			do {
				++m_pos;
			} while ((m_pos < size) && (! m_parent->m_pos2key.has(m_pos)));
		}
		void update_value() { if (m_has_value) return; assert(m_parent); key_type key; if (m_parent->m_pos2key.get(m_pos, key)) { T value; m_parent->m_map.get(key, value); m_value = value_type(key, value); m_has_value = true; }; }

		const ordered_map* m_parent;
		order_type m_pos;
		value_type m_value;
		bool m_has_value;
		friend class iterator;
	};

private:
	key_value_map_t m_map;
	key_to_order_t m_key2pos;
	order_to_key_t m_pos2key;
	order_type m_head;
	order_type m_tail;
};

} /* end of namespace milliways */

#include "ordered_map.impl.hpp"

#endif /* MILLIWAYS_ORDERED_MAP_H */
