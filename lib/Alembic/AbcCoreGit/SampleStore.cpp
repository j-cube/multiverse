//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

#include <Alembic/AbcCoreGit/SampleStore.h>
#include <Alembic/AbcCoreGit/Utils.h>


namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

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
void TypedSampleStore<T>::addSample( const T* iSamp )
{
    size_t extent = m_dataType.getExtent();
    for (size_t i = 0; i < extent; ++i)
        addSamplePiece( iSamp[i] );
}

template <typename T>
void TypedSampleStore<T>::addSample( const void *iSamp )
{
    const T* iSampT = reinterpret_cast<const T*>(iSamp);
    addSample(iSampT);
}

template <typename T>
void TypedSampleStore<T>::setFromPreviousSample()                           // duplicate last added sample
{
    ABCA_ASSERT( m_data.size() > 0,
        "No samples to duplicate in SampleStore" );

    size_t extent = m_dataType.getExtent();

    ABCA_ASSERT( m_data.size() > extent,
                  "wrong number of PODs in SampleStore" );

    std::vector<T> last_sample(m_data.end() - extent, m_data.end());

    assert(last_sample.size() == extent);
    ABCA_ASSERT( last_sample.size() == extent,
                  "wrong number of PODs in last sample" );

    m_data.insert( m_data.end(), last_sample.begin(), last_sample.end() );
}

template <typename T>
size_t TypedSampleStore<T>::getNumSamples() const
{
    ABCA_ASSERT( (m_data.size() % m_dataType.getExtent()) == 0,
                  "wrong number of PODs in SampleStore" );
    return m_data.size() / m_dataType.getExtent();
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

template <typename T>
Json::Value TypedSampleStore<T>::json() const
{
    Json::Value data( Json::arrayValue );

    size_t extent = m_dataType.getExtent();

    if (extent == 1)
    {
        typename std::vector<T>::const_iterator it;
        for (it = m_data.begin(); it != m_data.end(); ++it)
        {
            data.append( JsonFromValue(*it) );
        }
    } else
    {
        ABCA_ASSERT( extent > 1, "wrong extent" );

        Json::Value arr( Json::arrayValue );
        size_t count = 0;

        typename std::vector<T>::const_iterator it;
        for (it = m_data.begin(); it != m_data.end(); ++it)
        {
            arr.append( JsonFromValue(*it) );
            count++;

            if (count >= extent)
            {
                data.append( arr );
                arr = Json::Value( Json::arrayValue );
                count = 0;
            }
        }

        if (count >= 1)
        {
            data.append( arr );
            arr = Json::Value( Json::arrayValue );
            count = 0;
        }
    }

    return data;
}

template <typename T>
Json::Value TypedSampleStore<T>::JsonFromValue( const T& iValue )
{
    Json::Value data = iValue;
    return data;
}

template <>
Json::Value TypedSampleStore<AbcA::Dimensions>::JsonFromValue( const AbcA::Dimensions& iDims )
{
    Json::Value data( Json::arrayValue );
    for ( size_t i = 0; i < iDims.rank(); ++i )
        data.append( static_cast<Json::Value::UInt64>( iDims[i] ) );
    return data;
}

template <>
Json::Value TypedSampleStore<Util::bool_t>::JsonFromValue( const Util::bool_t& iValue )
{
    Json::Value data = iValue.asBool();
    return data;
}

template <>
Json::Value TypedSampleStore<half>::JsonFromValue( const half& iValue )
{
    Json::Value data = static_cast<double>(iValue);
    return data;
}

template <>
Json::Value TypedSampleStore<long>::JsonFromValue( const long& iValue )
{
    Json::Value data = static_cast<Json::Int64>(iValue);
    return data;
}

template <>
Json::Value TypedSampleStore<long unsigned>::JsonFromValue( const long unsigned& iValue )
{
    Json::Value data = static_cast<Json::UInt64>(iValue);
    return data;
}

template <>
Json::Value TypedSampleStore<Util::wstring>::JsonFromValue( const Util::wstring& iValue )
{
    std::string s((const char*)&iValue[0], sizeof(wchar_t)/sizeof(char)*iValue.size());
    Json::Value data = s;
    return data;
}

template <>
Json::Value TypedSampleStore< wchar_t >::JsonFromValue( const wchar_t& iValue )
{
    Json::Value data = static_cast<int>(iValue);
    return data;
}

AbstractTypedSampleStore* BuildSampleStore( const AbcA::DataType &iDataType, const AbcA::Dimensions &iDims )
{
    size_t extent = iDataType.getExtent();

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
