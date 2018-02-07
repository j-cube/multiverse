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

#include <Alembic/AbcCoreGit/AprImpl.h>
//#include <Alembic/AbcCoreGit/ReadUtil.h>
//#include <Alembic/AbcCoreGit/StreamManager.h>
#include <Alembic/AbcCoreGit/OrImpl.h>
#include <Alembic/AbcCoreGit/SampleStore.h>
#include <Alembic/AbcCoreGit/Git.h>
#include <Alembic/AbcCoreGit/Utils.h>

#include <iostream>
#include <fstream>

#include <Alembic/AbcCoreGit/JSON.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
AprImpl::AprImpl( AbcA::CompoundPropertyReaderPtr iParent,
                  GitGroupPtr iGroup,
                  PropertyHeaderPtr iHeader )
  : m_parent( iParent )
  , m_group( iGroup )
  , m_header( iHeader )
  , m_store( BuildSampleStore( getCprImplPtr(iParent)->getArchiveImpl(), iHeader->datatype(), /* rank-0 */ AbcA::Dimensions() ) )
  , m_read( false )
{
    TRACE("AprImpl::AprImpl(parent:" << CONCRETE_CPRPTR(m_parent)->repr() << ", parent group:" << m_group->repr() << ", header:'" << m_header->name() << "')");

    // Validate all inputs.
    ABCA_ASSERT( m_parent, "Invalid parent" );
    ABCA_ASSERT( m_group, "Invalid array property group" );
    ABCA_ASSERT( m_header, "Invalid header" );

    if ( m_header->header.getPropertyType() != AbcA::kArrayProperty )
    {
        ABCA_THROW( "Attempted to create a ArrayPropertyReader from a "
                    "non-array property type" );
    }

    readFromDisk();
}

//-*****************************************************************************
const AbcA::PropertyHeader & AprImpl::getHeader() const
{
    return m_header->header;
}

//-*****************************************************************************
AbcA::ObjectReaderPtr AprImpl::getObject()
{
    return m_parent->getObject();
}

//-*****************************************************************************
AbcA::CompoundPropertyReaderPtr AprImpl::getParent()
{
    return m_parent;
}

//-*****************************************************************************
AbcA::ArrayPropertyReaderPtr AprImpl::asArrayPtr()
{
    return shared_from_this();
}

//-*****************************************************************************
size_t AprImpl::getNumSamples()
{
    return m_header->nextSampleIndex;
}

//-*****************************************************************************
bool AprImpl::isConstant()
{
    return ( m_header->firstChangedIndex == 0 );
}

//-*****************************************************************************
void AprImpl::getSample( index_t iSampleIndex, AbcA::ArraySamplePtr &oSample )
{
    TRACE( "AprImpl::getSample(iSampleIndex:" << iSampleIndex << ")");
    // size_t index = m_header->verifyIndex( iSampleIndex );

    m_store->getSample( oSample, iSampleIndex );

    // size_t index = m_header->verifyIndex( iSampleIndex ) * 2;

    // StreamIDPtr streamId = Alembic::Util::dynamic_pointer_cast< ArImpl,
    //     AbcA::ArchiveReader > ( getObject()->getArchive() )->getStreamID();

    // std::size_t id = streamId->getID();
    // Git::IDataPtr dims = m_group->getData(index + 1, id);
    // Git::IDataPtr data = m_group->getData(index, id);

    // ReadArraySample( dims, data, id, m_header->header.getDataType(), oSample );
}

//-*****************************************************************************
std::pair<index_t, chrono_t> AprImpl::getFloorIndex( chrono_t iTime )
{
    return m_header->header.getTimeSampling()->getFloorIndex( iTime,
        m_header->nextSampleIndex );
}

//-*****************************************************************************
std::pair<index_t, chrono_t> AprImpl::getCeilIndex( chrono_t iTime )
{
    return m_header->header.getTimeSampling()->getCeilIndex( iTime,
        m_header->nextSampleIndex );
}

//-*****************************************************************************
std::pair<index_t, chrono_t> AprImpl::getNearIndex( chrono_t iTime )
{
    return m_header->header.getTimeSampling()->getNearIndex( iTime,
        m_header->nextSampleIndex );
}

//-*****************************************************************************
bool AprImpl::getKey( index_t iSampleIndex, AbcA::ArraySampleKey & oKey )
{
    oKey.readPOD = m_header->header.getDataType().getPod();
    oKey.origPOD = oKey.readPOD;
    oKey.numBytes = 0;

    return m_store->getKey( iSampleIndex, oKey );

#if 0
    oKey.readPOD = m_header->header.getDataType().getPod();
    oKey.origPOD = oKey.readPOD;
    oKey.numBytes = 0;

    // * 2 for Array properties (since we also write the dimensions)
    size_t index = m_header->verifyIndex( iSampleIndex ) * 2;

    StreamIDPtr streamId = Alembic::Util::dynamic_pointer_cast< ArImpl,
        AbcA::ArchiveReader > ( getObject()->getArchive() )->getStreamID();

    std::size_t id = streamId->getID();
    Git::IDataPtr data = m_group->getData( index, id );

    if ( data )
    {
        if ( data->getSize() >= 16 )
        {
            oKey.numBytes = data->getSize() - 16;
            data->read( 16, oKey.digest.d, 0, id );
        }

        return true;
    }

    return false;
#endif
}

//-*****************************************************************************
bool AprImpl::isScalarLike()
{
    return m_header->isScalarLike;
}

//-*****************************************************************************
void AprImpl::getDimensions( index_t iSampleIndex,
                             Alembic::Util::Dimensions & oDim )
{
    m_store->getDimensions(iSampleIndex, oDim);

    // size_t index = m_header->verifyIndex( iSampleIndex ) * 2;

    // StreamIDPtr streamId = Alembic::Util::dynamic_pointer_cast< ArImpl,
    //     AbcA::ArchiveReader > ( getObject()->getArchive() )->getStreamID();

    // std::size_t id = streamId->getID();
    // Git::IDataPtr dims = m_group->getData(index + 1, id);
    // Git::IDataPtr data = m_group->getData(index, id);

    // ReadDimensions( dims, data, id, m_header->header.getDataType(), oDim );
}

static void Convert(void *toBuffer, void *fromBuffer, size_t fromSize, const AbcA::DataType &fromDataType, Alembic::Util::PlainOldDataType toPod)
{
    Alembic::Util::PlainOldDataType fromPod = fromDataType.getPod();

    ABCA_ASSERT( ( toPod == fromPod ) || (
        toPod != Alembic::Util::kStringPOD &&
        toPod != Alembic::Util::kWstringPOD &&
        fromPod != Alembic::Util::kStringPOD &&
        fromPod != Alembic::Util::kWstringPOD ),
        "Cannot convert the data to or from a string, or wstring." );

    if (toPod == fromPod)
    {
        // no conversion to perform
        memcpy(toBuffer, fromBuffer, fromSize);
    } else if (PODNumBytes( fromPod ) <= PODNumBytes( toPod ))
    {
        // // in-place conversion
        // char * buf = static_cast< char * >( fromBuffer );
        // ConvertData( fromPod, toPod, buf, toBuffer, fromSize );

        // read into a temporary buffer and cast them one at a time
        char * buf = new char[ fromSize ];
        memcpy(buf, fromBuffer, fromSize);

        ConvertData( fromPod, toPod, buf, toBuffer, fromSize );

        delete [] buf;

    } else if (PODNumBytes( fromPod ) > PODNumBytes( toPod ))
    {
        // read into a temporary buffer and cast them one at a time
        char * buf = new char[ fromSize ];
        memcpy(buf, fromBuffer, fromSize);

        ConvertData( fromPod, toPod, buf, toBuffer, fromSize );

        delete [] buf;
    } else
    {
        TRACE("conversion not supported (from pod " << PODName( fromPod ) << " to pod " << PODName( toPod ) << ")");
    }

}

//-*****************************************************************************
void AprImpl::getAs( index_t iSampleIndex, void *iIntoLocation,
                     Alembic::Util::PlainOldDataType iPod )
{
    Alembic::Util::PlainOldDataType srcPod = m_store->getPod();

    TRACE("getAs pod:" << PODName( iPod ) << " size:" << PODNumBytes(iPod) << " srcPod:" << PODName(srcPod) << " srcSize:" << PODNumBytes(srcPod));

    Alembic::Util::Dimensions dims;
    m_store->getDimensions(iSampleIndex, dims);

    size_t extent = m_store->getDataType().getExtent();
    size_t pods_per_sample;
    if (dims.rank() == 0)
    {
        pods_per_sample = extent;
    } else
    {
        assert( dims.rank() >= 1 );

        size_t points_per_sample = dims.numPoints();
        pods_per_sample = points_per_sample * extent;
    }

    // size_t dstSize = PODNumBytes(iPod) * pods_per_sample;

    size_t srcBufferSize = PODNumBytes(srcPod) * pods_per_sample;
    // TRACE("extent:" << extent << " pods_per_sample:" << pods_per_sample << " srcSize:" << srcBufferSize << " dstSize:" << dstSize);

    char* srcBuffer = new char[ srcBufferSize ];
    /* void *srcBuffer = alloca( srcBufferSize ); */
    memset(srcBuffer, 0, srcBufferSize);
    m_store->getSample((void *)srcBuffer, iSampleIndex);

    Convert(iIntoLocation, (void *)srcBuffer, srcBufferSize, m_header->header.getDataType(), iPod);

    delete [] srcBuffer;
}

CprImplPtr AprImpl::getTParent() const
{
    Util::shared_ptr< CprImpl > parent =
       Alembic::Util::dynamic_pointer_cast< CprImpl,
        AbcA::CompoundPropertyReader > ( m_parent );
    return parent;
}

std::string AprImpl::repr(bool extended) const
{
    std::ostringstream ss;
    if (extended)
    {
        CprImplPtr parentPtr = getTParent();

        ss << "<AprImpl(parent:" << parentPtr->repr() << ", header:'" << m_header->name() << "', parent-group:" << m_group->repr() << ")>";
    } else
    {
        ss << "'" << m_header->name() << "'";
    }
    return ss.str();
}

std::string AprImpl::relPathname() const
{
    ABCA_ASSERT(m_group, "invalid group");

    std::string parent_path = m_group->relPathname();
    if (parent_path == "/")
    {
        return parent_path + name();
    } else
    {
        return parent_path + "/" + name();
    }
}

std::string AprImpl::absPathname() const
{
    ABCA_ASSERT(m_group, "invalid group");

    std::string parent_path = m_group->absPathname();
    if (parent_path == "/")
    {
        return parent_path + name();
    } else
    {
        return parent_path + "/" + name();
    }
}

bool AprImpl::readFromDisk()
{
    if (m_read)
    {
        TRACE("AprImpl::readFromDisk() path:'" << absPathname() << "' (skipping, already read)");
        return true;
    }

    ABCA_ASSERT( !m_read, "already read" );

    TRACE("[AprImpl " << *this << "] AprImpl::readFromDisk() path:'" << absPathname() << "' (READING)");
    ABCA_ASSERT( m_group, "invalid group" );

    m_group->readFromDisk();

    std::string jsonPathname = absPathname() + ".json";

#if JSON_TO_DISK
    std::ifstream jsonFile(jsonPathname.c_str());
    std::stringstream jsonBuffer;
    jsonBuffer << jsonFile.rdbuf();
    jsonFile.close();

    std::string jsonContents = jsonBuffer.str();
#else
    GitGroupPtr parentGroup = m_group;
    boost::optional<std::string> optJsonContents = parentGroup->tree()->getChildFile(name() + ".json");
    if (! optJsonContents)
    {
        ABCA_THROW( "can't read git blob '" << jsonPathname << "'" );
        return false;
    }
    std::string jsonContents = *optJsonContents;

#if MSGPACK_SAMPLES
    boost::optional<std::string> optBinContents = parentGroup->tree()->getChildFile(name() + ".bin");
    if (! optBinContents)
    {
        ABCA_THROW( "can't read git blob '" << absPathname() + ".bin" << "'" );
        return false;
    }
#endif /* MSGPACK_SAMPLES */
#endif

    JSONParser json(jsonPathname, jsonContents);
    rapidjson::Document& document = json.document;

    // TRACE( "AprImpl::readFromDisk - read JSON:" << jsonBuffer.str() );
    TODO("add AprImpl core read functionality");

    std::string v_name = JsonGetString(document, "name").get_value_or("UNKNOWN");
    std::string v_kind = JsonGetString(document, "kind").get_value_or("UNKNOWN");

    if (v_kind != "ArrayProperty")
    {
        ABCA_THROW( "expected kind 'ArrayProperty' while reading '" << jsonPathname << "'" );
        return false;
    }

    ABCA_ASSERT( v_kind == "ArrayProperty", "invalid kind" );

    std::string v_typename = JsonGetString(document, "typename").get_value_or("UNKNOWN");

    // uint8_t v_extent = JsonGetUInt(document, "extent").get_value_or(0);
    // size_t v_rank = JsonGetSizeT(document, "rank").get_value_or(0);
    // size_t v_num_samples = JsonGetSizeT(document, "num_samples").get_value_or(0);

#if MSGPACK_SAMPLES
    m_store->unpack( *optBinContents );
#else
    m_store->fromJson( root["data"] );
#endif

    m_read = true;

    ABCA_ASSERT( m_read, "data not read" );
    TRACE("[AprImpl " << *this << "]  completed read from disk");
    return true;
}

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreGit
} // End namespace Alembic
