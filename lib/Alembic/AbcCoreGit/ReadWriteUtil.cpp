//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

#include <Alembic/AbcCoreGit/ReadWriteUtil.h>
#include <Alembic/AbcCoreGit/AwImpl.h>
#include <Alembic/AbcCoreGit/Git.h>

#include <Alembic/AbcCoreGit/JSON.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
void pushUint32WithHint( std::vector< Util::uint8_t > & ioData,
                         Util::uint32_t iVal, Util::uint32_t iHint )
{
    Util::uint8_t * data = ( Util::uint8_t * ) &iVal;
    if ( iHint == 0)
    {
        ioData.push_back( data[0] );
    }
    else if ( iHint == 1 )
    {
        ioData.push_back( data[0] );
        ioData.push_back( data[1] );
    }
    else if ( iHint == 2 )
    {
        ioData.push_back( data[0] );
        ioData.push_back( data[1] );
        ioData.push_back( data[2] );
        ioData.push_back( data[3] );
    }
}

//-*****************************************************************************
void pushChrono( std::vector< Util::uint8_t > & ioData, chrono_t iVal )
{
    Util::uint8_t * data = ( Util::uint8_t * ) &iVal;

    for ( std::size_t i = 0; i < sizeof( chrono_t ); ++i )
    {
        ioData.push_back( data[i] );
    }
}

//-*****************************************************************************
void HashPropertyHeader( const AbcA::PropertyHeader & iHeader,
                         Util::SpookyHash & ioHash )
{

    // turn our header into something to add to our hash
    std::vector< Util::uint8_t > data;

    data.insert( data.end(), iHeader.getName().begin(),
                 iHeader.getName().end() );

    std::string metaData = iHeader.getMetaData().serialize();
    data.insert( data.end(), metaData.begin(), metaData.end() );

    if ( !iHeader.isCompound() )
    {
        data.push_back( ( Util::uint8_t )iHeader.getDataType().getPod() );
        data.push_back( iHeader.getDataType().getExtent() );

        // toss this in to differentiate between array and scalar props that
        // have no samples
        if ( iHeader.isScalar() )
        {
            data.push_back( 0 );
        }

        AbcA::TimeSamplingPtr ts = iHeader.getTimeSampling();
        AbcA::TimeSamplingType tst = ts->getTimeSamplingType();

        chrono_t tpc = tst.getTimePerCycle();

        pushChrono( data, tpc );

        const std::vector < chrono_t > & samps = ts->getStoredTimes();

        Util::uint32_t spc = ( Util::uint32_t ) samps.size();

        pushUint32WithHint( data, spc, 2 );

        for ( std::size_t i = 0; i < samps.size(); ++i )
        {
            pushChrono( data, samps[i] );
        }
    }

    if ( !data.empty() )
    {
        ioHash.Update( &data.front(), data.size() );
    }
}

//-*****************************************************************************
void HashDimensions( const AbcA::Dimensions & iDims,
                     Util::Digest & ioHash )
{
    size_t rank = iDims.rank();

    if ( rank > 0 )
    {
        Util::SpookyHash hash;
        hash.Init(0, 0);

        hash.Update( iDims.rootPtr(), rank * 8 );
        hash.Update( ioHash.d, 16 );

        Util::uint64_t hash0, hash1;
        hash.Final( &hash0, &hash1 );
        ioHash.words[0] = hash0;
        ioHash.words[1] = hash1;
    }
}

//-*****************************************************************************
WrittenSampleMap &
GetWrittenSampleMap( AbcA::ArchiveWriterPtr iVal )
{
    AwImpl *ptr = dynamic_cast<AwImpl*>( iVal.get() );
    ABCA_ASSERT( ptr, "NULL Impl Ptr" );
    return ptr->getWrittenSampleMap();
}

WrittenSampleIDPtr getWrittenSampleID( WrittenSampleMap &iMap,
                                       const AbcA::ArraySample &iSamp,
                                       const AbcA::ArraySample::Key &iKey,
                                       size_t iWhere )
{
    // See whether or not we've already stored this.
    WrittenSampleIDPtr writeID = iMap.find( iKey );
    if (writeID)
        return writeID;

    const AbcA::Dimensions &dims = iSamp.getDimensions();
    const AbcA::DataType &dataType = iSamp.getDataType();

    writeID.reset( new WrittenSampleID(iKey, iWhere,
                        dataType.getExtent() * dims.numPoints()) );
    iMap.store( writeID );

    // Return the reference.
    return writeID;
}

//-*****************************************************************************
void WritePropertyInfo( std::vector< Util::uint8_t > & ioData,
                    const AbcA::PropertyHeader &iHeader,
                    bool isScalarLike,
                    bool isHomogenous,
                    Util::uint32_t iTimeSamplingIndex,
                    Util::uint32_t iNumSamples,
                    Util::uint32_t iFirstChangedIndex,
                    Util::uint32_t iLastChangedIndex,
                    MetaDataMapPtr iMap )
{

    Util::uint32_t info = 0;

    // 0000 0000 0000 0000 0000 0000 0000 0011
    static const Util::uint32_t ptypeMask = 0x0003;

    // 0000 0000 0000 0000 0000 0000 0000 1100
    static const Util::uint32_t sizeHintMask = 0x000c;

    // 0000 0000 0000 0000 0000 0000 1111 0000
    static const Util::uint32_t podMask = 0x00f0;

    // 0000 0000 0000 0000 0000 0001 0000 0000
    static const Util::uint32_t hasTsidxMask = 0x0100;

    // 0000 0000 0000 0000 0000 0010 0000 0000
    static const Util::uint32_t needsFirstLastMask = 0x0200;

    // 0000 0000 0000 0000 0000 0100 0000 0000
    static const Util::uint32_t homogenousMask = 0x400;

    // 0000 0000 0000 0000 0000 1000 0000 0000
    static const Util::uint32_t constantMask = 0x800;

    // 0000 0000 0000 1111 1111 0000 0000 0000
    static const Util::uint32_t extentMask = 0xff000;

    // 0000 1111 1111 0000 0000 0000 0000 0000
    static const Util::uint32_t metaDataIndexMask = 0xff00000;

    std::string metaData = iHeader.getMetaData().serialize();
    Util::uint32_t metaDataSize = metaData.size();

    // keep track of the longest thing for byteSizeHint and so we don't
    // have to use 4 byte sizes if we don't need it
    Util::uint32_t maxSize = metaDataSize;

    Util::uint32_t nameSize = iHeader.getName().size();
    if ( maxSize < nameSize )
    {
        maxSize = nameSize;
    }

    if ( maxSize < iNumSamples )
    {
        maxSize = iNumSamples;
    }

    if ( maxSize < iTimeSamplingIndex )
    {
        maxSize = iTimeSamplingIndex;
    }

    // size hint helps determine how many bytes are enough to store the
    // various string size, num samples and time sample indices
    // 0 for 1 byte
    // 1 for 2 bytes
    // 2 for 4 bytes
    Util::uint32_t sizeHint = 0;
    if ( maxSize > 255 && maxSize < 65536 )
    {
        sizeHint = 1;
    }
    else if ( maxSize >= 65536)
    {
        sizeHint = 2;
    }

    info |= sizeHintMask & ( sizeHint << 2 );

    Util::uint32_t metaDataIndex = iMap->getIndex( metaData );

    info |= metaDataIndexMask & ( metaDataIndex << 20 );

    // compounds are treated differently
    if ( !iHeader.isCompound() )
    {
        // Slam the property type in there.
        info |= ptypeMask & ( Util::uint32_t )iHeader.getPropertyType();

        // arrays may be scalar like, scalars are already scalar like
        info |= ( Util::uint32_t ) isScalarLike;

        Util::uint32_t pod = ( Util::uint32_t )iHeader.getDataType().getPod();
        info |= podMask & ( pod << 4 );

        if (iTimeSamplingIndex != 0)
        {
            info |= hasTsidxMask;
        }

        bool needsFirstLast = false;

        if (iFirstChangedIndex == 0 && iLastChangedIndex == 0)
        {
            info |= constantMask;
        }
        else if (iFirstChangedIndex != 1 ||
                 iLastChangedIndex != iNumSamples - 1)
        {
            info |= needsFirstLastMask;
            needsFirstLast = true;
        }

        Util::uint32_t extent =
            ( Util::uint32_t )iHeader.getDataType().getExtent();
        info |= extentMask & ( extent << 12 );

        if ( isHomogenous )
        {
            info |= homogenousMask;
        }

        ABCA_ASSERT( iFirstChangedIndex <= iNumSamples &&
            iLastChangedIndex <= iNumSamples &&
            iFirstChangedIndex <= iLastChangedIndex,
            "Illegal Sampling!" << std::endl <<
            "Num Samples: " << iNumSamples << std::endl <<
            "First Changed Index: " << iFirstChangedIndex << std::endl <<
            "Last Changed Index: " << iLastChangedIndex << std::endl );

        // info is always 4 bytes so use hint 2
        pushUint32WithHint( ioData, info, 2 );

        pushUint32WithHint( ioData, iNumSamples, sizeHint );

        // don't bother writing out first and last change if every sample
        // was different
        if ( needsFirstLast )
        {
            pushUint32WithHint( ioData, iFirstChangedIndex, sizeHint );
            pushUint32WithHint( ioData, iLastChangedIndex, sizeHint );
        }

        // finally set time sampling index on the end if necessary
        if (iTimeSamplingIndex != 0)
        {
            pushUint32WithHint( ioData, iTimeSamplingIndex, sizeHint );
        }

    }
    // compound, shove in just info (hint is 2)
    else
    {
        pushUint32WithHint( ioData, info, 2 );
    }

    pushUint32WithHint( ioData, nameSize, sizeHint );

    ioData.insert( ioData.end(), iHeader.getName().begin(),
                   iHeader.getName().end() );

    if ( metaDataIndex == 0xff )
    {
        pushUint32WithHint( ioData, metaDataSize, sizeHint );

        if ( metaDataSize )
        {
            ioData.insert( ioData.end(), metaData.begin(), metaData.end() );
        }
    }

}

//-*****************************************************************************
void WriteObjectHeader( std::vector< Util::uint8_t > & ioData,
                    const AbcA::ObjectHeader &iHeader,
                    MetaDataMapPtr iMap )
{
    Util::uint32_t nameSize = iHeader.getName().size();
    pushUint32WithHint( ioData, nameSize, 2 );
    ioData.insert( ioData.end(), iHeader.getName().begin(),
                   iHeader.getName().end() );

    std::string metaData = iHeader.getMetaData().serialize();
    Util::uint32_t metaDataSize = ( Util::uint32_t ) metaData.size();

    Util::uint32_t metaDataIndex = iMap->getIndex( metaData );

    // write 1 byte for the meta data index
    pushUint32WithHint( ioData, metaDataIndex, 0 );

    // write the size and meta data IF necessary
    if ( metaDataIndex == 0xff )
    {
        pushUint32WithHint( ioData, metaDataSize, 2 );
        if ( metaDataSize )
        {
            ioData.insert( ioData.end(), metaData.begin(), metaData.end() );
        }
    }
}

#ifdef OBSOLETE
//-*****************************************************************************
void WriteTimeSampling( std::vector< Util::uint8_t > & ioData,
                    Util::uint32_t  iMaxSample,
                    const AbcA::TimeSampling &iTsmp )
{
    pushUint32WithHint( ioData, iMaxSample, 2 );

    AbcA::TimeSamplingType tst = iTsmp.getTimeSamplingType();

    chrono_t tpc = tst.getTimePerCycle();

    pushChrono( ioData, tpc );

    const std::vector < chrono_t > & samps = iTsmp.getStoredTimes();
    ABCA_ASSERT( samps.size() > 0, "No TimeSamples to write!");

    Util::uint32_t spc = ( Util::uint32_t ) samps.size();

    pushUint32WithHint( ioData, spc, 2 );

    for ( std::size_t i = 0; i < samps.size(); ++i )
    {
        pushChrono( ioData, samps[i] );
    }

}
#endif /* OBSOLETE */

void toJson( rapidjson::Document& document, rapidjson::Value& dst,
             const AbcA::TimeSamplingType& value )
{
    dst.SetObject();

    JsonSet(document, dst, "uniform", value.isUniform());
    JsonSet(document, dst, "cyclic", value.isCyclic());
    JsonSet(document, dst, "acyclic", value.isAcyclic());
    JsonSet(document, dst, "samplesPerCycle", value.getNumSamplesPerCycle());
    JsonSet(document, dst, "timePerCycle", value.getTimePerCycle());
}

static bool fromJson( AbcA::TimeSamplingType& result, const rapidjson::Value& jsonValue )
{
    bool isUniform = JsonGetBool(jsonValue, "uniform").get_value_or(false);
    bool isCyclic  = JsonGetBool(jsonValue, "cyclic").get_value_or(false);
    bool isAcyclic = JsonGetBool(jsonValue, "acyclic").get_value_or(false);
    uint32_t spc = JsonGetUint(jsonValue, "samplesPerCycle").get_value_or(0);
    chrono_t tpc = JsonGetDouble(jsonValue, "timePerCycle").get_value_or(0.0);

    TRACE("isUniform:" << (isUniform ? "T" : "F") << " isCyclic:" << (isCyclic ? "T" : "F") << " isAcyclic:" << (isAcyclic ? "T" : "F") << " spc:" << spc << " tpc:" << tpc);
    if (isAcyclic)
    {
        result = AbcA::TimeSamplingType(AbcA::TimeSamplingType::kAcyclic);
        return true;
    } else if (isCyclic)
    {
        result = AbcA::TimeSamplingType(spc, tpc);
        return true;
    } else if (isUniform)
    {
        result = AbcA::TimeSamplingType(tpc);
        return true;
    } else
    {
        result = AbcA::TimeSamplingType();
        return true;
    }

    return false;
}


void toJson( rapidjson::Document& document, rapidjson::Value& dst,
             const AbcA::TimeSampling& value )
{
    rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

    dst.SetObject();

    rapidjson::Value tstObject( rapidjson::kObjectType );
    toJson(document, tstObject, value.getTimeSamplingType());
    JsonSet(document, dst, "tst", tstObject);

    const std::vector < chrono_t > & samps = value.getStoredTimes();
    ABCA_ASSERT( samps.size() > 0, "No TimeSamples to convert!");

    Util::uint32_t spc = ( Util::uint32_t ) samps.size();

    JsonSet(document, dst, "n_times", spc);

    rapidjson::Value values( rapidjson::kArrayType );

    for ( std::size_t i = 0; i < samps.size(); ++i )
    {
        values.PushBack( samps[i], allocator );
    }

    JsonSet(document, dst, "times", values);
}

static bool fromJson( AbcA::TimeSampling& result, const rapidjson::Value& jsonValue )
{
    AbcA::TimeSamplingType tst;
    fromJson( tst, jsonValue["tst"] );

    uint32_t n_times = JsonGetUint(jsonValue, "n_times").get_value_or(0);

    std::vector<chrono_t> sampleTimes;
    const rapidjson::Value& v_times = jsonValue["times"];
    for (rapidjson::Value::ConstValueIterator v_it = v_times.Begin(); v_it != v_times.End(); ++v_it)
    {
        const rapidjson::Value& v_time = *v_it;
        sampleTimes.push_back( *JsonGetDouble(v_time) );
    }

    Util::uint32_t spc = sampleTimes.size();

    ABCA_ASSERT( (spc == n_times), "wrong # of stored time samples");

    result = AbcA::TimeSampling( tst, sampleTimes );
    return true;
}

void jsonWriteTimeSampling( rapidjson::Document& document, rapidjson::Value& dst,
                            Util::uint32_t iMaxSample,
                            const AbcA::TimeSampling &iTsmp )
{
    TRACE("jsonWriteTimeSampling(iMaxSample:" << iMaxSample << ")");

    dst.SetObject();

    JsonSet(document, dst, "maxSample", iMaxSample);

    rapidjson::Value timeSamplingObject( rapidjson::kObjectType );
    toJson( document, timeSamplingObject, iTsmp );

    JsonSet(document, dst, "timeSampling", timeSamplingObject);
}

static void jsonReadTimeSampling( const rapidjson::Value& jsonValue,
    Util::uint32_t& oMaxSample, AbcA::TimeSampling& oTsmp )
{
    oMaxSample = JsonGetUint64(jsonValue, "maxSample").get_value_or(0);
    fromJson( oTsmp, jsonValue["timeSampling"] );
}

void jsonReadTimeSamples( const rapidjson::Value& jsonTimeSamples,
                       std::vector < AbcA::TimeSamplingPtr > & oTimeSamples,
                       std::vector < AbcA::index_t > & oMaxSamples )
{
    // oTimeSamples.clear();
    // // add the intrinsic default sampling
    // AbcA::TimeSamplingPtr ts( new AbcA::TimeSampling() );
    // oTimeSamples.push_back( ts );

    for (rapidjson::Value::ConstValueIterator it = jsonTimeSamples.Begin(); it != jsonTimeSamples.End(); ++it)
    {
        const rapidjson::Value& element = *it;

        Util::uint32_t maxSample;
        AbcA::TimeSampling* tsmp_ptr = new AbcA::TimeSampling();

        jsonReadTimeSampling(element, maxSample, *tsmp_ptr);

        oMaxSamples.push_back( maxSample );

        AbcA::TimeSamplingPtr tptr( tsmp_ptr );

        oTimeSamples.push_back( tptr );
    }
}

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreGit
} // End namespace Alembic
