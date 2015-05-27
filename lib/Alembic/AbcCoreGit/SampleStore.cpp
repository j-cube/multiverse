//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cassert>

#include <Alembic/AbcCoreGit/SampleStore.h>
#include <Alembic/AbcCoreGit/Utils.h>

#include <Alembic/AbcCoreGit/msgpack_support.h>

#include <msgpack.hpp>


namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {


template <typename T>
static void TRACE_VALUE(std::string msg, T v)
{
    TRACE(msg << "UNKNOWN TYPE");
}

template <>
void TRACE_VALUE(std::string msg, Util::bool_t v)
{
    TRACE(msg << " (u::bool_t) " << v);
}

template <>
void TRACE_VALUE(std::string msg, Util::uint8_t v)
{
    TRACE(msg << " (u::uint8_t) " << v);
}

template <>
void TRACE_VALUE(std::string msg, Util::int8_t v)
{
    TRACE(msg << " (u::int8_t) " << v);
}

template <>
void TRACE_VALUE(std::string msg, Util::uint16_t v)
{
    TRACE(msg << " (u::uint16_t) " << v);
}

template <>
void TRACE_VALUE(std::string msg, Util::int16_t v)
{
    TRACE(msg << " (u::int16_t) " << v);
}

template <>
void TRACE_VALUE(std::string msg, Util::uint32_t v)
{
    TRACE(msg << " (u::uint32_t) " << v);
}

template <>
void TRACE_VALUE(std::string msg, Util::int32_t v)
{
    TRACE(msg << " (u::int32_t) " << v);
}

template <>
void TRACE_VALUE(std::string msg, Util::uint64_t v)
{
    TRACE(msg << " (u::uint64_t) " << v);
}

template <>
void TRACE_VALUE(std::string msg, Util::int64_t v)
{
    TRACE(msg << " (u::int64_t) " << v);
}

template <>
void TRACE_VALUE(std::string msg, Util::float16_t v)
{
    TRACE(msg << " (u::float16_t) " << v);
}

template <>
void TRACE_VALUE(std::string msg, Util::float32_t v)
{
    TRACE(msg << " (u::float32_t) " << v);
}

template <>
void TRACE_VALUE(std::string msg, Util::float64_t v)
{
    TRACE(msg << " (u::float64_t) " << v);
}

template <>
void TRACE_VALUE(std::string msg, Util::string v)
{
    TRACE(msg << " (u::string) " << v);
}

template <>
void TRACE_VALUE(std::string msg, Util::wstring v)
{
    TRACE(msg << " (u::wstring) (DON'T KNOW HOW TO REPRESENT)");
}

template <>
void TRACE_VALUE(std::string msg, char v)
{
    TRACE(msg << " (char) " << v);
}

#if 0
static std::string pod2str(const Alembic::Util::PlainOldDataType& pod)
{
    std::ostringstream ss;
    ss << PODName( pod );
    return ss.str();
}

static uint8_t hexchar2int(char input)
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
static size_t hex2bin(uint8_t *dst, const char* src)
{
    uint8_t *dst_orig = dst;

    while (*src && src[1])
    {
        *dst      = hexchar2int(*(src++)) << 4;
        *(dst++) |= hexchar2int(*(src++));
    }

    return (dst - dst_orig);
}
#endif

/* --------------------------------------------------------------------
 *
 *   TypedSampleStore<T>
 *
 * -------------------------------------------------------------------- */

template <typename T>
TypedSampleStore<T>::TypedSampleStore( AwImplPtr awimpl_ptr, const AbcA::DataType &iDataType, const AbcA::Dimensions &iDims )
    : m_awimpl_ptr(awimpl_ptr)
    , m_rwmode(WRITE)
    , m_dataType(iDataType)
    , m_dimensions(iDims)
    , m_next_index(0)
{
}

template <typename T>
TypedSampleStore<T>::TypedSampleStore( ArImplPtr arimpl_ptr, const AbcA::DataType &iDataType, const AbcA::Dimensions &iDims )
    : m_arimpl_ptr(arimpl_ptr)
    , m_rwmode(READ)
    , m_dataType(iDataType)
    , m_dimensions(iDims)
    , m_next_index(0)
{
}

template <typename T>
TypedSampleStore<T>::~TypedSampleStore()
{
}

template <typename T>
typename TypedSampleStore<T>::KeyStorePtr TypedSampleStore<T>::ks()
{
    switch (mode())
    {
    case WRITE:
        assert(m_awimpl_ptr);
        if (m_awimpl_ptr)
            return m_awimpl_ptr->ksm().getOrCreate<T>();
        break;
    case READ:
        assert(m_arimpl_ptr);
        if (m_arimpl_ptr)
            return m_arimpl_ptr->ksm().getOrCreate<T>();
        break;
    }
    return NULL;
}

template <typename T>
int TypedSampleStore<T>::sampleIndexToDataIndex( int sampleIndex )
{
    if (rank() == 0)
    {
        size_t extent = m_dataType.getExtent();

        return sampleIndex * extent;
    } else
    {
        assert( rank() >= 1 );

        size_t extent = m_dataType.getExtent();
        size_t points_per_sample = m_dimensions.numPoints();
        size_t pods_per_sample = points_per_sample * extent;

        return sampleIndex * pods_per_sample;
    }
}

template <typename T>
int TypedSampleStore<T>::dataIndexToSampleIndex( int dataIndex )
{
    if (dataIndex == 0)
        return 0;

    if (rank() == 0)
    {
        size_t extent = m_dataType.getExtent();

        return dataIndex / extent;
    } else
    {
        assert( rank() >= 1 );

        size_t extent = m_dataType.getExtent();
        size_t points_per_sample = m_dimensions.numPoints();
        size_t pods_per_sample = points_per_sample * extent;

        return dataIndex / pods_per_sample;
    }
}

template <typename T>
void TypedSampleStore<T>::getSampleT( T* iIntoLocation, int index )
{
    Alembic::Util::PlainOldDataType curPod = m_dataType.getPod();

    TRACE( "TypedSampleStore::getSample() index:" << index << "  #bytes:" << PODNumBytes( curPod ) );
    ABCA_ASSERT( (curPod != Alembic::Util::kStringPOD) && (curPod != Alembic::Util::kWstringPOD),
        "Can't convert " << m_dataType <<
        "to non-std::string / non-std::wstring" );

    // TODO: optimize skipping the key, by using a key/value index instead:
    //       sample index -> key-index -> value
    size_t kid = sampleIndexToKid(index);
    // TRACE("sampleIndex:" << index << " kid:" << kid);
    const std::vector<T>& sampleData = kidToSampleData(kid);
    // TRACE("got sample data");
    // const std::vector<T>& sampleData = sampleIndexToSampleData(index);

    if (rank() == 0)
    {
        size_t extent = m_dataType.getExtent();

        assert(extent <= sampleData.size());

        for (size_t i = 0; i < extent; ++i)
            iIntoLocation[i] = sampleData[i];
    } else
    {
        assert( rank() >= 1 );

        size_t extent = m_dataType.getExtent();
        size_t points_per_sample = m_dimensions.numPoints();
        size_t pods_per_sample = points_per_sample * extent;

        for (size_t i = 0; i < pods_per_sample; ++i)
            iIntoLocation[i] = sampleData[i];
    }
}

template <>
void TypedSampleStore<std::string>::getSampleT( std::string* iIntoLocation, int index )
{
    Alembic::Util::PlainOldDataType curPod = dataType().getPod();

    TODO("FIXME!!!");

    ABCA_ASSERT( curPod == Alembic::Util::kStringPOD,
        "Can't convert " << dataType() <<
        "to std::string" );

    const std::vector<std::string>& sampleData = sampleIndexToSampleData(index);

    const std::string& src = sampleData[0];
    // std::string src = m_data[index];
    std::string * strPtr =
        reinterpret_cast< std::string * > ( iIntoLocation );

    size_t numChars = src.size() + 1;
    char * buf = new char[ numChars + 1 ];
    std::copy(src.begin(), src.end(), buf);
    buf[src.size()] = '\0';

    std::size_t startStr = 0;
    std::size_t strPos = 0;

    for ( std::size_t i = 0; i < numChars; ++i )
    {
        if ( buf[i] == 0 )
        {
            strPtr[strPos] = buf + startStr;
            startStr = i + 1;
            strPos ++;
        }
    }

    delete [] buf;
}

template <typename T>
void TypedSampleStore<T>::getSample( void *iIntoLocation, int index )
{
    T* iIntoLocationT = reinterpret_cast<T*>(iIntoLocation);
    getSampleT(iIntoLocationT, index);
}

template <typename T>
void TypedSampleStore<T>::getSample( AbcA::ArraySamplePtr& oSample, int index )
{
    // how much space do we need?
    Util::Dimensions dims = getDimensions();

    oSample = AbcA::AllocateArraySample( getDataType(), dims );

    ABCA_ASSERT( oSample->getDataType() == m_dataType,
        "DataType on ArraySample oSamp: " << oSample->getDataType() <<
        ", does not match the DataType of the SampleStore: " <<
        m_dataType );

    getSample( const_cast<void*>( oSample->getData() ), index );

    // AbcA::ArraySample::Key key = oSample->getKey();
}

// type traits
template <typename T> struct scalar_traits
{
    static T Zero() { return static_cast<T>(0); }
};

template <>
struct scalar_traits<bool>
{
    static bool Zero() { return false; }
};

template <>
struct scalar_traits<Util::bool_t>
{
    static Util::bool_t Zero() { return false; }
};

template <>
struct scalar_traits<float>
{
    static float Zero() { return 0.0; }
};

template <>
struct scalar_traits<double>
{
    static double Zero() { return 0.0; }
};

template <>
struct scalar_traits<std::string>
{
    static std::string Zero() { return ""; }
};

template <typename T>
void TypedSampleStore<T>::addSample( const T* iSamp, const AbcA::ArraySample::Key& key )
{
    std::string key_str = key.digest.str();

    size_t at = m_next_index++;
    size_t kid = addKey(key);
    if (! m_kid_indexes.count(kid))
        m_kid_indexes[kid] = std::vector<size_t>();
    m_kid_indexes[kid].push_back( at );
    m_index_to_kid[at] = kid;
    // TRACE("set index_to_kid[" << at << "] := " << kid);

    TRACE("SampleStore::addSample(key:'" << key_str << "' #bytes:" << key.numBytes << " kid:" << kid << " index:=" << at << ")");

    if (! ks()->hasData(kid))
    {
        // m_kid_to_data[kid] = std::vector<T>();
        // std::vector<T>& data = m_kid_to_data[kid];
        std::vector<T> data;

        if (rank() == 0)
        {
            size_t extent = m_dataType.getExtent();

            data.reserve(extent);

            if (! iSamp)
            {
                for (size_t i = 0; i < extent; ++i)
                    data.push_back( scalar_traits<T>::Zero() );
            } else
            {
                for (size_t i = 0; i < extent; ++i)
                    data.push_back( iSamp[i] );
            }
        } else
        {
            assert( rank() >= 1 );

            size_t extent = m_dataType.getExtent();
            size_t points_per_sample = m_dimensions.numPoints();
            size_t pods_per_sample = points_per_sample * extent;

            data.reserve(pods_per_sample);

            if (! iSamp)
            {
                for (size_t i = 0; i < pods_per_sample; ++i)
                    data.push_back( scalar_traits<T>::Zero() );
            } else
            {
                for (size_t i = 0; i < pods_per_sample; ++i)
                    data.push_back( iSamp[i] );
            }
        }

        addData(kid, data);
    }
}

template <typename T>
void TypedSampleStore<T>::addSample( const void *iSamp, const AbcA::ArraySample::Key& key )
{
    const T* iSampT = reinterpret_cast<const T*>(iSamp);
    addSample(iSampT, key);
}

template <typename T>
void TypedSampleStore<T>::addSample( const AbcA::ArraySample& iSamp )
{
    ABCA_ASSERT( iSamp.getDataType() == m_dataType,
        "DataType on ArraySample iSamp: " << iSamp.getDataType() <<
        ", does not match the DataType of the SampleStore: " <<
        m_dataType );

    AbcA::ArraySample::Key key = iSamp.getKey();

    addSample( iSamp.getData(), key );
}

template <typename T>
void TypedSampleStore<T>::setFromPreviousSample()                           // duplicate last added sample
{
    size_t previousSampleIndex = m_next_index;

    ABCA_ASSERT( previousSampleIndex > 0,
        "No samples to duplicate in SampleStore" );

    size_t kid = sampleIndexToKid(previousSampleIndex);
    size_t at = m_next_index++;

    m_kid_indexes[kid].push_back( at );
    m_index_to_kid[at] = kid;
    // TRACE("set index_to_kid[" << at << "] := " << kid);
}

template <typename T>
bool TypedSampleStore<T>::getKey( AbcA::index_t iSampleIndex, AbcA::ArraySampleKey& oKey )
{
    if (hasIndex(iSampleIndex))
    {
        oKey = sampleIndexToKey(iSampleIndex);
        return true;
    }

    return false;
}

template <typename T>
size_t TypedSampleStore<T>::getNumSamples() const
{
    return m_next_index;
}

template <typename T>
std::string TypedSampleStore<T>::getFullTypeName() const
{
    std::ostringstream ss;
    ss << m_dataType;
    return ss.str();
}

template <typename T>
std::string TypedSampleStore<T>::repr(bool extended) const
{
    std::ostringstream ss;
    if (extended)
    {
        ss << "<TypedSampleStore samples:" << getNumSamples() <<  " type:" << m_dataType << " dims:" << m_dimensions << ">";
    } else
    {
        ss << "<TypedSampleStore samples:" << getNumSamples() << ">";
    }
    return ss.str();
}


/*
 * ScalarProperty: # of samples is fixed
 * ArrayProperty: # of samples is variable
 *
 * dimensions: array of integers (eg: [], [ 1 ], [ 6 ])
 * rank:       dimensions.rank() == len(dimensions) (eg: 0, 1, ...)
 * extent:     number of scalar values in basic element
 *
 * example: dimensions = [2, 2], rank = 3, type = f
 * corresponds to a 2x2 matrix of vectors made of 3 ing point values (3f)
 *
 * ScalarProperty has rank 0 and ArrayProperty rank > 0 ?
 *
 * # points per sample: dimensions.numPoints() == product of all dimensions' values
 *
 */

// binary serialization
template <typename T>
std::string TypedSampleStore<T>::pack()
{
    TRACE("TypedSampleStore<T>::pack() T:" << GetTypeStr<T>());

    std::string v_typename;
    {
        std::ostringstream ss;
        ss << PODName( m_dataType.getPod() );
        v_typename = ss.str();
    }

    std::string v_type;
    {
        std::ostringstream ss;
        ss << m_dataType;
        v_type = ss.str();
    }

    // std::vector<uint64_t> v_dimensions;
    // for (size_t i = 0; i < m_dimensions.rank(); ++i )
    //     v_dimensions.push_back( static_cast<uint64_t>( m_dimensions[i] ) );

    // msgpack::type::tuple< std::string, std::string, int, size_t, size_t, std::vector<uint64_t>, std::vector<T> >
    //     src(s_typename, s_type, extent(), rank(), getNumSamples(), dimensions, m_data);
    // // serialize the object into the buffer.
    // // any classes that implements write(const char*,size_t) can be a buffer.
    // std::stringstream buffer;
    // msgpack::pack(buffer, src);

    std::stringstream buffer;
    msgpack::packer<std::stringstream> pk(&buffer);

    // mp_pack(pk, v_typename);
    mp_pack(pk, v_type);        // not necessary actually (redundant, given m_dataType)
    // mp_pack(pk, extent());      // not necessary actually (redundant, given m_dataType)
    mp_pack(pk, m_dataType);
    mp_pack(pk, rank());
    mp_pack(pk, getNumSamples());
    // mp_pack(pk, v_dimensions);
    // TRACE("[JKLT] type:" << v_type << " pod:" << PODName( m_dataType.getPod() ) << " rank:" << rank() << " numSamples:" << getNumSamples());
    mp_pack(pk, m_dimensions);
    // mp_pack(pk, m_data);
    // TRACE("saved dimensions of rank " << m_dimensions.rank());
    // for (size_t i = 0; i < m_dimensions.rank(); ++i )
    //     TRACE("dim[" << i << "] = " << m_dimensions[i]);

#if 0
    // save key store
    if (! ks()->saved())
    {
        TRACE("saving key store");
        ks()->saved(true);

        mp_pack(pk, /* contains key store */ true);
        std::string ks_packed = ks()->pack();
        TRACE("key store size: " << ks_packed.length() << " bytes");
        mp_pack(pk, ks_packed);
    } else
    {
        TRACE("skipping save of key store");
        mp_pack(pk, /* contains key store */ false);
    }
    assert(ks()->saved());
#endif

    // save index->kid map (and data)
    {
        mp_pack(pk, static_cast<size_t>(m_index_to_kid.size()));
        std::map< size_t, size_t >::const_iterator p_it;
        for (p_it = m_index_to_kid.begin(); p_it != m_index_to_kid.end(); ++p_it)
        {
            size_t index = (*p_it).first;
            size_t kid   = (*p_it).second;

            msgpack::type::tuple< size_t, size_t >
                tuple(index, kid);

            mp_pack(pk, tuple);
            // TRACE("serialized index_to_kid[" << index << "] == " << kid);

            // std::vector<T>& data = m_kid_to_data[kid];
            // mp_pack(pk, data);
            // TRACE("written sample data (size:" << data.size() << ")");
        }
        mp_pack(pk, m_next_index);
    }

    // mp_pack(pk, m_index_to_kid);
    // mp_pack(pk, m_kid_indexes);
    // mp_pack(pk, m_kid_to_data);
    // mp_pack(pk, m_next_index);

#if 0
    // save keys
    mp_pack(pk, m_pos_key.size());
    std::map<size_t, AbcA::ArraySample::Key>::const_iterator p_it;
    for (p_it = m_pos_key.begin(); p_it != m_pos_key.end(); ++p_it)
    {
        size_t at = (*p_it).first;
        const AbcA::ArraySample::Key& key = (*p_it).second;

        msgpack::type::tuple< size_t, size_t, std::string, std::string, std::string >
            tuple(at, key.numBytes, pod2str(key.origPOD), pod2str(key.readPOD), key.digest.str());

        mp_pack(pk, tuple);
    }
#endif /* 0 */

    return buffer.str();
}

template <typename T>
void TypedSampleStore<T>::unpack(const std::string& packed)
{
    TRACE("TypedSampleStore<T>::unpack() T:" << GetTypeStr<T>());

    msgpack::unpacker pac;

    // copy the buffer data to the unpacker object
    pac.reserve_buffer(packed.size());
    memcpy(pac.buffer(), packed.data(), packed.size());
    pac.buffer_consumed(packed.size());

    // deserialize it.
    msgpack::unpacked msg;

    // std::string v_typename;
    std::string v_type;
    AbcA::DataType dataType;
    // int v_extent;
    size_t v_rank;
    size_t v_num_samples;
    AbcA::Dimensions dimensions;
    // std::vector<uint64_t> v_dimensions;
    // std::vector<T> v_data;
    // size_t v_n_keys;

    // unpack objects in the order in which they were packed

    // pac.next(&msg);
    // msgpack::object pko = msg.get();
    // mp_unpack(pko, v_typename);

    pac.next(&msg);
    msgpack::object pko = msg.get();
    mp_unpack(pko, v_type);

    // pac.next(&msg);
    // pko = msg.get();
    // mp_unpack(pko, v_extent);

    pac.next(&msg);
    pko = msg.get();
    mp_unpack(pko, dataType);

    pac.next(&msg);
    pko = msg.get();
    mp_unpack(pko, v_rank);

    pac.next(&msg);
    pko = msg.get();
    mp_unpack(pko, v_num_samples);

    // pac.next(&msg);
    // pko = msg.get();
    // mp_unpack(pko, v_dimensions);

    // TRACE("[JKLT] type:" << v_type << " pod:" << PODName( dataType.getPod() ) << " rank:" << v_rank << " numSamples:" << v_num_samples);
    pac.next(&msg);
    pko = msg.get();
    mp_unpack(pko, dimensions);
    // TRACE("got dimensions of rank " << dimensions.rank());
    // for (size_t i = 0; i < dimensions.rank(); ++i )
    //     TRACE("dim[" << i << "] = " << dimensions[i]);

    // pac.next(&msg);
    // pko = msg.get();
    // mp_unpack(pko, v_data);

    // re-initialize using deserialized data (keys done later)

    // AbcA::Dimensions dimensions;
    // dimensions.setRank( v_dimensions.size() );
    // {
    //     int idx = 0;
    //     for (std::vector<uint64_t>::const_iterator it = v_dimensions.begin(); it != v_dimensions.end(); ++it)
    //     {
    //         dimensions[idx] = (*it);
    //         idx++;
    //     }
    // }

    ABCA_ASSERT( dimensions.rank() == v_rank, "wrong dimensions rank" );

    // Alembic::Util::PlainOldDataType pod = Alembic::Util::PODFromName( v_typename );
    // AbcA::DataType dataType(pod, v_extent);

    // ABCA_ASSERT( dataType.getExtent() == v_extent, "wrong datatype extent" );

    m_dataType = dataType;
    m_dimensions = dimensions;

    ABCA_ASSERT( rank() == v_rank, "wrong rank" );
    // ABCA_ASSERT( extent() == v_extent, "wrong extent" );

    // m_data.clear();
    // m_data = v_data;

#if 0
    // deserialize key store
    {
        bool contains_key_store = false;

        pac.next(&msg);
        pko = msg.get();
        mp_unpack(pko, contains_key_store);

        TRACE("contains_key_store:" << (contains_key_store ? "TRUE" : "FALSE"));
        if (contains_key_store)
        {
            std::string ks_packed;

            pac.next(&msg);
            pko = msg.get();
            mp_unpack(pko, ks_packed);
            TRACE("key store size: " << ks_packed.length() << " bytes");

            ks()->unpack(ks_packed);
            ks()->loaded(true);
        }
    }
#endif
    assert(ks()->loaded());

    // deserialize index->kid map
    m_index_to_kid.clear();
    m_kid_indexes.clear();
    m_next_index = 0;
    size_t v_n_index = 0;
    pac.next(&msg);
    pko = msg.get();
    mp_unpack(pko, v_n_index);
    for (size_t i = 0; i < v_n_index; ++i)
    {
        msgpack::type::tuple< size_t, size_t > tuple;

        pac.next(&msg);
        pko = msg.get();
        mp_unpack(pko, tuple);

        size_t k_index = tuple.get<0>();
        size_t k_kid   = tuple.get<1>();

        m_index_to_kid[k_index] = k_kid;
        if (! m_kid_indexes.count(k_kid))
            m_kid_indexes[k_kid] = std::vector<size_t>();
        m_kid_indexes[k_kid].push_back( k_index );
        // TRACE("deserialized index_to_kid[" << k_index << "] := " << k_kid);

        // std::vector<T> data;
        // pac.next(&msg);
        // pko = msg.get();
        // mp_unpack(pko, data);
        // m_kid_to_data[k_kid] = data;
        // TRACE("got sample data (size:" << data.size() << ")");
    }
    pac.next(&msg);
    pko = msg.get();
    mp_unpack(pko, m_next_index);

#if 0
    // deserialize keys
    pac.next(&msg);
    pko = msg.get();
    mp_unpack(pko, v_n_keys);

    m_key_pos.clear();
    m_pos_key.clear();
    for (size_t i = 0; i < v_n_keys; ++i)
    {
        msgpack::type::tuple< size_t, size_t, std::string, std::string, std::string > tuple;

        pac.next(&msg);
        pko = msg.get();
        mp_unpack(pko, tuple);

        size_t      k_at        = tuple.get<0>();
        size_t      k_num_bytes = tuple.get<1>();
        std::string k_orig_pod  = tuple.get<2>();
        std::string k_read_pod  = tuple.get<3>();
        std::string k_digest    = tuple.get<4>();

        AbcA::ArraySample::Key key;

        key.numBytes = k_num_bytes;
        key.origPOD = Alembic::Util::PODFromName( k_orig_pod );
        key.readPOD = Alembic::Util::PODFromName( k_read_pod );
        hex2bin(key.digest.d, k_digest.c_str());

        //std::string key_str = j_key.asString();

        if (! m_key_pos.count(k_digest))
            m_key_pos[k_digest] = std::vector<size_t>();
        m_key_pos[k_digest].push_back( k_at );
        m_pos_key[k_at] = key;
    }
#endif /* 0 */
}

template <typename T>
GitRepoPtr TypedSampleStore<T>::repo()
{
    if (m_rwmode == READ)
        return m_arimpl_ptr->repo();
    else
        return m_awimpl_ptr->repo();
}

template <typename T>
GitGroupPtr TypedSampleStore<T>::group()
{
    if (m_rwmode == READ)
        return m_arimpl_ptr->group();
    else
        return m_awimpl_ptr->group();
}


AbstractTypedSampleStore* BuildSampleStore( AwImplPtr awimpl_ptr, const AbcA::DataType &iDataType, const AbcA::Dimensions &iDims )
{
    size_t extent = iDataType.getExtent();

    TRACE("BuildSampleStore({W} iDataType pod:" << PODName( iDataType.getPod() ) << " extent:" << extent << ")");
    ABCA_ASSERT( iDataType.getPod() != Alembic::Util::kUnknownPOD && (extent > 0),
                 "Degenerate data type" );

    switch ( iDataType.getPod() )
    {
    case Alembic::Util::kBooleanPOD:
        return new TypedSampleStore<Util::bool_t>( awimpl_ptr, iDataType, iDims ); break;
    case Alembic::Util::kUint8POD:
        return new TypedSampleStore<Util::uint8_t>( awimpl_ptr, iDataType, iDims ); break;
    case Alembic::Util::kInt8POD:
        return new TypedSampleStore<Util::int8_t>( awimpl_ptr, iDataType, iDims ); break;
    case Alembic::Util::kUint16POD:
        return new TypedSampleStore<Util::uint16_t>( awimpl_ptr, iDataType, iDims ); break;
    case Alembic::Util::kInt16POD:
        return new TypedSampleStore<Util::int16_t>( awimpl_ptr, iDataType, iDims ); break;
    case Alembic::Util::kUint32POD:
        return new TypedSampleStore<Util::uint32_t>( awimpl_ptr, iDataType, iDims ); break;
    case Alembic::Util::kInt32POD:
        return new TypedSampleStore<Util::int32_t>( awimpl_ptr, iDataType, iDims ); break;
    case Alembic::Util::kUint64POD:
        return new TypedSampleStore<Util::uint64_t>( awimpl_ptr, iDataType, iDims ); break;
    case Alembic::Util::kInt64POD:
        return new TypedSampleStore<Util::int64_t>( awimpl_ptr, iDataType, iDims ); break;
    case Alembic::Util::kFloat16POD:
        return new TypedSampleStore<Util::float16_t>( awimpl_ptr, iDataType, iDims ); break;
    case Alembic::Util::kFloat32POD:
        return new TypedSampleStore<Util::float32_t>( awimpl_ptr, iDataType, iDims ); break;
    case Alembic::Util::kFloat64POD:
        return new TypedSampleStore<Util::float64_t>( awimpl_ptr, iDataType, iDims ); break;
    case Alembic::Util::kStringPOD:
        return new TypedSampleStore<Util::string>( awimpl_ptr, iDataType, iDims ); break;
    case Alembic::Util::kWstringPOD:
        return new TypedSampleStore<Util::wstring>( awimpl_ptr, iDataType, iDims ); break;
    default:
        ABCA_THROW( "Unknown datatype in BuildSampleStore: " << iDataType );
        break;
    }
    return NULL;
}

AbstractTypedSampleStore* BuildSampleStore( ArImplPtr arimpl_ptr, const AbcA::DataType &iDataType, const AbcA::Dimensions &iDims )
{
    size_t extent = iDataType.getExtent();

    TRACE("BuildSampleStore({R} iDataType pod:" << PODName( iDataType.getPod() ) << " extent:" << extent << ")");
    ABCA_ASSERT( iDataType.getPod() != Alembic::Util::kUnknownPOD && (extent > 0),
                 "Degenerate data type" );

    switch ( iDataType.getPod() )
    {
    case Alembic::Util::kBooleanPOD:
        return new TypedSampleStore<Util::bool_t>( arimpl_ptr, iDataType, iDims ); break;
    case Alembic::Util::kUint8POD:
        return new TypedSampleStore<Util::uint8_t>( arimpl_ptr, iDataType, iDims ); break;
    case Alembic::Util::kInt8POD:
        return new TypedSampleStore<Util::int8_t>( arimpl_ptr, iDataType, iDims ); break;
    case Alembic::Util::kUint16POD:
        return new TypedSampleStore<Util::uint16_t>( arimpl_ptr, iDataType, iDims ); break;
    case Alembic::Util::kInt16POD:
        return new TypedSampleStore<Util::int16_t>( arimpl_ptr, iDataType, iDims ); break;
    case Alembic::Util::kUint32POD:
        return new TypedSampleStore<Util::uint32_t>( arimpl_ptr, iDataType, iDims ); break;
    case Alembic::Util::kInt32POD:
        return new TypedSampleStore<Util::int32_t>( arimpl_ptr, iDataType, iDims ); break;
    case Alembic::Util::kUint64POD:
        return new TypedSampleStore<Util::uint64_t>( arimpl_ptr, iDataType, iDims ); break;
    case Alembic::Util::kInt64POD:
        return new TypedSampleStore<Util::int64_t>( arimpl_ptr, iDataType, iDims ); break;
    case Alembic::Util::kFloat16POD:
        return new TypedSampleStore<Util::float16_t>( arimpl_ptr, iDataType, iDims ); break;
    case Alembic::Util::kFloat32POD:
        return new TypedSampleStore<Util::float32_t>( arimpl_ptr, iDataType, iDims ); break;
    case Alembic::Util::kFloat64POD:
        return new TypedSampleStore<Util::float64_t>( arimpl_ptr, iDataType, iDims ); break;
    case Alembic::Util::kStringPOD:
        return new TypedSampleStore<Util::string>( arimpl_ptr, iDataType, iDims ); break;
    case Alembic::Util::kWstringPOD:
        return new TypedSampleStore<Util::wstring>( arimpl_ptr, iDataType, iDims ); break;
    default:
        ABCA_THROW( "Unknown datatype in BuildSampleStore: " << iDataType );
        break;
    }
    return NULL;
}


} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreOgawa
} // End namespace Alembic
