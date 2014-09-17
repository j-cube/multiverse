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
    if (rank() == 0)
    {
        size_t extent = m_dataType.getExtent();
        for (size_t i = 0; i < extent; ++i)
            addSamplePiece( iSamp[i] );
    } else
    {
        assert( rank() >= 1 );

        size_t extent = m_dataType.getExtent();
        size_t points_per_sample = m_dimensions.numPoints();
        size_t pods_per_sample = points_per_sample * extent;

        for (size_t i = 0; i < pods_per_sample; ++i)
            addSamplePiece( iSamp[i] );
    }
}

template <typename T>
void TypedSampleStore<T>::addSample( const void *iSamp )
{
    const T* iSampT = reinterpret_cast<const T*>(iSamp);
    addSample(iSampT);
}

template <typename T>
void TypedSampleStore<T>::addSample( const AbcA::ArraySample& iSamp )
{
    ABCA_ASSERT( iSamp.getDataType() == m_dataType,
        "DataType on ArraySample iSamp: " << iSamp.getDataType() <<
        ", does not match the DataType of the SampleStore: " <<
        m_dataType );

    addSample( iSamp.getData() );
}

template <typename T>
void TypedSampleStore<T>::setFromPreviousSample()                           // duplicate last added sample
{
    ABCA_ASSERT( m_data.size() > 0,
        "No samples to duplicate in SampleStore" );

    if (rank() == 0)
    {
        size_t extent = m_dataType.getExtent();

        ABCA_ASSERT( m_data.size() > extent,
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

        ABCA_ASSERT( m_data.size() > pods_per_sample,
                      "wrong number of PODs in SampleStore" );

        std::vector<T> last_sample(m_data.end() - pods_per_sample, m_data.end());

        assert(last_sample.size() == pods_per_sample);
        ABCA_ASSERT( last_sample.size() == pods_per_sample,
                      "wrong number of PODs in last sample" );

        m_data.insert( m_data.end(), last_sample.begin(), last_sample.end() );
    }
}

template <typename T>
size_t TypedSampleStore<T>::getNumSamples() const
{
    if (rank() == 0)
    {
        ABCA_ASSERT( (m_data.size() % m_dataType.getExtent()) == 0,
                      "wrong number of PODs in SampleStore" );
        return m_data.size() / m_dataType.getExtent();
    } else
    {
        assert( rank() >= 1 );

        size_t points_per_sample = m_dimensions.numPoints();
        size_t pods_per_sample = points_per_sample * m_dataType.getExtent();

        ABCA_ASSERT( (m_data.size() % pods_per_sample) == 0,
                      "wrong number of PODs in SampleStore" );
        return m_data.size() / pods_per_sample;
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

template <typename T>
Json::Value TypedSampleStore<T>::json() const
{
    Json::Value root( Json::objectValue );

    {
        std::ostringstream ss;
        ss << PODName( m_dataType.getPod() );
        root["typename"] = ss.str();
    }

    {
        std::ostringstream ss;
        ss << m_dataType;
        root["type"] = ss.str();
    }

    assert( m_dataType.getExtent() == extent() );

    root["extent"] = extent();
    root["rank"] = TypedSampleStore<size_t>::JsonFromValue( rank() );

    root["num_samples"] = TypedSampleStore<size_t>::JsonFromValue( getNumSamples() );

    root["dimensions"] = TypedSampleStore<AbcA::Dimensions>::JsonFromValue( getDimensions() );

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

    root["data"] = data;

    return root;
}

template <typename T>
Json::Value TypedSampleStore<T>::JsonFromValue( const T& iValue )
{
    Json::Value data = iValue;
    return data;
}

template <typename T>
T TypedSampleStore<T>::ValueFromJson( const Json::Value& jsonValue )
{
    T value = jsonValue;
    return value;
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
Util::bool_t TypedSampleStore<Util::bool_t>::ValueFromJson( const Json::Value& jsonValue )
{
    return static_cast<Util::bool_t>(jsonValue.asBool());
}

template <>
Json::Value TypedSampleStore<half>::JsonFromValue( const half& iValue )
{
    Json::Value data = static_cast<double>(iValue);
    return data;
}

template <>
half TypedSampleStore<half>::ValueFromJson( const Json::Value& jsonValue )
{
    return static_cast<half>(jsonValue.asDouble());
}

template <>
float TypedSampleStore<float>::ValueFromJson( const Json::Value& jsonValue )
{
    return static_cast<float>(jsonValue.asFloat());
}

template <>
double TypedSampleStore<double>::ValueFromJson( const Json::Value& jsonValue )
{
    return static_cast<double>(jsonValue.asDouble());
}

template <>
Json::Value TypedSampleStore<long>::JsonFromValue( const long& iValue )
{
    Json::Value data = static_cast<Json::Int64>(iValue);
    return data;
}

template <>
long TypedSampleStore<long>::ValueFromJson( const Json::Value& jsonValue )
{
    return static_cast<long>(jsonValue.asInt64());
}

template <>
int TypedSampleStore<int>::ValueFromJson( const Json::Value& jsonValue )
{
    return static_cast<int>(jsonValue.asInt());
}

template <>
Json::Value TypedSampleStore<long unsigned>::JsonFromValue( const long unsigned& iValue )
{
    Json::Value data = static_cast<Json::UInt64>(iValue);
    return data;
}

template <>
long unsigned TypedSampleStore<long unsigned>::ValueFromJson( const Json::Value& jsonValue )
{
    return static_cast<long unsigned>(jsonValue.asUInt64());
}

template <>
unsigned int TypedSampleStore<unsigned int>::ValueFromJson( const Json::Value& jsonValue )
{
    return static_cast<unsigned int>(jsonValue.asUInt());
}

template <>
short TypedSampleStore<short>::ValueFromJson( const Json::Value& jsonValue )
{
    return static_cast<short>(jsonValue.asInt());
}

template <>
unsigned short TypedSampleStore<unsigned short>::ValueFromJson( const Json::Value& jsonValue )
{
    return static_cast<unsigned short>(jsonValue.asUInt());
}

template <>
Json::Value TypedSampleStore<Util::wstring>::JsonFromValue( const Util::wstring& iValue )
{
    std::string s((const char*)&iValue[0], sizeof(wchar_t)/sizeof(char)*iValue.size());
    Json::Value data = s;
    return data;
}

template <>
Util::wstring TypedSampleStore<Util::wstring>::ValueFromJson( const Json::Value& jsonValue )
{
    std::wstringstream wss;
    wss << jsonValue.asString().c_str();
    return wss.str();
}

template <>
Util::string TypedSampleStore<Util::string>::ValueFromJson( const Json::Value& jsonValue )
{
    return jsonValue.asString();
}

template <>
Json::Value TypedSampleStore< wchar_t >::JsonFromValue( const wchar_t& iValue )
{
    Json::Value data = static_cast<int>(iValue);
    return data;
}

template <>
wchar_t TypedSampleStore<wchar_t>::ValueFromJson( const Json::Value& jsonValue )
{
    return static_cast<wchar_t>(jsonValue.asInt());
}

template <>
signed char TypedSampleStore<signed char>::ValueFromJson( const Json::Value& jsonValue )
{
    return static_cast<signed char>(jsonValue.asInt());
}

template <>
unsigned char TypedSampleStore<unsigned char>::ValueFromJson( const Json::Value& jsonValue )
{
    return static_cast<unsigned char>(jsonValue.asUInt());
}

template <typename T>
void TypedSampleStore<T>::fromJson(const Json::Value& root)
{
    TRACE("TypedSampleStore<T>::fromJson()");

    std::string v_typename = root.get("typename", "UNKNOWN").asString();
    std::string v_type = root.get("type", "UNKNOWN").asString();

    uint8_t v_extent = root.get("extent", 0).asUInt();
    size_t v_rank = root.get("rank", 0).asUInt();
    size_t v_num_samples = root.get("num_samples", 0).asUInt();

    Json::Value v_dimensions = root["dimensions"];
    Json::Value v_data = root["data"];

    AbcA::Dimensions dimensions;
    dimensions.setRank( v_dimensions.size() );
    {
        int idx = 0;
        for (Json::Value::iterator it = v_dimensions.begin(); it != v_dimensions.end(); ++it)
        {
            dimensions[idx] = (*it).asUInt64();
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

    for (Json::Value::iterator it = v_data.begin(); it != v_data.end(); ++it)
    {
        if ((*it).isArray())
        {
            Json::Value& sub = *it;
            for (Json::Value::iterator sub_it = sub.begin(); sub_it != sub.end(); ++sub_it)
            {
                m_data.push_back( ValueFromJson(*sub_it) );
            }
        } else
        {
            m_data.push_back( ValueFromJson(*it) );
        }
    }

    ABCA_ASSERT( getNumSamples() == v_num_samples, "wrong number of samples" );
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
