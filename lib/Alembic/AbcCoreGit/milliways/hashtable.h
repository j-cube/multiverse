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
#define MILLIWAYS_HASHTABLE_H

#include <iostream>
#include <fstream>
#include <string>
#include <functional>

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>

namespace milliways {

template <typename Key>
struct Hasher {
	typedef Key key_type;
	static size_t hash(const key_type& key);
	static size_t hash2(const key_type& key);
};

template < typename Key, typename T, class KeyCompare = std::equal_to<Key> >
class hashtable
{
public:
	typedef Key key_type;
	typedef T mapped_type;
	typedef size_t size_type;
	typedef size_type hash_type;

	class iterator;
	class const_iterator;

	static const size_type DEFAULT_CAPACITY = 256;
	static const size_type MAX_LOAD_FACTOR = 30;
	static const size_type EXPANSION_FACTOR = 2;

private:
	class bucket
	{
	public:
		enum state_t { FREE, USED, DELETED };

		bucket() : m_key(), m_value(), m_state(FREE) {}
		bucket(const key_type& key_, const mapped_type& value_, state_t state_ = FREE) : m_key(key_), m_value(value_), m_state(state_) {}
		bucket(const bucket& other) : m_key(other.m_key), m_value(other.m_value), m_state(other.m_state) {}
		bucket& operator= (const bucket& other) { if (this == &other) return *this; m_key = other.m_key; m_value = other.m_value; m_state = other.m_state; return *this; }
		~bucket() {}

		const key_type& key() const { return m_key; }
		key_type& key() { return m_key; }
		bucket& key(const key_type& key_) { m_key = key_; return *this; }

		const mapped_type& value() const { return m_value; }
		mapped_type& value() { return m_value; }
		bucket& value(const mapped_type& value_) { m_value = value_; return *this; }

		state_t state() const { return m_state; }
		bucket& state(state_t state_) { m_state = state_; return *this; }

		bool isFree() const { return (state() == FREE); }
		bool notFree() const { return (state() != FREE); }
		bool isUsed() const { return (state() == USED); }
		bool notUsed() const { return (state() != USED); }
		bool isDeleted() const { return (state() == DELETED); }

	private:
		key_type m_key;
		mapped_type m_value;
		state_t m_state;
	};

public:
	hashtable();
	hashtable(size_type for_size);
	~hashtable();

	bool empty() const { return m_size == 0; }
	size_type size() const { return m_size; }
	size_type max_size() const { return capacity(); }

	size_type capacity() const { return m_capacity; }

	bool has(const key_type& key) const;
	bool get(const key_type& key, mapped_type& value) const;
	hashtable& set(const key_type& key, const mapped_type& value);
	bool erase(const key_type& key);

	size_type count(const key_type& key) const { return has(key) ? 1 : 0; }
	mapped_type& operator[] (const key_type& key) { 
		bucket* found = find_bucket(key);
		if (found)
			return found->value();
		bucket* where = set_(key, T());
		return where->value();
	}
	// const mapped_type& operator[] (const key_type& key) const { 
	// 	const bucket* found = find_bucket(key);
	// 	if (found)
	// 		return found->value();
	// 	bucket* where = set_(key, T());
	// 	return where->value();
	// }

	void clear();

	iterator begin() { return iterator(this, 0); }
	iterator end() { return iterator(this, -1); }

	const_iterator begin() const { return const_iterator(this, 0); }
	const_iterator end() const { return const_iterator(this, -1); }
	const_iterator cbegin() const { return const_iterator(this, 0); }
	const_iterator cend() const { return const_iterator(this, -1); }

	iterator find(const key_type& key) {
		ssize_t pos = -1;
		find_bucket(key, pos);
		return iterator(this, static_cast<int64_t>(pos));
	}
	const_iterator find(const key_type& key) const {
		ssize_t pos = -1;
		find_bucket(key, pos);
		return const_iterator(this, static_cast<int64_t>(pos));
	}
	bool erase(iterator& pos) {
		if (! pos) return false;
		bucket* bk = pos.m_bucket;
		if (bk) {
			bk->key(Key());
			bk->value(T());
			bk->state(bucket::DELETED);
			m_size--;
			pos.invalidate();
			return true;
		}
		return false;
	}

	class iterator
	{
	public:
		typedef iterator self_type;
		typedef std::pair<Key, T> value_type;
		typedef std::pair<Key, T>& reference;
		typedef std::pair<Key, T>* pointer;
		typedef std::forward_iterator_tag iterator_category;
		typedef int difference_type;

		iterator() : m_parent(NULL), m_pos(-1), m_buckets(NULL), m_bucket(NULL), m_has_value(false) { }
		iterator(hashtable* parent, int64_t bucket_pos) : m_parent(parent), m_pos(bucket_pos), m_buckets(parent->m_buckets), m_bucket(NULL), m_has_value(false) { if (m_pos >= 0) { m_pos--; advance(); } }
		iterator(const iterator& other) : m_parent(other.m_parent), m_pos(other.m_pos), m_buckets(other.m_buckets), m_bucket(other.m_bucket), m_has_value(false) { }
		iterator(const const_iterator& other) : m_parent(other.m_parent), m_pos(other.m_pos), m_buckets(other.m_buckets), m_bucket(other.m_bucket), m_has_value(false) { }
		iterator& operator= (const iterator& other) { m_parent = other.m_parent; m_pos = other.m_pos; m_buckets = other.m_buckets; m_bucket = other.m_bucket; m_has_value = false; return *this; }

		self_type& operator++() { advance(); return *this; }										/* prefix  */
		self_type operator++(int) { self_type i = *this; advance(); return i; }						/* postfix */
		reference operator*() { update_value(); return m_value; }
		pointer operator->() { update_value(); return &m_value; }
		bool operator==(const self_type& rhs) {
			if (m_parent != rhs.m_parent) return false;
			if ((! valid()) && (! rhs.valid())) return true;
			return (m_pos == rhs.m_pos) && (m_bucket == rhs.m_bucket);
		}
		bool operator!=(const self_type& rhs) { return ! (*this == rhs); }

		operator bool() const { return (m_parent && (m_buckets == m_parent->m_buckets) && (m_pos >= 0) && m_bucket && m_bucket->isUsed()); }

		const key_type& key() const { assert(m_bucket); return m_bucket->key(); }
		key_type& key() { assert(m_bucket); return m_bucket->key(); }
		iterator& key(const key_type& key_) { assert(m_bucket); m_bucket->key(key_); return *this; }

		const mapped_type& value() const { assert(m_bucket); return m_bucket->value(); }
		mapped_type& value() { assert(m_bucket); return m_bucket->value(); }
		iterator& value(const mapped_type& value_) { assert(m_bucket); m_bucket->value(value_); return *this; }

		bool valid() const { return m_parent && (m_buckets == m_parent->m_buckets) && m_bucket && m_bucket->isUsed() && (m_pos >= 0); }
		void invalidate() { m_pos = -1; m_buckets = NULL; m_bucket = NULL; m_has_value = false; }

	private:
		hashtable* m_parent;
		int64_t m_pos;
		bucket* m_buckets;
		bucket* m_bucket;
		value_type m_value;
		bool m_has_value;
		friend class const_iterator;
		friend class hashtable;

		void advance() {
			if ((! m_parent) || (m_parent->m_buckets != m_buckets)) {
				invalidate();
				return;
			}
			m_has_value = false;
			do {
				if (++m_pos >= static_cast<int64_t>(m_parent->m_capacity)) {
					m_bucket = NULL;
					m_pos = -1;
					return;
				}
				m_bucket = &m_parent->m_buckets[m_pos];
			} while (m_bucket && m_bucket->notUsed());
		}
		void update_value() { if (! m_bucket) return; if (! m_has_value) { m_value = value_type(m_bucket->key(), m_bucket->value()); m_has_value = true; } }
	};

	class const_iterator
	{
	public:
		typedef const_iterator self_type;
		typedef std::pair<Key, T> value_type;
		typedef std::pair<Key, T>& reference;
		typedef std::pair<Key, T>* pointer;
		typedef std::forward_iterator_tag iterator_category;
		typedef int difference_type;

		const_iterator() : m_parent(NULL), m_pos(-1), m_buckets(NULL), m_bucket(NULL), m_has_value(false) { }
		const_iterator(const hashtable* parent, int64_t bucket_pos) : m_parent(parent), m_pos(bucket_pos), m_buckets(parent->m_buckets), m_bucket(NULL), m_has_value(false) { if (m_pos >= 0) { m_pos--; advance(); } }
		const_iterator(const const_iterator& other) : m_parent(other.m_parent), m_pos(other.m_pos), m_buckets(other.m_buckets), m_bucket(other.m_bucket), m_has_value(false) { }
		const_iterator(const iterator& other) : m_parent(other.m_parent), m_pos(other.m_pos), m_buckets(other.m_buckets), m_bucket(other.m_bucket), m_has_value(false) { }
		const_iterator& operator= (const const_iterator& other) { m_parent = other.m_parent; m_pos = other.m_pos; m_buckets = other.m_buckets; m_bucket = other.m_bucket; m_has_value = false; return *this; }

		self_type& operator++() { advance(); return *this; }										/* prefix  */
		self_type operator++(int) { self_type i = *this; advance(); return i; }						/* postfix */
		reference operator*() { update_value(); return m_value; }
		pointer operator->() { update_value(); return &m_value; }
		bool operator==(const self_type& rhs) {
			if (m_parent != rhs.m_parent) return false;
			if ((! valid()) && (! rhs.valid())) return true;
			return (m_pos == rhs.m_pos) && (m_bucket == rhs.m_bucket);
		}
		bool operator!=(const self_type& rhs) { return ! (*this == rhs); }

		operator bool() const { return (m_parent && (m_buckets == m_parent->m_buckets) && (m_pos > 0) && m_bucket && m_bucket->isUsed()); }

		const key_type& key() const { assert(m_bucket); return m_bucket->key(); }
		const mapped_type& value() const { assert(m_bucket); return m_bucket->value(); }

		bool valid() const { return m_parent && (m_buckets == m_parent->m_buckets) && m_bucket && m_bucket->isUsed() && (m_pos >= 0); }
		void invalidate() { m_pos = -1; m_buckets = NULL; m_bucket = NULL; m_has_value = false; }

	private:
		const hashtable* m_parent;
		int64_t m_pos;
		bucket* m_buckets;
		const bucket* m_bucket;
		value_type m_value;
		bool m_has_value;
		friend class iterator;

		void advance() {
			if ((! m_parent) || (m_parent->m_buckets != m_buckets)) {
				invalidate();
				return;
			}
			m_has_value = false;
			do {
				if (++m_pos >= static_cast<int64_t>(m_parent->m_capacity)) {
					m_bucket = NULL;
					m_pos = -1;
					return;
				}
				m_bucket = &m_parent->m_buckets[m_pos];
			} while (m_bucket && m_bucket->notUsed());
		}
		void update_value() { if (! m_has_value) { m_value = value_type(m_bucket->key(), m_bucket->value()); m_has_value = true; } }
	};

protected:
	hash_type compute_hash2(const key_type& key) const;
	bool expand(size_t for_size = 0);
	size_type capacity_for(size_type size);

	bucket* set_(const key_type& key, const mapped_type& value);

	const bucket* find_bucket(const key_type& key, ssize_t& pos_) const;
	const bucket* find_bucket(const key_type& key) const;
	bucket* find_bucket(const key_type& key, ssize_t& pos_);
	bucket* find_bucket(const key_type& key);

public:
	friend inline std::ostream& operator<< ( std::ostream& out, const hashtable& value )
	{
		out << "<hashtable: [";
		size_t printed = 0;
		for (size_t i = 0; i < value.m_capacity; i++)
		{
			bucket& bk = value.m_buckets[i];
			if (bk.isUsed()) {
				out << "(" << bk.key() << ", " << bk.value() << ")";
				printed++;
				if (printed < value.size())
					out << ", ";
			}
		}
		out << "]>";
		return out;
	}

private:
	void compute_prime_lt_capacity();

	/* disable copy and assignment */
	hashtable(const hashtable&) {}
	hashtable& operator= (const hashtable&) {}

	bucket* m_buckets;
	size_type m_capacity;
	size_type m_size;
	size_type m_prime_lt_capacity;

	friend class iterator;
	friend class const_iterator;
};

} /* end of namespace milliways */

#include "hashtable.impl.hpp"

#endif /* MILLIWAYS_HASHTABLE_H */
