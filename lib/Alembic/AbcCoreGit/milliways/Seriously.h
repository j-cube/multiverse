/*
 * Seriously C++ serialization library.
 *
 * Copyright (C) 2014-2016 Marco Pantaleoni. All rights reserved.
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

#ifndef SERIOUSLY_H
#define SERIOUSLY_H

#include <string>

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "config.h"

#ifndef UNUSED
#define UNUSED(expr) do { (void)(expr); } while (0)
#endif

namespace seriously {

// Traits most general case not implemented

template <typename T>
struct Traits;
// {
// 	typedef T type;
// 	typedef T serialized_type;
// 	enum { Size = sizeof(type) };
// 	enum { SerializedSize = sizeof(serialized_type) };

// 	static ssize_t serialize(char*& dst, size_t& avail, const type& v);
// 	static ssize_t deserialize(const char*& src, size_t& avail, type& v);

// static size_t size(const type& value);
// static size_t maxsize(const type& value);

// static size_t serializedsize(const type& value);

// static bool valid(const type& value);

// static int compare(const type& a, cont type& b);
// };

// Traits specializations

template <>
struct Traits<bool>
{
	typedef bool type;
	typedef char serialized_type;
	enum { Size = sizeof(type) };
	enum { SerializedSize = sizeof(serialized_type) };

	static ssize_t serialize(char*& dst, size_t& avail, const type& v);
	static ssize_t deserialize(const char*& src, size_t& avail, type& v);

	static size_t size(const type& value)    { UNUSED(value); return sizeof(type); }
	static size_t maxsize(const type& value) { UNUSED(value); return sizeof(type); }
	static size_t serializedsize(const type& value) { UNUSED(value); return sizeof(serialized_type); /* pay attention here */ }

	static bool valid(const type& value)     { UNUSED(value); return true; }

	static int compare(const type& a, const type& b) { return static_cast<int>(a - b); }
};

template <>
struct Traits<int8_t>
{
	typedef int8_t type;
	typedef type serialized_type;
	enum { Size = sizeof(type) };
	enum { SerializedSize = sizeof(serialized_type) };

	static ssize_t serialize(char*& dst, size_t& avail, const type& v);
	static ssize_t deserialize(const char*& src, size_t& avail, type& v);

	static size_t size(const type& value)    { UNUSED(value); return Size; }
	static size_t maxsize(const type& value) { UNUSED(value); return Size; }
	static size_t serializedsize(const type& value) { UNUSED(value); return SerializedSize; }

	static bool valid(const type& value)     { UNUSED(value); return true; }

	static int compare(const type& a, const type& b) { return static_cast<int>(a - b); }
};

template <>
struct Traits<uint8_t>
{
	typedef uint8_t type;
	typedef type serialized_type;
	enum { Size = sizeof(type) };
	enum { SerializedSize = sizeof(serialized_type) };

	static ssize_t serialize(char*& dst, size_t& avail, const type& v);
	static ssize_t deserialize(const char*& src, size_t& avail, type& v);

	static size_t size(const type& value)    { UNUSED(value); return Size; }
	static size_t maxsize(const type& value) { UNUSED(value); return Size; }
	static size_t serializedsize(const type& value) { UNUSED(value); return Size; }

	static bool valid(const type& value)     { UNUSED(value); return true; }

	static int compare(const type& a, const type& b) { return static_cast<int>(a - b); }
};

template <>
struct Traits<int16_t>
{
	typedef int16_t type;
	typedef type serialized_type;
	enum { Size = sizeof(type) };
	enum { SerializedSize = sizeof(serialized_type) };

	static ssize_t serialize(char*& dst, size_t& avail, const type& v);
	static ssize_t deserialize(const char*& src, size_t& avail, type& v);

	static size_t size(const type& value)    { UNUSED(value); return Size; }
	static size_t maxsize(const type& value) { UNUSED(value); return Size; }
	static size_t serializedsize(const type& value) { UNUSED(value); return Size; }

	static bool valid(const type& value)     { UNUSED(value); return true; }

	static int compare(const type& a, const type& b) { return static_cast<int>(a - b); }
};

template <>
struct Traits<uint16_t>
{
	typedef uint16_t type;
	typedef type serialized_type;
	enum { Size = sizeof(type) };
	enum { SerializedSize = sizeof(serialized_type) };

	static ssize_t serialize(char*& dst, size_t& avail, const type& v);
	static ssize_t deserialize(const char*& src, size_t& avail, type& v);

	static size_t size(const type& value)    { UNUSED(value); return Size; }
	static size_t maxsize(const type& value) { UNUSED(value); return Size; }
	static size_t serializedsize(const type& value) { UNUSED(value); return Size; }

	static bool valid(const type& value)     { UNUSED(value); return true; }

	static int compare(const type& a, const type& b) { return static_cast<int>(a - b); }
};

template <>
struct Traits<int32_t>
{
	typedef int32_t type;
	typedef type serialized_type;
	enum { Size = sizeof(type) };
	enum { SerializedSize = sizeof(serialized_type) };

	static ssize_t serialize(char*& dst, size_t& avail, const type& v);
	static ssize_t deserialize(const char*& src, size_t& avail, type& v);

	static size_t size(const type& value)    { UNUSED(value); return Size; }
	static size_t maxsize(const type& value) { UNUSED(value); return Size; }
	static size_t serializedsize(const type& value) { UNUSED(value); return Size; }

	static bool valid(const type& value)     { UNUSED(value); return true; }

	static int compare(const type& a, const type& b) { return static_cast<int>(a - b); }
};

template <>
struct Traits<uint32_t>
{
	typedef uint32_t type;
	typedef type serialized_type;
	enum { Size = sizeof(type) };
	enum { SerializedSize = sizeof(serialized_type) };

	static ssize_t serialize(char*& dst, size_t& avail, const type& v);
	static ssize_t deserialize(const char*& src, size_t& avail, type& v);

	static size_t size(const type& value)    { UNUSED(value); return Size; }
	static size_t maxsize(const type& value) { UNUSED(value); return Size; }
	static size_t serializedsize(const type& value) { UNUSED(value); return Size; }

	static bool valid(const type& value)     { UNUSED(value); return true; }

	static int compare(const type& a, const type& b) { return static_cast<int>(a - b); }
};

template <>
struct Traits<int64_t>
{
	typedef int64_t type;
	typedef type serialized_type;
	enum { Size = sizeof(type) };
	enum { SerializedSize = sizeof(serialized_type) };

	static ssize_t serialize(char*& dst, size_t& avail, const type& v);
	static ssize_t deserialize(const char*& src, size_t& avail, type& v);

	static size_t size(const type& value)    { UNUSED(value); return Size; }
	static size_t maxsize(const type& value) { UNUSED(value); return Size; }
	static size_t serializedsize(const type& value) { UNUSED(value); return Size; }

	static bool valid(const type& value)     { UNUSED(value); return true; }

	static int compare(const type& a, const type& b) { return static_cast<int>(a - b); }
};

template <>
struct Traits<uint64_t>
{
	typedef uint64_t type;
	typedef type serialized_type;
	enum { Size = sizeof(type) };
	enum { SerializedSize = sizeof(serialized_type) };

	static ssize_t serialize(char*& dst, size_t& avail, const type& v);
	static ssize_t deserialize(const char*& src, size_t& avail, type& v);

	static size_t size(const type& value)    { UNUSED(value); return Size; }
	static size_t maxsize(const type& value) { UNUSED(value); return Size; }
	static size_t serializedsize(const type& value) { UNUSED(value); return Size; }

	static bool valid(const type& value)     { UNUSED(value); return true; }

	static int compare(const type& a, const type& b) { return static_cast<int>(a - b); }
};

#if ALLOWS_TEMPLATED_SIZE_T

template <>
struct Traits<size_t>
{
	typedef size_t type;
	typedef uint64_t serialized_type;
	enum { Size = sizeof(type) };
	enum { SerializedSize = sizeof(serialized_type) };

	static ssize_t serialize(char*& dst, size_t& avail, const type& v);
	static ssize_t deserialize(const char*& src, size_t& avail, type& v);

	static size_t size(const type& value)    { UNUSED(value); return Size; }
	static size_t maxsize(const type& value) { UNUSED(value); return Size; }
	static size_t serializedsize(const type& value) { UNUSED(value); return Size; }

	static bool valid(const type& value)     { UNUSED(value); return true; }

	static int compare(const type& a, const type& b) { return static_cast<int>(static_cast<int64_t>(a) - static_cast<int64_t>(b)); }
};

#endif /* ALLOWS_TEMPLATED_SIZE_T */

template <>
struct Traits<std::string>
{
	typedef std::string type;
	typedef type serialized_type;
	enum { Size = sizeof(type) };
	enum { SerializedSize = -1 };

	static ssize_t serialize(char*& dst, size_t& avail, const type& v);
	static ssize_t deserialize(const char*& src, size_t& avail, type& v);

	static size_t size(const type& value)    { return value.size(); }
	static size_t maxsize(const type& value) { return value.size(); }
	static size_t serializedsize(const type& value) { return (sizeof(uint32_t) + value.size()); }

	static bool valid(const type& value)     { UNUSED(value); return true; }

	static int compare(const type& a, const type& b) { return a.compare(b); }
};

template <size_t SIZE>
class Packer
{
public:
	static const size_t Size = SIZE;

	typedef Packer<SIZE> self_type;

	Packer() :
		m_dstp(m_buffer), m_dst_avail(sizeof(m_buffer)),
		m_srcp(m_buffer), m_src_avail(0),
		m_length(0), m_error(false) {}
	Packer(std::string data_) :
		m_dstp(m_buffer), m_dst_avail(sizeof(m_buffer)),
		m_srcp(m_buffer), m_src_avail(0),
		m_length(0), m_error(false) { data(data_); }
	Packer(const char *data_, size_t size_) :
		m_dstp(m_buffer), m_dst_avail(sizeof(m_buffer)),
		m_srcp(m_buffer), m_src_avail(0),
		m_length(0), m_error(false) { data(data_, size_); }
	Packer(const Packer& other) :
		m_dstp(m_buffer), m_dst_avail(sizeof(m_buffer)),
		m_srcp(m_buffer), m_src_avail(0),
		m_length(0), m_error(false) { data(other.data(), other.size()); }
	~Packer() {}

	const char* data() const { return m_buffer; }
	bool data(const char *data_, size_t size_) {
		if (size_ > sizeof(m_buffer))
			return false;
		memcpy(m_buffer, data_, size_);
		if (size_ < sizeof(m_buffer))
			m_buffer[size_] = '\0';
		m_length = size_;
		m_dstp = m_buffer;
		m_dst_avail = sizeof(m_buffer) - m_length;
		m_srcp = m_buffer;
		m_src_avail = m_length;
		m_error = false;
		return true;
	}
	bool data(const std::string& data_) {
		return data(data_.data(), data_.size());
	}

	size_t size() const { return m_length; }
	size_t maxsize() const { return Size; }

	size_t packing_avail() const { return m_dst_avail; }
	size_t unpacking_avail() const { return m_src_avail; }
	bool error() const { return m_error; }

	Packer& rewind() { m_dstp = m_buffer; m_dst_avail = sizeof(m_buffer); m_length = 0; m_srcp = m_buffer; m_src_avail = 0; m_error = false; return *this; }
	Packer& unpacking_rewind() { m_srcp = m_buffer; m_src_avail = m_length; return *this; }

protected:
	bool seterror(bool value = true) { bool old = m_error; m_error = value; return old; }

private:
	Packer& operator= (const Packer& other);

	char m_buffer[SIZE];
	char* m_dstp;
	size_t m_dst_avail;
	const char* m_srcp;
	size_t m_src_avail;
	size_t m_length;
	bool m_error;

	template <typename T>
	friend inline Packer& operator<< (Packer& os, const T& value)
	{
		if (os.m_dst_avail < Traits<T>::serializedsize(value))
		{
			os.seterror();
			return os;
		}
		ssize_t used = Traits<T>::serialize(os.m_dstp, os.m_dst_avail, value);
		if (used >= 0) {
			os.m_length += static_cast<size_t>(used);
			os.m_src_avail += static_cast<size_t>(used);
		} else
			os.seterror();
		return os;
	}

	template <typename T>
	friend inline Packer& operator>> (Packer& os, T& value)
	{
		if (os.m_src_avail < Traits<T>::serializedsize(value))
		{
			os.seterror();
			return os;
		}
		ssize_t consumed = Traits<T>::deserialize(os.m_srcp, os.m_src_avail, value);
		if (consumed < 0)
			os.seterror();
		return os;
	}

};

} /* end of namespace seriously */

#include "Seriously.impl.hpp"

#endif /* SERIOUSLY_H */
