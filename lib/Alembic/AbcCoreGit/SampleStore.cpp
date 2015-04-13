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
void TRACE_VALUE(std::string msg, T v)
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

template <typename T>
TypedSampleStore<T>::TypedSampleStore( const AbcA::DataType &iDataType, const AbcA::Dimensions &iDims )
    : m_dataType(iDataType)
    , m_dimensions(iDims)
{
}

template <typename T>
TypedSampleStore<T>::TypedSampleStore( const void *iData, const AbcA::DataType &iDataType, const AbcA::Dimensions &iDims )
    : m_dataType(iDataType)
    , m_dimensions(iDims)
{
    copyFrom( iData );
}

template <typename T>
TypedSampleStore<T>::~TypedSampleStore()
{
}

template <typename T>
void TypedSampleStore<T>::copyFrom( const std::vector<T>& iData )
{
    m_data = iData;
}

template <typename T>
void TypedSampleStore<T>::copyFrom( const T* iData )
{
    const T *iDataT = reinterpret_cast<const T *>( iData );
    size_t N = m_data.size();
    for ( size_t i = 0; i < N; ++i )
    {
        m_data[i] = iDataT[i];
    }
}

template <typename T>
void TypedSampleStore<T>::copyFrom( const void* iData )
{
    const T *iDataT = reinterpret_cast<const T *>( iData );
    copyFrom( iDataT );
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
void TypedSampleStore<T>::getSamplePieceT( T* iIntoLocation, size_t dataIndex, int index, int subIndex )
{
    Alembic::Util::PlainOldDataType curPod = m_dataType.getPod();

    TRACE( "TypedSampleStore::getSamplePiece(dataIndex:" << dataIndex << " index:" << index << " subIndex:" << subIndex << ")   #bytes:" << PODNumBytes( curPod ) );
    ABCA_ASSERT( (curPod != Alembic::Util::kStringPOD) && (curPod != Alembic::Util::kWstringPOD),
        "Can't convert " << m_dataType <<
        "to non-std::string / non-std::wstring" );

    *iIntoLocation = m_data[dataIndex];
}

template <typename T>
void TypedSampleStore<T>::getSampleT( T* iIntoLocation, int index )
{
    Alembic::Util::PlainOldDataType curPod = m_dataType.getPod();

    TRACE( "TypedSampleStore::getSample() index:" << index << "  #bytes:" << PODNumBytes( curPod ) );
    ABCA_ASSERT( (curPod != Alembic::Util::kStringPOD) && (curPod != Alembic::Util::kWstringPOD),
        "Can't convert " << m_dataType <<
        "to non-std::string / non-std::wstring" );

    if (rank() == 0)
    {
        size_t extent = m_dataType.getExtent();
        size_t baseIndex = (index * extent);
        for (size_t i = 0; i < extent; ++i)
            getSamplePieceT( iIntoLocation + i, baseIndex + i, index, i );
    } else
    {
        assert( rank() >= 1 );

        size_t extent = m_dataType.getExtent();
        size_t points_per_sample = m_dimensions.numPoints();
        size_t pods_per_sample = points_per_sample * extent;
        size_t baseIndex = (index * pods_per_sample);

        for (size_t i = 0; i < pods_per_sample; ++i)
            getSamplePieceT( iIntoLocation + i, baseIndex + i, index, i );
    }
}

template <>
void TypedSampleStore<std::string>::getSampleT( std::string* iIntoLocation, int index )
{
    Alembic::Util::PlainOldDataType curPod = m_dataType.getPod();

    TODO("FIXME!!!");

    ABCA_ASSERT( curPod == Alembic::Util::kStringPOD,
        "Can't convert " << m_dataType <<
        "to std::string" );

    std::string src = m_data[index];
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

    TRACE("SampleStore::addSample(key:'" << key_str << "' #bytes:" << key.numBytes << ")");

    size_t at = m_data.size();
    if (! m_key_pos.count(key_str))
        m_key_pos[key_str] = std::vector<size_t>();
    m_key_pos[key_str].push_back( at );
    m_pos_key[at] = key;

    if (rank() == 0)
    {
        size_t extent = m_dataType.getExtent();

        if (! iSamp)
        {
            for (size_t i = 0; i < extent; ++i)
                addSamplePiece( scalar_traits<T>::Zero() );
        } else
        {
            for (size_t i = 0; i < extent; ++i)
                addSamplePiece( iSamp[i] );
        }
    } else
    {
        assert( rank() >= 1 );

        size_t extent = m_dataType.getExtent();
        size_t points_per_sample = m_dimensions.numPoints();
        size_t pods_per_sample = points_per_sample * extent;

        if (! iSamp)
        {
            for (size_t i = 0; i < pods_per_sample; ++i)
                addSamplePiece( scalar_traits<T>::Zero() );
        } else
        {
            for (size_t i = 0; i < pods_per_sample; ++i)
                addSamplePiece( iSamp[i] );
        }
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
    ABCA_ASSERT( m_data.size() > 0,
        "No samples to duplicate in SampleStore" );

    if (rank() == 0)
    {
        size_t extent = m_dataType.getExtent();

        ABCA_ASSERT( m_data.size() >= extent,
                      "wrong number of PODs in SampleStore" );

        std::vector<T> last_sample(m_data.end() - extent, m_data.end());

        assert(last_sample.size() == extent);
        ABCA_ASSERT( last_sample.size() == extent,
                      "wrong number of PODs in last sample" );

        m_data.insert( m_data.end(), last_sample.begin(), last_sample.end() );
    } else
    {
        assert( rank() >= 1 );

        size_t extent = m_dataType.getExtent();
        size_t points_per_sample = m_dimensions.numPoints();
        size_t pods_per_sample = points_per_sample * extent;

        ABCA_ASSERT( m_data.size() >= pods_per_sample,
                      "wrong number of PODs in SampleStore" );

        std::vector<T> last_sample(m_data.end() - pods_per_sample, m_data.end());

        assert(last_sample.size() == pods_per_sample);
        ABCA_ASSERT( last_sample.size() == pods_per_sample,
                      "wrong number of PODs in last sample" );

        m_data.insert( m_data.end(), last_sample.begin(), last_sample.end() );
    }
}

template <typename T>
bool TypedSampleStore<T>::getKey( AbcA::index_t iSampleIndex, AbcA::ArraySampleKey& oKey )
{
    int dataIndex = sampleIndexToDataIndex( iSampleIndex );

    if (m_pos_key.count(dataIndex))
    {
        oKey = m_pos_key[dataIndex];
        return true;
    }

    return false;
}

template <typename T>
size_t TypedSampleStore<T>::getNumSamples() const
{
    if (rank() == 0)
    {
        size_t data_count = m_data.size();

        if (data_count == 0)
            return 0;

        ABCA_ASSERT( (data_count % m_dataType.getExtent()) == 0,
                      "wrong number of PODs in SampleStore" );
        return data_count / m_dataType.getExtent();
    } else
    {
        assert( rank() >= 1 );

        size_t points_per_sample = m_dimensions.numPoints();
        size_t pods_per_sample = points_per_sample * m_dataType.getExtent();

        size_t data_count = m_data.size();

        if (data_count == 0)
            return 0;

        ABCA_ASSERT( (data_count % pods_per_sample) == 0,
                      "wrong number of PODs in SampleStore" );
        return data_count / pods_per_sample;
    }
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

std::string pod2str(const Alembic::Util::PlainOldDataType& pod)
{
    std::ostringstream ss;
    ss << PODName( pod );
    return ss.str();
}

uint8_t hexchar2int(char input)
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
size_t hex2bin(uint8_t *dst, const char* src)
{
    uint8_t *dst_orig = dst;

    while (*src && src[1])
    {
        *dst      = hexchar2int(*(src++)) << 4;
        *(dst++) |= hexchar2int(*(src++));
    }

    return (dst - dst_orig);
}

// binary serialization
template <typename T>
std::string TypedSampleStore<T>::pack() const
{
    TRACE("TypedSampleStore<T>::pack()");

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

    std::vector<uint64_t> v_dimensions;
    for (size_t i = 0; i < m_dimensions.rank(); ++i )
        v_dimensions.push_back( static_cast<uint64_t>( m_dimensions[i] ) );

    // msgpack::type::tuple< std::string, std::string, int, size_t, size_t, std::vector<uint64_t>, std::vector<T> >
    //     src(s_typename, s_type, extent(), rank(), getNumSamples(), dimensions, m_data);
    // // serialize the object into the buffer.
    // // any classes that implements write(const char*,size_t) can be a buffer.
    // std::stringstream buffer;
    // msgpack::pack(buffer, src);

    std::stringstream buffer;
    msgpack::packer<std::stringstream> pk(&buffer);

    mp_pack(pk, v_typename);
    mp_pack(pk, v_type);
    mp_pack(pk, extent());
    mp_pack(pk, rank());
    mp_pack(pk, getNumSamples());
    mp_pack(pk, v_dimensions);
    mp_pack(pk, m_data);

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

    return buffer.str();
}

template <typename T>
void TypedSampleStore<T>::unpack(const std::string& packed)
{
    TRACE("TypedSampleStore<T>::unpack()");

    msgpack::unpacker pac;

    // copy the buffer data to the unpacker object
    pac.reserve_buffer(packed.size());
    memcpy(pac.buffer(), packed.data(), packed.size());
    pac.buffer_consumed(packed.size());

    // deserialize it.
    msgpack::unpacked msg;

    std::string v_typename;
    std::string v_type;
    int v_extent;
    size_t v_rank;
    size_t v_num_samples;
    std::vector<uint64_t> v_dimensions;
    std::vector<T> v_data;
    size_t v_n_keys;

    // unpack objects in the order in which they were packed

    pac.next(&msg);
    msgpack::object pko = msg.get();
    mp_unpack(pko, v_typename);

    pac.next(&msg);
    pko = msg.get();
    mp_unpack(pko, v_type);

    pac.next(&msg);
    pko = msg.get();
    mp_unpack(pko, v_extent);

    pac.next(&msg);
    pko = msg.get();
    mp_unpack(pko, v_rank);

    pac.next(&msg);
    pko = msg.get();
    mp_unpack(pko, v_num_samples);

    pac.next(&msg);
    pko = msg.get();
    mp_unpack(pko, v_dimensions);

    pac.next(&msg);
    pko = msg.get();
    mp_unpack(pko, v_data);

    // re-initialize using deserialized data (keys done later)

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

    ABCA_ASSERT( dimensions.rank() == v_rank, "wrong dimensions rank" );

    Alembic::Util::PlainOldDataType pod = Alembic::Util::PODFromName( v_typename );
    AbcA::DataType dataType(pod, v_extent);

    ABCA_ASSERT( dataType.getExtent() == v_extent, "wrong datatype extent" );

    m_dataType = dataType;
    m_dimensions = dimensions;

    ABCA_ASSERT( rank() == v_rank, "wrong rank" );
    ABCA_ASSERT( extent() == v_extent, "wrong extent" );

    m_data.clear();
    m_data = v_data;

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
}

AbstractTypedSampleStore* BuildSampleStore( const AbcA::DataType &iDataType, const AbcA::Dimensions &iDims )
{
    size_t extent = iDataType.getExtent();

    TRACE("BuildSampleStore(iDataType pod:" << PODName( iDataType.getPod() ) << " extent:" << extent << ")");
    ABCA_ASSERT( iDataType.getPod() != Alembic::Util::kUnknownPOD && (extent > 0),
                 "Degenerate data type" );
    switch ( iDataType.getPod() )
    {
    case Alembic::Util::kBooleanPOD:
        return new TypedSampleStore<Util::bool_t>( iDataType, iDims ); break;
    case Alembic::Util::kUint8POD:
        return new TypedSampleStore<Util::uint8_t>( iDataType, iDims ); break;
    case Alembic::Util::kInt8POD:
        return new TypedSampleStore<Util::int8_t>( iDataType, iDims ); break;
    case Alembic::Util::kUint16POD:
        return new TypedSampleStore<Util::uint16_t>( iDataType, iDims ); break;
    case Alembic::Util::kInt16POD:
        return new TypedSampleStore<Util::int16_t>( iDataType, iDims ); break;
    case Alembic::Util::kUint32POD:
        return new TypedSampleStore<Util::uint32_t>( iDataType, iDims ); break;
    case Alembic::Util::kInt32POD:
        return new TypedSampleStore<Util::int32_t>( iDataType, iDims ); break;
    case Alembic::Util::kUint64POD:
        return new TypedSampleStore<Util::uint64_t>( iDataType, iDims ); break;
    case Alembic::Util::kInt64POD:
        return new TypedSampleStore<Util::int64_t>( iDataType, iDims ); break;
    case Alembic::Util::kFloat16POD:
        return new TypedSampleStore<Util::float16_t>( iDataType, iDims ); break;
    case Alembic::Util::kFloat32POD:
        return new TypedSampleStore<Util::float32_t>( iDataType, iDims ); break;
    case Alembic::Util::kFloat64POD:
        return new TypedSampleStore<Util::float64_t>( iDataType, iDims ); break;
    case Alembic::Util::kStringPOD:
        return new TypedSampleStore<Util::string>( iDataType, iDims ); break;
    case Alembic::Util::kWstringPOD:
        return new TypedSampleStore<Util::wstring>( iDataType, iDims ); break;
    default:
        ABCA_THROW( "Unknown datatype in BuildSampleStore: " << iDataType );
        break;
    }
    return NULL;
}


} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreOgawa
} // End namespace Alembic
