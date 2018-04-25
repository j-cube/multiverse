/*****************************************************************************/
/*  multiverse - a next generation storage back-end for Alembic              */
/*                                                                           */
/*  Copyright 2015 J CUBE Inc. Tokyo, Japan.                                 */
/*                                                                           */
/*  Licensed under the Apache License, Version 2.0 (the "License");          */
/*  you may not use this file except in compliance with the License.         */
/*  You may obtain a copy of the License at                                  */
/*                                                                           */
/*      http://www.apache.org/licenses/LICENSE-2.0                           */
/*                                                                           */
/*  Unless required by applicable law or agreed to in writing, software      */
/*  distributed under the License is distributed on an "AS IS" BASIS,        */
/*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. */
/*  See the License for the specific language governing permissions and      */
/*  limitations under the License.                                           */
/*****************************************************************************/

#ifndef _Alembic_AbcCoreGit_JSON_h_
#define _Alembic_AbcCoreGit_JSON_h_

#include <Alembic/AbcCoreGit/Foundation.h>

#include <iostream>
#include <fstream>

#include <boost/optional/optional.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <rapidjson/document.h>     // rapidjson's DOM-style API
#include <rapidjson/error/en.h>     // english error messages
#include <rapidjson/stringbuffer.h> // for I/O to/from strings
#include <rapidjson/writer.h>       // for output writer

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
//
// JSON types
//
//-*****************************************************************************

enum JsonType { JSON_UNKNOWN, JSON_BOOL, JSON_INT, JSON_UINT, JSON_INT64, JSON_UINT64, JSON_DOUBLE, JSON_STRING };

//-*****************************************************************************
//
// JSON Traits Class
//
//-*****************************************************************************

#define JSON_TRAITS_PLAIN_CONVERSIONS_BOILERPLATE() \
    static NativeType ToNative(const OwnType& own) \
    { \
        return static_cast<NativeType>(own); \
    } \
    \
    static OwnType FromNative(const NativeType& native) \
    { \
        return static_cast<OwnType>(native); \
    }

#define JSON_TRAITS_BOILERPLATE(TARGET) \
    static boost::optional<TARGET> Get(const rapidjson::Value& container, const char *key) \
    { \
        rapidjson::Value::ConstMemberIterator itr = container.FindMember(key); \
        if (itr != container.MemberEnd()) \
            return Get(itr->value); \
        return boost::none; \
    } \
    \
    static boost::optional<TARGET> Get(const rapidjson::Value& container, const std::string& key) \
    { \
        rapidjson::Value::ConstMemberIterator itr = container.FindMember(key.c_str()); \
        if (itr != container.MemberEnd()) \
            return Get(itr->value); \
        return boost::none; \
    }

#define JSON_TRAITS_PLAIN_SET_BOILERPLATE() \
    static void Set(rapidjson::Document& document, rapidjson::Value& container, const std::string& key, const OwnType& value) \
    { \
        if (container.HasMember(key.c_str())) \
            container[key.c_str()] = ToNative(value); \
        else \
        { \
            rapidjson::Value k(key.c_str(), key.length(), document.GetAllocator()); \
            container.AddMember(k, ToNative(value), document.GetAllocator()); \
        } \
    } \
    static void Set(rapidjson::Document& document, rapidjson::Value& container, const char* key, const OwnType& value) \
    { \
        if (container.HasMember(key)) \
            container[key] = ToNative(value); \
        else \
        { \
            rapidjson::Value k(key, strlen(key), document.GetAllocator()); \
            container.AddMember(k, ToNative(value), document.GetAllocator()); \
        } \
    } \
    \
    static void Set(rapidjson::Document& document, rapidjson::Value& container, const std::string& key, const rapidjson::Value& value) \
    { \
        rapidjson::Value v(value, document.GetAllocator()); \
        if (container.HasMember(key.c_str())) \
            container[key.c_str()] = v; \
        else \
        { \
            rapidjson::Value k(key.c_str(), key.length(), document.GetAllocator()); \
            container.AddMember(k, v, document.GetAllocator()); \
        } \
    } \
    static void Set(rapidjson::Document& document, rapidjson::Value& container, const char* key, const rapidjson::Value& value) \
    { \
        rapidjson::Value v(value, document.GetAllocator()); \
        if (container.HasMember(key)) \
            container[key] = v; \
        else \
        { \
            rapidjson::Value k(key, strlen(key), document.GetAllocator()); \
            container.AddMember(k, v, document.GetAllocator()); \
        } \
    } \
    \
    static void Set(rapidjson::Document& document, const std::string& key, const OwnType& value) \
    { \
        Set(document, document, key, value); \
    } \
    static void Set(rapidjson::Document& document, const char* key, const OwnType& value) \
    { \
        Set(document, document, key, value); \
    } \
    \
    static void Set(rapidjson::Document& document, const std::string& key, const rapidjson::Value& value) \
    { \
        Set(document, document, key, value); \
    } \
    static void Set(rapidjson::Document& document, const char* key, const rapidjson::Value& value) \
    { \
        Set(document, document, key, value); \
    }

template <typename T>
struct JsonTraits
{
    typedef T NativeType;
};

template <>
struct JsonTraits<bool>
{
    enum { TypeId = JSON_BOOL };

    typedef bool NativeType;
    typedef bool OwnType;

    JSON_TRAITS_PLAIN_CONVERSIONS_BOILERPLATE();

    JSON_TRAITS_BOILERPLATE(OwnType);

    static boost::optional<OwnType> Get(const rapidjson::Value& value)
    {
        if (value.IsBool())
            return FromNative(value.GetBool());
        return boost::none;
    }

    JSON_TRAITS_PLAIN_SET_BOILERPLATE();
};

template <>
struct JsonTraits<int>
{
    enum { TypeId = JSON_INT };

    typedef int NativeType;
    typedef int OwnType;

    JSON_TRAITS_PLAIN_CONVERSIONS_BOILERPLATE();

    JSON_TRAITS_BOILERPLATE(OwnType);

    static boost::optional<OwnType> Get(const rapidjson::Value& value)
    {
        if (value.IsInt())
            return FromNative(value.GetInt());
        else if (value.IsUint())
            return FromNative(value.GetUint());
        else if (value.IsInt64())
            return FromNative(value.GetInt64());
        else if (value.IsUint64())
            return FromNative(value.GetUint64());
        else if (value.IsDouble())
            return FromNative(value.GetDouble());
        return boost::none;
    }

    JSON_TRAITS_PLAIN_SET_BOILERPLATE();
};

template <>
struct JsonTraits<unsigned>
{
    enum { TypeId = JSON_UINT };

    typedef unsigned NativeType;
    typedef unsigned OwnType;

    JSON_TRAITS_PLAIN_CONVERSIONS_BOILERPLATE();

    JSON_TRAITS_BOILERPLATE(OwnType);

    static boost::optional<OwnType> Get(const rapidjson::Value& value)
    {
        if (value.IsUint())
            return FromNative(value.GetUint());
        else if (value.IsInt())
            return FromNative(value.GetInt());
        else if (value.IsInt64())
            return FromNative(value.GetInt64());
        else if (value.IsUint64())
            return FromNative(value.GetUint64());
        else if (value.IsDouble())
            return FromNative(value.GetDouble());
        return boost::none;
    }

    JSON_TRAITS_PLAIN_SET_BOILERPLATE();
};

template <>
struct JsonTraits<int64_t>
{
    enum { TypeId = JSON_INT64 };

    typedef int64_t NativeType;
    typedef int64_t OwnType;

    JSON_TRAITS_PLAIN_CONVERSIONS_BOILERPLATE();

    JSON_TRAITS_BOILERPLATE(OwnType);

    static boost::optional<OwnType> Get(const rapidjson::Value& value)
    {
      if (value.IsInt64())
            return FromNative(value.GetInt64());
        else if (value.IsInt())
            return FromNative(value.GetInt());
        else if (value.IsUint())
            return FromNative(value.GetUint());
        else if (value.IsUint64())
            return FromNative(value.GetUint64());
        else if (value.IsDouble())
            return FromNative(value.GetDouble());
        return boost::none;
      }

    JSON_TRAITS_PLAIN_SET_BOILERPLATE();
};

template <>
struct JsonTraits<uint64_t>
{
    enum { TypeId = JSON_UINT64 };

    typedef uint64_t NativeType;
    typedef uint64_t OwnType;

    JSON_TRAITS_PLAIN_CONVERSIONS_BOILERPLATE();

    JSON_TRAITS_BOILERPLATE(OwnType);

    static boost::optional<OwnType> Get(const rapidjson::Value& value)
    {
        if (value.IsUint64())
            return FromNative(value.GetUint64());
        else if (value.IsInt())
            return FromNative(value.GetInt());
        else if (value.IsUint())
            return FromNative(value.GetUint());
        else if (value.IsInt64())
            return FromNative(value.GetInt64());
        else if (value.IsDouble())
            return FromNative(value.GetDouble());
        return boost::none;
    }

    JSON_TRAITS_PLAIN_SET_BOILERPLATE();
};

template <>
struct JsonTraits<double>
{
    enum { TypeId = JSON_DOUBLE };

    typedef double NativeType;
    typedef double OwnType;

    JSON_TRAITS_PLAIN_CONVERSIONS_BOILERPLATE();

    JSON_TRAITS_BOILERPLATE(OwnType);

    static boost::optional<OwnType> Get(const rapidjson::Value& value)
    {
        if (value.IsDouble())
            return FromNative(value.GetDouble());
        else if (value.IsInt())
            return FromNative(value.GetInt());
        else if (value.IsUint())
            return FromNative(value.GetUint());
        else if (value.IsInt64())
            return FromNative(value.GetInt64());
        else if (value.IsUint64())
            return FromNative(value.GetUint64());
        return boost::none;
    }

    JSON_TRAITS_PLAIN_SET_BOILERPLATE();
};

template <>
struct JsonTraits<int8_t>
{
    enum { TypeId = JSON_INT };

    typedef int NativeType;
    typedef int8_t OwnType;

    JSON_TRAITS_PLAIN_CONVERSIONS_BOILERPLATE();

    JSON_TRAITS_BOILERPLATE(OwnType);

    static boost::optional<OwnType> Get(const rapidjson::Value& value)
    {
        if (value.IsInt())
            return FromNative(value.GetInt());
        else if (value.IsUint())
            return FromNative(value.GetUint());
        else if (value.IsInt64())
            return FromNative(value.GetInt64());
        else if (value.IsUint64())
            return FromNative(value.GetUint64());
        else if (value.IsDouble())
            return FromNative(value.GetDouble());
        return boost::none;
      }

    JSON_TRAITS_PLAIN_SET_BOILERPLATE();
};

template <>
struct JsonTraits<uint8_t>
{
    enum { TypeId = JSON_UINT };

    typedef unsigned NativeType;
    typedef uint8_t OwnType;

    JSON_TRAITS_PLAIN_CONVERSIONS_BOILERPLATE();

    JSON_TRAITS_BOILERPLATE(OwnType);

    static boost::optional<OwnType> Get(const rapidjson::Value& value)
    {
        if (value.IsUint())
            return FromNative(value.GetUint());
        else if (value.IsInt())
            return FromNative(value.GetInt());
        else if (value.IsInt64())
            return FromNative(value.GetInt64());
        else if (value.IsUint64())
            return FromNative(value.GetUint64());
        else if (value.IsDouble())
            return FromNative(value.GetDouble());
        return boost::none;
    }

    JSON_TRAITS_PLAIN_SET_BOILERPLATE();
};

template <>
struct JsonTraits<std::string>
{
    enum { TypeId = JSON_STRING };

    typedef rapidjson::Value::Ch* NativeType;
    typedef std::string OwnType;

#if 0
    static NativeType ToNative(const std::string& own)
    {
        return (NativeType)(own.c_str());
    }

    static std::string FromNative(const NativeType& native)
    {
        return std::string(native);
    }
#endif

    JSON_TRAITS_BOILERPLATE(OwnType);

    static boost::optional<OwnType> Get(const rapidjson::Value& value)
    {
        if (value.IsString())
            return std::string(value.GetString(), value.GetStringLength());
        return boost::none;
    }

    static void Set(rapidjson::Document& document, rapidjson::Value& container, const std::string& key, const OwnType& value)
    {
        rapidjson::Value v(value.data(), value.size(), document.GetAllocator());
        if (container.HasMember(key.c_str()))
            container[key.c_str()] = v;
        else
        {
            rapidjson::Value k(key.c_str(), key.length(), document.GetAllocator());
            container.AddMember(k, v, document.GetAllocator());
        }
    }

    static void Set(rapidjson::Document& document, rapidjson::Value& container, const char* key, const OwnType& value)
    {
        rapidjson::Value v(value.data(), value.size(), document.GetAllocator());
        if (container.HasMember(key))
            container[key] = v;
        else
        {
            rapidjson::Value k(key, strlen(key), document.GetAllocator());
            container.AddMember(k, v, document.GetAllocator());
        }
    }

    static void Set(rapidjson::Document& document, rapidjson::Value& container, const std::string& key, const rapidjson::Value& value)
    {
        rapidjson::Value v(value, document.GetAllocator());
        if (container.HasMember(key.c_str()))
            container[key.c_str()] = v;
        else
        {
            rapidjson::Value k(key.c_str(), key.length(), document.GetAllocator());
            container.AddMember(k, v, document.GetAllocator());
        }
    }

    static void Set(rapidjson::Document& document, rapidjson::Value& container, const char* key, const rapidjson::Value& value)
    {
        rapidjson::Value v(value, document.GetAllocator());
        if (container.HasMember(key))
            container[key] = v;
        else
        {
            rapidjson::Value k(key, strlen(key), document.GetAllocator());
            container.AddMember(k, v, document.GetAllocator());
        }
    }

    static void Set(rapidjson::Document& document, const std::string& key, const OwnType& value)
    {
        Set(document, document, key, value);
    }

    static void Set(rapidjson::Document& document, const char* key, const OwnType& value)
    {
        Set(document, document, key, value);
    }

    static void Set(rapidjson::Document& document, const std::string& key, const rapidjson::Value& value)
    {
        Set(document, document, key, value);
    }

    static void Set(rapidjson::Document& document, const char* key, const rapidjson::Value& value)
    {
        Set(document, document, key, value);
    }
};

template <>
struct JsonTraits<char*>
{
    enum { TypeId = JSON_STRING };

    typedef rapidjson::Value::Ch* NativeType;
    typedef const char* OwnType;                    // Use const so it's easier to deal with return values

#if 0
    static NativeType ToNative(const std::string& own)
    {
        return (NativeType)(own.c_str());
    }

    static std::string FromNative(const NativeType& native)
    {
        return std::string(native);
    }
#endif

    JSON_TRAITS_BOILERPLATE(OwnType);

    static boost::optional<OwnType> Get(const rapidjson::Value& value)
    {
        // TODO: use a shared pointer or something
        if (value.IsString())
            return value.GetString();
        return boost::none;
    }

    static void Set(rapidjson::Document& document, rapidjson::Value& container, const std::string& key, const OwnType& value)
    {
        rapidjson::Value v((char *)value, strlen(value), document.GetAllocator());
        if (container.HasMember(key.c_str()))
            container[key.c_str()] = v;
        else
        {
            rapidjson::Value k(key.c_str(), key.length(), document.GetAllocator());
            container.AddMember(k, v, document.GetAllocator());
        }
    }

    static void Set(rapidjson::Document& document, rapidjson::Value& container, const char* key, const OwnType& value)
    {
        rapidjson::Value v((char *)value, strlen(value), document.GetAllocator());
        if (container.HasMember(key))
            container[key] = v;
        else
        {
            rapidjson::Value k(key, strlen(key), document.GetAllocator());
            container.AddMember(k, v, document.GetAllocator());
        }
    }

    static void Set(rapidjson::Document& document, rapidjson::Value& container, const std::string& key, const rapidjson::Value& value)
    {
        rapidjson::Value v(value, document.GetAllocator());
        if (container.HasMember(key.c_str()))
            container[key.c_str()] = v;
        else
        {
            rapidjson::Value k(key.c_str(), key.length(), document.GetAllocator());
            container.AddMember(k, v, document.GetAllocator());
        }
    }

    static void Set(rapidjson::Document& document, rapidjson::Value& container, const char* key, const rapidjson::Value& value)
    {
        rapidjson::Value v(value, document.GetAllocator());
        if (container.HasMember(key))
            container[key] = v;
        else
        {
            rapidjson::Value k(key, strlen(key), document.GetAllocator());
            container.AddMember(k, v, document.GetAllocator());
        }
    }

    static void Set(rapidjson::Document& document, const std::string& key, const OwnType& value)
    {
        Set(document, document, key, value);
    }

    static void Set(rapidjson::Document& document, const char* key, const OwnType& value)
    {
        Set(document, document, key, value);
    }

    static void Set(rapidjson::Document& document, const std::string& key, const rapidjson::Value& value)
    {
        Set(document, document, key, value);
    }

    static void Set(rapidjson::Document& document, const char* key, const rapidjson::Value& value)
    {
        Set(document, document, key, value);
    }
};

//-*****************************************************************************
//
// Helper functions
//
//-*****************************************************************************

inline boost::optional<bool> JsonGetBool(const rapidjson::Value& value)
{
    return JsonTraits<bool>::Get(value);
}

inline boost::optional<bool> JsonGetBool(const rapidjson::Value& container, const char *key)
{
    return JsonTraits<bool>::Get(container, key);
}

inline boost::optional<int> JsonGetInt(const rapidjson::Value& value)
{
    return JsonTraits<int>::Get(value);
}

inline boost::optional<int> JsonGetInt(const rapidjson::Value& container, const char *key)
{
    return JsonTraits<int>::Get(container, key);
}

inline boost::optional<unsigned int> JsonGetUint(const rapidjson::Value& value)
{
    return JsonTraits<unsigned>::Get(value);
}

inline boost::optional<unsigned int> JsonGetUint(const rapidjson::Value& container, const char *key)
{
    return JsonTraits<unsigned>::Get(container, key);
}

inline boost::optional<Util::int64_t> JsonGetInt64(const rapidjson::Value& value)
{
    boost::optional<int64_t> o_r = JsonTraits<int64_t>::Get(value);
    if (o_r)
        return static_cast<Util::int64_t>(*o_r);
    return boost::none;
}

inline boost::optional<Util::int64_t> JsonGetInt64(const rapidjson::Value& container, const char *key)
{
    boost::optional<int64_t> o_r = JsonTraits<int64_t>::Get(container, key);
    if (o_r)
        return static_cast<Util::int64_t>(*o_r);
    return boost::none;
}

inline boost::optional<Util::uint64_t> JsonGetUint64(const rapidjson::Value& value)
{
    boost::optional<uint64_t> o_r = JsonTraits<uint64_t>::Get(value);
    if (o_r)
        return static_cast<Util::uint64_t>(*o_r);
    return boost::none;
}

inline boost::optional<Util::uint64_t> JsonGetUint64(const rapidjson::Value& container, const char *key)
{
    boost::optional<uint64_t> o_r = JsonTraits<uint64_t>::Get(container, key);
    if (o_r)
        return static_cast<Util::uint64_t>(*o_r);
    return boost::none;
}

inline boost::optional<size_t> JsonGetSizeT(const rapidjson::Value& value)
{
    boost::optional<uint64_t> o_r = JsonTraits<uint64_t>::Get(value);
    if (o_r)
        return static_cast<size_t>(*o_r);
    return boost::none;
}

inline boost::optional<size_t> JsonGetSizeT(const rapidjson::Value& container, const char *key)
{
    boost::optional<uint64_t> o_r = JsonTraits<uint64_t>::Get(container, key);
    if (o_r)
        return static_cast<size_t>(*o_r);
    return boost::none;
}

inline boost::optional<double> JsonGetDouble(const rapidjson::Value& value)
{
    return JsonTraits<double>::Get(value);
}

inline boost::optional<double> JsonGetDouble(const rapidjson::Value& container, const char *key)
{
    return JsonTraits<double>::Get(container, key);
}

inline boost::optional<std::string> JsonGetString(const rapidjson::Value& value)
{
    return JsonTraits<std::string>::Get(value);
}

inline boost::optional<std::string> JsonGetString(const rapidjson::Value& container, const char *key)
{
    return JsonTraits<std::string>::Get(container, key);
}

template <typename T>
inline void JsonSet(rapidjson::Document& container, const char *key, const T& value)
{
    JsonTraits<T>::Set(container, key, value);
}

// template specialization for size_t for portability (no guarantees on size_t size across platforms...)
// we use uint64_t to account for the largest possible size
template <>
inline void JsonSet<size_t>(rapidjson::Document& container, const char *key, const size_t& value)
{
    JsonTraits<uint64_t>::Set(container, key, static_cast<uint64_t>(value));
}

// NOTE: this is not a template specialization!
inline void JsonSet(rapidjson::Document& container, const char *key, const char* value)
{
    JsonTraits<char*>::Set(container, key, value);
}

template <>
inline void JsonSet<rapidjson::Value>(rapidjson::Document& container, const char *key, const rapidjson::Value& value)
{
    rapidjson::Value v(value, container.GetAllocator());
    if (container.HasMember(key))
        container[key] = v;
    else
    {
        rapidjson::Value k(key, strlen(key), container.GetAllocator());
        container.AddMember(k, v, container.GetAllocator());
    }
}

template <typename T>
inline void JsonSet(rapidjson::Document& document, rapidjson::Value& container, const char *key, const T& value)
{
    JsonTraits<T>::Set(document, container, key, value);
}

// template specialization for size_t for portability (no guarantees on size_t size across platforms...)
// we use uint64_t to account for the largest possible size
template <>
inline void JsonSet<size_t>(rapidjson::Document& document, rapidjson::Value& container, const char *key, const size_t& value)
{
    JsonTraits<uint64_t>::Set(document, container, key, static_cast<uint64_t>(value));
}

template <>
inline void JsonSet<std::string>(rapidjson::Document& document, rapidjson::Value& container, const char *key, const std::string& value)
{
    JsonTraits<std::string>::Set(document, container, key, value);
}

template <>
inline void JsonSet<rapidjson::Value>(rapidjson::Document& document, rapidjson::Value& container, const char *key, const rapidjson::Value& value)
{
    rapidjson::Value v(value, document.GetAllocator());
    if (container.HasMember(key))
        container[key] = v;
    else
    {
        rapidjson::Value k(key, strlen(key), document.GetAllocator());
        container.AddMember(k, v, document.GetAllocator());
    }
}

//-*****************************************************************************
//
// JSON Parsing
//
//-*****************************************************************************

class JSONParser
{
public:
    JSONParser();
    JSONParser(const std::string& jsonPathname, const std::string& jsonContents);
    JSONParser(const std::string& jsonPathname);
    virtual ~JSONParser() {}

    rapidjson::Document document;  // Default template parameter uses UTF8 and MemoryPoolAllocator.
    bool ok;

    bool parseFile(const std::string& jsonPathname);
    bool parseString(const std::string& jsonPathname, const std::string& jsonContents);

private:
    Util::shared_ptr<char> m_jsonCharBufferPtr;
};

inline std::string JsonWrite(const rapidjson::Document& document)
{
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);
    // rapidjson::StringStream ss;
    // PrettyWriter<StringStream> writer(ss);
    // document.Accept(writer);

    return buffer.GetString();
}

} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif
