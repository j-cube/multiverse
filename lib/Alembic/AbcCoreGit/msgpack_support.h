//-*****************************************************************************
//
// Copyright (c) 2015,
//
// All rights reserved.
//
//-*****************************************************************************

#ifndef _Alembic_AbcCoreGit_msgpack_support_h_
#define _Alembic_AbcCoreGit_msgpack_support_h_

#include <Alembic/AbcCoreGit/Foundation.h>
#include <Alembic/AbcCoreGit/Utils.h>

#include <boost/locale.hpp>

#include <msgpack.hpp>


namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

// std::wstring support

inline std::vector<int> wstring_to_intvec(const std::wstring& ws)
{
    std::vector<int> iv;

    for (std::wstring::const_iterator it = ws.begin(); it != ws.end(); ++it)
    {
        const wchar_t& ch = *it;
        iv.push_back(static_cast<int>(ch));
    }
    return iv;
}

inline std::wstring intvec_to_wstring(const std::vector<int>& iv)
{
    std::wstring ws;

    for (std::vector<int>::const_iterator it = iv.begin(); it != iv.end(); ++it)
    {
        const int& val = *it;
        ws.push_back(static_cast<wchar_t>(val));
    }
    return ws;
}


// msgpack helper traits

template <typename Stream, typename T>
struct MsgPackTraits
{
    typedef T NativeType;
    typedef T OwnType;

    inline static NativeType ToNative(const OwnType& own)
    {
        return static_cast<NativeType>(own);
    }

    inline static OwnType FromNative(const NativeType& native)
    {
        return static_cast<OwnType>(native);
    }

    inline static bool Pack(msgpack::packer<Stream>& pk, const OwnType& value)
    {
        pk.pack(ToNative(value));
        return true;
    }

    inline static bool Unpack(const msgpack::object& pko, OwnType& value)
    {
        pko.convert(&value);
        return true;
    }
};

template <typename Stream>
struct MsgPackTraits<Stream, wchar_t>
{
    typedef int NativeType;
    typedef wchar_t OwnType;

    inline static NativeType ToNative(const OwnType& own)
    {
        return static_cast<NativeType>(own);
    }

    inline static OwnType FromNative(const NativeType& native)
    {
        return static_cast<OwnType>(native);
    }

    inline static bool Pack(msgpack::packer<Stream>& pk, const wchar_t& value)
    {
        pk.pack(ToNative(value));
        return true;
    }

    inline static bool Unpack(const msgpack::object& pko, wchar_t& value)
    {
        NativeType nv;
        pko.convert(&nv);
        value = FromNative(nv);
        return true;
    }
};

template <typename Stream>
struct MsgPackTraits<Stream, Util::bool_t>
{
    typedef bool NativeType;
    typedef Util::bool_t OwnType;

    inline static NativeType ToNative(const OwnType& own)
    {
        return static_cast<NativeType>(own);
    }

    inline static OwnType FromNative(const NativeType& native)
    {
        return static_cast<OwnType>(native);
    }

    inline static bool Pack(msgpack::packer<Stream>& pk, const wchar_t& value)
    {
        pk.pack(ToNative(value));
        return true;
    }

    inline static bool Unpack(const msgpack::object& pko, wchar_t& value)
    {
        NativeType nv;
        pko.convert(&nv);
        value = FromNative(nv);
        return true;
    }
};

template <typename Stream>
struct MsgPackTraits<Stream, half>
{
    typedef double NativeType;
    typedef half OwnType;

    inline static NativeType ToNative(const OwnType& own)
    {
        return static_cast<NativeType>(own);
    }

    inline static OwnType FromNative(const NativeType& native)
    {
        return static_cast<OwnType>(native);
    }

    inline static bool Pack(msgpack::packer<Stream>& pk, const half& value)
    {
        pk.pack(ToNative(value));
        return true;
    }

    inline static bool Unpack(const msgpack::object& pko, half& value)
    {
        NativeType nv;
        pko.convert(&nv);
        value = FromNative(nv);
        return true;
    }
};

template <typename Stream>
struct MsgPackTraits<Stream, std::wstring>
{
    typedef std::vector<int> NativeType;
    typedef std::wstring OwnType;

    inline static NativeType ToNative(const OwnType& own)
    {
        return wstring_to_intvec(own);
    }

    inline static OwnType FromNative(const NativeType& native)
    {
        return intvec_to_wstring(native);
    }

    inline static bool Pack(msgpack::packer<Stream>& pk, const std::wstring& value)
    {
        pk.pack(wstring_to_intvec(value));
        return true;
    }

    inline static bool Unpack(const msgpack::object& pko, std::wstring& value)
    {
        std::vector<int> iv;
        pko.convert(&iv);
        value = intvec_to_wstring(iv);
        return true;
    }
};

template <typename Stream, typename W>
struct MsgPackTraits<Stream, std::vector<W> >
{
    inline static bool Pack(msgpack::packer<Stream>& pk, const std::vector<W>& value)
    {
        typedef MsgPackTraits<Stream, W> InnerTraits;

        typedef std::vector< typename InnerTraits::NativeType > NativeVector;

        NativeVector nVec;
        for (typename std::vector<W>::const_iterator it = value.begin(); it != value.end(); ++it)
        {
            const W& el = *it;

            nVec.push_back(InnerTraits::ToNative(el));
        }

        pk.pack(nVec);
        return true;
    }

    inline static bool Unpack(const msgpack::object& pko, std::vector<W>& value)
    {
        typedef MsgPackTraits<Stream, W> InnerTraits;

        typedef std::vector< typename InnerTraits::NativeType > NativeVector;

        NativeVector nVec;
        pko.convert(&nVec);

        value.clear();
        for (typename NativeVector::const_iterator it = nVec.begin(); it != nVec.end(); ++it)
        {
            const typename InnerTraits::NativeType& el = *it;

            value.push_back(InnerTraits::FromNative(el));
        }

        return true;
    }
};


// packing

/*
 * template <typename Stream, typename T>
 *   bool mp_pack(msgpack::packer<Stream>& pk, const T& value);
 */

template <typename Stream, typename T>
inline bool mp_pack(msgpack::packer<Stream>& pk, const T& value)
{
    typedef MsgPackTraits< Stream, T > Traits;
    return Traits::Pack(pk, value);
}



/*
 * template <typename Stream, typename T>
 *   bool mp_pack(msgpack::object::with_zone& pko, const T& value);
 */

template <typename Stream, typename T>
inline bool mp_pack(msgpack::object::with_zone& pko, const T& value)
{
    pko << value;
    return true;
}

template <typename Stream>
inline bool mp_pack(msgpack::object::with_zone& pko, const std::wstring& value)
{
    std::vector<int> iv = wstring_to_intvec(value);

    pko.type = msgpack::type::ARRAY;
    pko.via.array.size = iv.size();
    pko.via.array.ptr = static_cast<msgpack::object*>(pko.zone.allocate_align(sizeof(msgpack::object) * pko.via.array.size));

    int idx = 0;
    for (std::vector<int>::const_iterator it = iv.begin(); it != iv.end(); ++it)
    {
        const int& el = *it;
        pko.via.array.ptr[idx] = msgpack::object(el, pko.zone);
        idx++;
    }

    return true;
}

template <typename Stream>
inline bool mp_pack(msgpack::object::with_zone& pko, const wchar_t& value)
{
    pko.type = msgpack::type::NEGATIVE_INTEGER;
    pko.via.i64 = static_cast<int>(value);
    return true;
}

template <typename Stream>
inline bool mp_pack(msgpack::object::with_zone& pko, const half& value)
{
    pko.type = msgpack::type::FLOAT;
    pko.via.i64 = static_cast<double>(value);
    return true;
}

template <typename Stream>
inline bool mp_pack(msgpack::object::with_zone& pko, const Util::bool_t& value)
{
    pko.type = msgpack::type::BOOLEAN;
    pko.via.boolean = static_cast<bool>(value);
    return true;
}

/*
 * template <typename T>
 * bool mp_unpack(const msgpack::object& pko, T& value);
 */

template <typename T>
inline bool mp_unpack(const msgpack::object& pko, T& value)
{
    typedef MsgPackTraits< std::stringstream, T > Traits;
    return Traits::Unpack(pko, value);
}



} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif
