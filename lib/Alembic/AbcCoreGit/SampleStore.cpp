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
void TypedSampleStore<T>::getSampleT( T* iIntoLocation, int index )
{
    Alembic::Util::PlainOldDataType curPod = m_dataType.getPod();

    TRACE( "TypedSampleStore::getSample() index:" << index << "  #bytes:" << PODNumBytes( curPod ) << " dataType:" << m_dataType << " T:" << GetTypeStr<T>());

    ABCA_ASSERT( hasIndex(index),
        "Requested sample index not present (index:" << index << ")" );

    ABCA_ASSERT( (curPod != Alembic::Util::kStringPOD) && (curPod != Alembic::Util::kWstringPOD),
        "Can't convert " << m_dataType <<
        " to non-std::string / non-std::wstring" );

    // TODO: optimize skipping the key, by using a key/value index instead:
    //       sample index -> key-index -> value
    size_t kid = sampleIndexToKid(index);
    // TRACE("sampleIndex:" << index << " kid:" << kid);
    const std::vector<T>& sampleData = kidToSampleData(kid);
    // TRACE("got sample data");
    // const std::vector<T>& sampleData = sampleIndexToSampleData(index);

    ABCA_ASSERT( hasDimensions(kid),
        "Can't obtain dimension info for sample with index:" << index <<
        " (kid:" << kid << ")" );
    const AbcA::Dimensions dims = getDimensions(kid);

    if (dims.rank() == 0)
    {
        size_t extent = m_dataType.getExtent();

        if (extent > sampleData.size())
        {
            TRACE("extent:" << extent << " sampleData.size:" << sampleData.size() << " kid:" << kid << " index:" << index);
        }
        assert(extent <= sampleData.size());

        for (size_t i = 0; i < extent; ++i)
            iIntoLocation[i] = sampleData[i];
    } else
    {
        assert( dims.rank() >= 1 );

        size_t extent = m_dataType.getExtent();
        size_t points_per_sample = dims.numPoints();
        size_t pods_per_sample = points_per_sample * extent;

        for (size_t i = 0; i < pods_per_sample; ++i)
            iIntoLocation[i] = sampleData[i];
    }
}

template <>
void TypedSampleStore<std::string>::getSampleT( std::string* iIntoLocation, int index )
{
    Alembic::Util::PlainOldDataType curPod = dataType().getPod();

    ABCA_ASSERT( hasIndex(index),
        "Requested sample index not present (index:" << index << ")" );

    ABCA_ASSERT( curPod == Alembic::Util::kStringPOD,
        "Can't convert " << dataType() <<
        " to std::string" );

    const std::vector<std::string>& sampleData = sampleIndexToSampleData(index);

    std::vector<std::string>::const_iterator it;

    std::string * strPtrStart =
        reinterpret_cast< std::string * > ( iIntoLocation );

    size_t idx = 0;
    for (it = sampleData.begin(); it != sampleData.end(); ++it)
    {
        const std::string& src = (*it);
        // std::string src = m_data[index];
        std::string * strPtr = strPtrStart + idx;

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

        idx++;
    }
}

template <>
void TypedSampleStore<std::wstring>::getSampleT( std::wstring* iIntoLocation, int index )
{
    Alembic::Util::PlainOldDataType curPod = dataType().getPod();

    ABCA_ASSERT( hasIndex(index),
        "Requested sample index not present (index:" << index << ")" );

    ABCA_ASSERT( curPod == Alembic::Util::kWstringPOD,
        "Can't convert " << dataType() <<
        " to std::wstring" );

    const std::vector<std::wstring>& sampleData = sampleIndexToSampleData(index);

    std::vector<std::wstring>::const_iterator it;

    std::wstring * strPtrStart =
        reinterpret_cast< std::wstring * > ( iIntoLocation );

    size_t idx = 0;
    for (it = sampleData.begin(); it != sampleData.end(); ++it)
    {
        const std::wstring& src = (*it);
        // std::wstring src = m_data[index];
        std::wstring * strPtr = strPtrStart + idx;

        size_t numChars = src.size() + 1;
        wchar_t * buf = new wchar_t[ numChars + 1 ];
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

        idx++;
    }
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
    ABCA_ASSERT( hasIndex(index),
        "Requested sample index not present (index:" << index << ")" );

    // how much space do we need?
    // determine sample dimensions to know
    size_t kid = sampleIndexToKid(index);
    ABCA_ASSERT( hasDimensions(kid),
        "Can't obtain dimension info for sample with index:" << index <<
        " (kid:" << kid << ")" );
    const AbcA::Dimensions dims = getDimensions(kid);

    // Util::Dimensions dims = getDimensions();

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
size_t TypedSampleStore<T>::adjustKey( const T* iSamp, AbcA::ArraySample::Key& key, const AbcA::Dimensions& dims )
{
    return key.numBytes;
}

template <>
size_t TypedSampleStore<Util::string>::adjustKey( const Util::string* iSamp, AbcA::ArraySample::Key& key, const AbcA::Dimensions& dims )
{
    ABCA_ASSERT(m_dataType.getPod() == Alembic::Util::kStringPOD, "wrong POD");

    // re-compute key size, since by default it's fsck-ed up for strings...
    size_t extent = m_dataType.getExtent();
    size_t pods_per_sample;

    size_t sample_size = 0;
    if (dims.rank() == 0)
    {
        pods_per_sample = extent;
    } else
    {
        assert( dims.rank() >= 1 );

        size_t points_per_sample = dims.numPoints();
        pods_per_sample = points_per_sample * extent;

        for (size_t i = 0; i < pods_per_sample; ++i)
        {
            const Util::string* sPtr = &(iSamp[i]);
            const Util::string& s = *sPtr;
            sample_size += s.size() + 1;
        }
    }

    key.numBytes = sample_size;

    return sample_size;
}

template <>
size_t TypedSampleStore<Util::wstring>::adjustKey( const Util::wstring* iSamp, AbcA::ArraySample::Key& key, const AbcA::Dimensions& dims )
{
    ABCA_ASSERT(m_dataType.getPod() == Alembic::Util::kWstringPOD, "wrong POD");

    // re-compute key size, since by default it's fsck-ed up for strings...
    size_t extent = m_dataType.getExtent();
    size_t pods_per_sample;

    size_t sample_size = 0;
    if (dims.rank() == 0)
    {
        pods_per_sample = extent;
    } else
    {
        assert( dims.rank() >= 1 );

        size_t points_per_sample = dims.numPoints();
        pods_per_sample = points_per_sample * extent;

        for (size_t i = 0; i < pods_per_sample; ++i)
        {
            const Util::wstring* sPtr = &(iSamp[i]);
            const Util::wstring& s = *sPtr;
            sample_size += s.size() + 1;
        }
    }

    sample_size *= sizeof(wchar_t);

    key.numBytes = sample_size;

    return sample_size;
}

template <typename T>
void TypedSampleStore<T>::checkSamplePieceForAdd( const T& iSamp, const AbcA::ArraySample::Key& key, const AbcA::Dimensions& dims )
{
}

template <>
void TypedSampleStore<std::string>::checkSamplePieceForAdd( const std::string& iSamp, const AbcA::ArraySample::Key& key, const AbcA::Dimensions& dims )
{
    ABCA_ASSERT( iSamp.find( '\0' ) == std::string::npos,
        "string contains NUL character" );

    // TRACE("adding a string '" << iSamp << "'");
}

template <>
void TypedSampleStore<std::wstring>::checkSamplePieceForAdd( const std::wstring& iSamp, const AbcA::ArraySample::Key& key, const AbcA::Dimensions& dims )
{
    ABCA_ASSERT( iSamp.find( L'\0' ) == std::wstring::npos,
        "string contains NUL character" );
}

template <typename T>
size_t TypedSampleStore<T>::addSample( const T* iSamp, AbcA::ArraySample::Key& key, const AbcA::Dimensions& dims )
{
    // before altering our data (next index, ...), check
    // the validity of the sample material
    if (dims.rank() == 0)
    {
        size_t extent = m_dataType.getExtent();

        if (iSamp)
        {
            for (size_t i = 0; i < extent; ++i)
                checkSamplePieceForAdd( iSamp[i], key, dims );
        }
    } else
    {
        assert( dims.rank() >= 1 );

        size_t extent = m_dataType.getExtent();
        size_t points_per_sample = dims.numPoints();
        size_t pods_per_sample = points_per_sample * extent;

        if (iSamp)
        {
            for (size_t i = 0; i < pods_per_sample; ++i)
                checkSamplePieceForAdd( iSamp[i], key, dims );
        }
    }

    // then do our magic...

    adjustKey(iSamp, key, dims);

    std::string key_str = key.digest.str();

    size_t at = m_next_index++;
    size_t kid = addKey(key);
    if (! m_kid_indexes.count(kid))
        m_kid_indexes[kid] = std::vector<size_t>();
    m_kid_indexes[kid].push_back( at );
    m_index_to_kid[at] = kid;
    // TRACE("set index_to_kid[" << at << "] := " << kid);

    if (! hasDimensions(kid))
    {
        setDimensions(kid, dims);
    }

    TRACE("SampleStore::addSample(key:'" << key_str << "' #bytes:" << key.numBytes << " kid:" << kid << " index:=" << at << ")  T:" << GetTypeStr<T>() << " dataType:" << m_dataType << " dims:" << dims);

    if (! ks()->hasData(kid))
    {
        // m_kid_to_data[kid] = std::vector<T>();
        // std::vector<T>& data = m_kid_to_data[kid];
        std::vector<T> data;

        if (dims.rank() == 0)
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
            assert( dims.rank() >= 1 );

            size_t extent = m_dataType.getExtent();
            size_t points_per_sample = dims.numPoints();
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

        addData(kid, key, data);
    }

    return at;
}

template <typename T>
size_t TypedSampleStore<T>::addSample( const T* iSamp, const AbcA::ArraySample::Key& origKey, const AbcA::Dimensions& dims )
{
    AbcA::ArraySample::Key key = origKey;

    return addSample(iSamp, key, dims);
}

template <typename T>
size_t TypedSampleStore<T>::addSample( const void *iSamp, const AbcA::ArraySample::Key& key, const AbcA::Dimensions& dims )
{
    const T* iSampT = reinterpret_cast<const T*>(iSamp);
    return addSample(iSampT, key, dims);
}

template <typename T>
size_t TypedSampleStore<T>::addSample( const AbcA::ArraySample& iSamp )
{
    ABCA_ASSERT( iSamp.getDataType() == m_dataType,
        "DataType on ArraySample iSamp: " << iSamp.getDataType() <<
        ", does not match the DataType of the SampleStore: " <<
        m_dataType );

    AbcA::ArraySample::Key key = iSamp.getKey();
    AbcA::Dimensions dims = iSamp.getDimensions();

    return addSample( iSamp.getData(), key, dims );
}

template <typename T>
size_t TypedSampleStore<T>::setFromPreviousSample()                           // duplicate last added sample
{
    size_t previousSampleIndex = m_next_index - 1;

    ABCA_ASSERT( previousSampleIndex >= 0,
        "No samples to duplicate in SampleStore" );

    size_t kid = sampleIndexToKid(previousSampleIndex);
    size_t at = m_next_index++;

    m_kid_indexes[kid].push_back( at );
    m_index_to_kid[at] = kid;
    // TRACE("set index_to_kid[" << at << "] := " << kid);

    return at;
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

    // std::string v_typename;
    // {
    //     std::ostringstream ss;
    //     ss << PODName( m_dataType.getPod() );
    //     v_typename = ss.str();
    // }

    std::string v_type;
    {
        std::ostringstream ss;
        ss << m_dataType;
        v_type = ss.str();
    }

    std::stringstream buffer;
    msgpack::packer<std::stringstream> pk(&buffer);

    // mp_pack(pk, v_typename);
    mp_pack(pk, v_type);        // not necessary actually (redundant, given m_dataType)
    // mp_pack(pk, extent());      // not necessary actually (redundant, given m_dataType)
    mp_pack(pk, m_dataType);
    mp_pack(pk, getNumSamples());
    mp_pack(pk, m_dimensions);
    // TRACE("saved dimensions of rank " << m_dimensions.rank());
    // for (size_t i = 0; i < m_dimensions.rank(); ++i )
    //     TRACE("dim[" << i << "] = " << m_dimensions[i]);

    // save index->kid map
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
        }
        mp_pack(pk, m_next_index);
    }

    // save kid->dimensions map
    {
        TRACE("serializing " << m_kid_dims.size() << " dimensions");
        mp_pack(pk, static_cast<size_t>(m_kid_dims.size()));
        std::map< size_t, AbcA::Dimensions >::const_iterator p_it;
        for (p_it = m_kid_dims.begin(); p_it != m_kid_dims.end(); ++p_it)
        {
            size_t kid                   = (*p_it).first;
            const AbcA::Dimensions& dims = (*p_it).second;

            mp_pack(pk, kid);
            mp_pack(pk, dims);
            // TRACE("serialized kid_to_dimensions[" << kid << "] == " << dims);
        }
    }

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
    size_t v_num_samples;
    AbcA::Dimensions dimensions;

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
    mp_unpack(pko, v_num_samples);

    pac.next(&msg);
    pko = msg.get();
    mp_unpack(pko, dimensions);
    // TRACE("got dimensions of rank " << dimensions.rank());
    // for (size_t i = 0; i < dimensions.rank(); ++i )
    //     TRACE("dim[" << i << "] = " << dimensions[i]);

    // re-initialize using deserialized data

    // ABCA_ASSERT( dataType.getExtent() == v_extent, "wrong datatype extent" );

    m_dataType = dataType;
    m_dimensions = dimensions;

    // ABCA_ASSERT( extent() == v_extent, "wrong extent" );

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
    }
    pac.next(&msg);
    pko = msg.get();
    mp_unpack(pko, m_next_index);

    // deserialize kid->dimensions map
    m_kid_dims.clear();
    {
        size_t v_n_dims = 0;
        pac.next(&msg);
        pko = msg.get();
        mp_unpack(pko, v_n_dims);
        TRACE("deserializing " << v_n_dims << " dimensions");
        for (size_t i = 0; i < v_n_dims; ++i)
        {
            size_t kid;
            AbcA::Dimensions dims;

            pac.next(&msg);
            pko = msg.get();
            mp_unpack(pko, kid);

            pac.next(&msg);
            pko = msg.get();
            mp_unpack(pko, dims);

            m_kid_dims[kid] = dims;
            // TRACE("deserialized kid_to_dimensions[" << kid << "] := " << dims);
        }
    }
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
