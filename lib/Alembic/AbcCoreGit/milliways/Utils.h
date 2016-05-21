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

#ifndef MILLIWAYS_UTILS_H
#define MILLIWAYS_UTILS_H

#include <iostream>
#include <map>
#include <unordered_map>
#include <assert.h>

#include "config.h"

// #define USE_MILLIWAYS_SHPTR

#if !defined(USE_MILLIWAYS_SHPTR)
	#if defined(HAVE_STD_SHARED_PTR)
		#include <memory>
		#define USE_STD_SHARED_PTR
	#elif defined(HAVE_STD_TR1_SHARED_PTR)
		#include <tr1/memory>
		#define USE_TR1_SHARED_PTR
	#else
		#define USE_MILLIWAYS_SHPTR
	#endif
#endif

#if ! defined(NDEBUG)

#undef TRACE
#define TRACE( TEXT )                  \
do                                     \
{                                      \
    std::cerr << "[TRACE] " << TEXT << \
        std::endl;                     \
    std::cerr.flush();                 \
}                                      \
while( 0 )

#undef CTRACE
#define CTRACE( COND, TEXT )               \
do                                         \
{                                          \
	if ((COND)) {                          \
        std::cerr << "[TRACE] " << TEXT << \
            std::endl;                     \
        std::cerr.flush();                 \
    }                                      \
}                                          \
while( 0 )

#else

#undef TRACE
#define TRACE( TEXT ) do { } while(0)

#undef CTRACE
#define CTRACE( COND, TEXT ) do { } while(0)

#endif


namespace milliways {

void hexdump(void* ptr, int buflen);
std::ostream& hexdump(std::ostream& out, const void* ptr, int buflen);
std::string s_hexdump(const void* ptr, int buflen);

std::string hexify(const std::string& input);
std::string dehexify(const std::string& input);

/* ----------------------------------------------------------------- *
 *   shptr<T>                                                        *
 * ----------------------------------------------------------------- */

#if defined(USE_STD_SHARED_PTR)
    #define MW_SHPTR std::shared_ptr
    #define MILLIWAYS_SHPTR std::shared_ptr
    // #define shptr std::shared_ptr
 	// #define SHPTR std::shared_ptr
	// template <typename T>
	// class shptr : public std::shared_ptr<T>
	// {
	// public:
	// 	shptr() : std::shared_ptr<T>() {}
	// 	explicit shptr(T* naked) : std::shared_ptr<T>(naked) {}
	// 	shptr(const shptr<T>& other) : std::shared_ptr<T>(other) {}
	// 	/* ~shptr() {} */
	// 	shptr<T>& operator=(const shptr<T>& rhs) { std::shared_ptr<T>::operator=(rhs); return *this; }

	// 	bool operator==(const shptr<T>& rhs) { return ((this == &rhs) || (this->get() == rhs.get())); }
	// 	bool operator!=(const shptr<T>& rhs) { return !(*this == rhs); }
	// 	bool operator<(const shptr<T>& rhs) const { return std::shared_ptr<T>::operator<(rhs); }

	// 	operator void*() const 			{ return this->get(); }
	// 	operator bool() const 			{ return this->get() ? true : false; }

	// 	long count() const { return -1; }
	// };
#elif defined(USE_STD_TR1_SHARED_PTR)
    #define MW_SHPTR std::tr1::shared_ptr
    #define MILLIWAYS_SHPTR std::tr1::shared_ptr
    // #define shptr std::tr1::shared_ptr
 	// #define SHPTR std::tr1::shared_ptr
	// template <typename T>
	// class shptr : public std::tr1::shared_ptr<T>
	// {
	// public:
	// 	shptr() : std::tr1::shared_ptr<T>() {}
	// 	explicit shptr(T* naked) : std::tr1::shared_ptr<T>(naked) {}
	// 	shptr(const shptr<T>& other) : std::tr1::shared_ptr<T>(other) {}
	// 	~shptr() {}
	// 	shptr<T>& operator=(const shptr<T>& rhs) { std::tr1::shared_ptr<T>::operator=(rhs); return *this; }

	// 	bool operator==(const shptr<T>& rhs) { return ((this == &rhs) || (this->get() == rhs.get())); }
	// 	bool operator!=(const shptr<T>& rhs) { return !(*this == rhs); }
	// 	bool operator<(const shptr<T>& rhs) const { return std::tr1::shared_ptr<T>::operator<(rhs); }

	// 	operator void*() const 			{ return this->get(); }
	// 	operator bool() const 			{ return this->get() ? true : false; }

	// 	long count() const { return -1; }
	// };
#elif defined(USE_MILLIWAYS_SHPTR)

#define MW_SHPTR shptr
#define MILLIWAYS_SHPTR milliways::shptr

template <typename T>
class shptr;

class shptr_manager
{
	typedef std::map<void*, long> refcnt_map_t;

public:
	static shptr_manager& Instance() { return s_instance; }

protected:
	refcnt_map_t& refcnt_map() { return m_refcnt_map; }
	static refcnt_map_t& RefcntMap() { return Instance().refcnt_map(); }

	template <typename T>
	friend class shptr;

	static shptr_manager s_instance;
	refcnt_map_t m_refcnt_map;

private:
	shptr_manager() {}
	shptr_manager(const shptr_manager&) {}
	~shptr_manager() {
		refcnt_map_t::const_iterator it;
		for (it = m_refcnt_map.begin(); it != m_refcnt_map.end(); ++it) {
			if (it->first && it->second) {
				std::cerr << "WARNING: pointer " << (void*)it->first << " still has " << it->second << " references at the end of program lifetime." << std::endl;
			}
		}
	}
};

template <typename T>
class shptr
{
	typedef shptr_manager::refcnt_map_t refcnt_map_t;

public:
	shptr() : m_naked(NULL) {}
	explicit shptr(T* naked) : m_naked(NULL) { initialize_ptr(naked); }
	shptr(const shptr<T>& other) : m_naked(NULL) { assert(other.verify()); initialize_ptr(other.m_naked); }
	~shptr() { assert(verify()); finalize_ptr(); }
	shptr<T>& operator=(const shptr<T>& rhs) {
		if ((this == &rhs) || (*this == rhs)) return *this;
		assert(verify());
		finalize_ptr();
		assert(rhs.verify());
		initialize_ptr(rhs.m_naked);
		return *this;
	}

	bool operator==(const shptr<T>& rhs) {
		if (this == &rhs) return true;
		assert(rhs.verify());
		return (m_naked == rhs.m_naked) ? true : false;
	}
	bool operator!=(const shptr<T>& rhs) { return !(*this == rhs); }
	bool operator<(const shptr<T>& rhs) const {
		if (*this == rhs) return false;
		assert(rhs.verify());
		return (m_naked < rhs.m_naked);
	}

	T* get() const 					{ assert(verify()); return m_naked; }
	const T& operator*() const 		{ assert(verify()); return *m_naked; }
	T& operator*()				 	{ assert(verify()); return *m_naked; }
	const T* operator->() const 	{ assert(verify()); return m_naked; }
	T* operator->() 				{ assert(verify()); return m_naked; }

	operator void*() const 			{ assert(verify()); return m_naked; }
	operator bool() const 			{ assert(verify()); return m_naked ? true : false; }

	shptr<T>& reset() { assert(verify()); finalize_ptr(); m_naked = NULL; assert(! m_naked); return *this; }
	shptr<T>& reset(T* naked) {
		if (naked == m_naked) return *this;
		assert(verify());
		finalize_ptr();
		if (! naked) {
			m_naked = naked;
			return *this;
		}
		initialize_ptr(naked);
		return *this;
	}
	shptr<T>& swap(shptr<T>& other) {
		if ((this == &other) || (*this == other)) return *this;
		assert(verify());
		T* tmp = m_naked;
		m_naked = other.m_naked; other.m_naked = tmp;
		return *this;
	}

	long count() const {
		if (! m_naked) return 0;
		// WARNING: not thread safe
		// TODO: implement mutex locking for thread-safety
		refcnt_map_t& refcnt_map = shptr_manager::RefcntMap();
		if (! refcnt_map.count(m_naked))
			return 0;
		return refcnt_map[m_naked];
	}

#if defined(MILLIWAYS_CHECK_SHARED_POINTER_CORRUPTION)
	bool verify() const {
		if (! m_naked) return true;
		// WARNING: not thread safe
		// TODO: implement mutex locking for thread-safety
		refcnt_map_t& refcnt_map = shptr_manager::RefcntMap();
		if (! refcnt_map.count(m_naked))
			return false;
		// return (refcnt_map[m_naked] > 0) ? true : false;
		return true;
	}
#else
	bool verify() const { return true; }
#endif

protected:
	void finalize_ptr() {
		if (! m_naked) return;
		assert(m_naked);
		assert(verify());
		// WARNING: not thread safe
		// TODO: implement mutex locking for thread-safety
		refcnt_map_t& refcnt_map = shptr_manager::RefcntMap();
		if (! refcnt_map.count((void*)m_naked)) {
			throw std::runtime_error("ERROR: raw pointer missing from internal books!");
			assert(false);
		} else {
			refcnt_map[(void*)m_naked]--;
			if (refcnt_map[(void*)m_naked] == 0) {
				refcnt_map.erase((void*)m_naked);
				delete m_naked;
				m_naked = NULL;
			}
		}
		m_naked = NULL;
	}
	void initialize_ptr(T* naked) {
		assert(! m_naked);
		m_naked = naked;
		if (! naked)
			return;
		assert(m_naked);
		// WARNING: not thread safe
		// TODO: implement mutex locking for thread-safety
		refcnt_map_t& refcnt_map = shptr_manager::RefcntMap();
		if (! refcnt_map.count((void*)m_naked)) {
			refcnt_map[(void*)m_naked] = 1;
		} else {
			refcnt_map[(void*)m_naked]++;
		}
		assert(m_naked == naked);
	}

	T* m_naked;

private:
};

template <typename T>
inline std::ostream& operator<< ( std::ostream& out, const shptr<T>& value )
{
	if (value)
		out << "<shptr ptr:" << (void*)value << " (" << value.count() << " refs)>";
	else
		out << "<shptr null>";
	return out;
}

#endif /* defined(USE_MILLIWAYS_SHPTR) */

} /* end of namespace milliways */

#include "Utils.impl.hpp"

#endif /* MILLIWAYS_UTILS_H */
