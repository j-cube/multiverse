/*****************************************************************************/
/*  multiverse - a next generation storage back-end for Alembic              */
/*  Copyright (C) 2015 J CUBE Inc. Tokyo, Japan. All Rights Reserved.        */
/*                                                                           */
/*  This program is free software: you can redistribute it and/or modify     */
/*  it under the terms of the GNU General Public License as published by     */
/*  the Free Software Foundation, either version 3 of the License, or        */
/*  (at your option) any later version.                                      */
/*                                                                           */
/*  This program is distributed in the hope that it will be useful,          */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of           */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            */
/*  GNU General Public License for more details.                             */
/*                                                                           */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    */
/*                                                                           */
/*    J CUBE Inc.                                                            */
/*    6F Azabu Green Terrace                                                 */
/*    3-20-1 Minami-Azabu, Minato-ku, Tokyo                                  */
/*    info@-jcube.jp                                                         */
/*****************************************************************************/

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

    inline static bool Pack(msgpack::packer<Stream>& pk, const Util::bool_t& value)
    {
        pk.pack(ToNative(value.asBool()));
        return true;
    }

    inline static bool Unpack(const msgpack::object& pko, Util::bool_t& value)
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

template <typename Stream>
struct MsgPackTraits<Stream, AbcA::ArraySample::Key>
{
    typedef AbcA::ArraySample::Key OwnType;

    inline static std::string pod2str(const Alembic::Util::PlainOldDataType& pod)
    {
        std::ostringstream ss;
        ss << PODName( pod );
        return ss.str();
    }

    inline static uint8_t hexchar2int(char input)
    {
        if ((input >= '0') && (input <= '9'))
            return (input - '0');
        if ((input >= 'A') && (input <= 'F'))
            return (input - 'A') + 10;
        if ((input >= 'a') && (input <= 'f'))
            return (input - 'a') + 10;
        ABCA_THROW( "invalid input char" );
    }

    // This function assumes src to be a zero terminated sanitized string with
    // an even number of [0-9a-f] characters, and target to be sufficiently large
    inline static size_t hex2bin(uint8_t *dst, const char* src)
    {
        uint8_t *dst_orig = dst;

        while (*src && src[1])
        {
            *dst      = hexchar2int(*(src++)) << 4;
            *(dst++) |= hexchar2int(*(src++));
        }

        return (dst - dst_orig);
    }

    inline static bool Pack(msgpack::packer<Stream>& pk, const AbcA::ArraySample::Key& value)
    {
        msgpack::type::tuple< size_t, std::string, std::string, std::string >
            tuple(value.numBytes, pod2str(value.origPOD), pod2str(value.readPOD), value.digest.str());
        pk.pack(tuple);
        return true;
    }

    inline static bool Unpack(const msgpack::object& pko, AbcA::ArraySample::Key& value)
    {
        msgpack::type::tuple< size_t, std::string, std::string, std::string > tuple;
        pko.convert(&tuple);

        size_t      k_num_bytes = tuple.get<0>();
        std::string k_orig_pod  = tuple.get<1>();
        std::string k_read_pod  = tuple.get<2>();
        std::string k_digest    = tuple.get<3>();

        AbcA::ArraySample::Key key;

        key.numBytes = k_num_bytes;
        key.origPOD = Alembic::Util::PODFromName( k_orig_pod );
        key.readPOD = Alembic::Util::PODFromName( k_read_pod );
        hex2bin(key.digest.d, k_digest.c_str());

        value = key;
        return true;
    }
};

template <typename Stream>
struct MsgPackTraits<Stream, AbcA::Dimensions>
{
    typedef AbcA::Dimensions OwnType;

    inline static bool Pack(msgpack::packer<Stream>& pk, const AbcA::Dimensions& value)
    {
        std::vector<uint64_t> v_dimensions;
        for (size_t i = 0; i < value.rank(); ++i )
            v_dimensions.push_back( static_cast<uint64_t>( value[i] ) );
        pk.pack(v_dimensions);
        return true;
    }

    inline static bool Unpack(const msgpack::object& pko, AbcA::Dimensions& value)
    {
        std::vector<uint64_t> v_dimensions;
        pko.convert(&v_dimensions);

        AbcA::Dimensions dimensions;
        dimensions.setRank( v_dimensions.size() );
        {
            int idx = 0;
            for (std::vector<uint64_t>::const_iterator it = v_dimensions.begin(); it != v_dimensions.end(); ++it)
            {
                dimensions[idx] = (*it);
                idx++;
            }
        }

        value = dimensions;
        return true;
    }
};

template <typename Stream>
struct MsgPackTraits<Stream, Alembic::Util::PlainOldDataType>
{
    typedef Alembic::Util::PlainOldDataType OwnType;

    inline static bool Pack(msgpack::packer<Stream>& pk, const Alembic::Util::PlainOldDataType& value)
    {
        std::string v_typename;
        {
            std::ostringstream ss;
            ss << PODName( value );
            v_typename = ss.str();
        }

        pk.pack(v_typename);
        return true;
    }

    inline static bool Unpack(const msgpack::object& pko, Alembic::Util::PlainOldDataType& value)
    {
        std::string v_typename;
        pko.convert(&v_typename);

        Alembic::Util::PlainOldDataType pod = Alembic::Util::PODFromName( v_typename );

        value = pod;
        return true;
    }
};

template <typename Stream>
struct MsgPackTraits<Stream, AbcA::DataType>
{
    typedef AbcA::DataType OwnType;

    inline static bool Pack(msgpack::packer<Stream>& pk, const AbcA::DataType& value)
    {
        std::string v_typename;
        {
            std::ostringstream ss;
            ss << PODName( value.getPod() );
            v_typename = ss.str();
        }

        int v_extent = value.getExtent();

        msgpack::type::tuple< std::string, int >
            tuple(v_typename, v_extent);
        pk.pack(tuple);
        return true;
    }

    inline static bool Unpack(const msgpack::object& pko, AbcA::DataType& value)
    {
        msgpack::type::tuple< std::string, int > tuple;
        pko.convert(&tuple);

        std::string v_typename  = tuple.get<0>();
        int         v_extent    = tuple.get<1>();

        Alembic::Util::PlainOldDataType pod = Alembic::Util::PODFromName( v_typename );
        AbcA::DataType dataType(pod, v_extent);

        value = dataType;
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
