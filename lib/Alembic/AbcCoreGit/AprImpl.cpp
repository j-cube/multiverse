/*****************************************************************************/
/*  Copyright (c) 2013-2015 J CUBE I. Tokyo, Japan. All Rights Reserved.     */
/*                                                                           */
/*  This program is free software; you can redistribute it and/or modify     */
/*  it under the terms of the GNU General Public License as published by     */
/*  the Free Software Foundation; either version 2 of the License, or        */
/*  (at your option) any later version.                                      */
/*                                                                           */
/*  This program is distributed in the hope that it will be useful,          */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of           */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            */
/*  GNU General Public License for more details.                             */
/*                                                                           */
/*  You should have received a copy of the GNU General Public License along  */
/*  with this program; if not, write to the Free Software Foundation, Inc.,  */
/*  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.              */
/*                                                                           */
/*  J CUBE Inc.                                                              */
/*  6F Azabu Green Terrace                                                   */
/*  3-20-1 Minami-Azabu, Minato-ku, Tokyo                                    */
/*  info@-jcube.jp                                                           */
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
    UNIMPLEMENTED("AprImpl::getDimensions()");
    // size_t index = m_header->verifyIndex( iSampleIndex ) * 2;

    // StreamIDPtr streamId = Alembic::Util::dynamic_pointer_cast< ArImpl,
    //     AbcA::ArchiveReader > ( getObject()->getArchive() )->getStreamID();

    // std::size_t id = streamId->getID();
    // Git::IDataPtr dims = m_group->getData(index + 1, id);
    // Git::IDataPtr data = m_group->getData(index, id);

    // ReadDimensions( dims, data, id, m_header->header.getDataType(), oDim );
}

//-*****************************************************************************
void AprImpl::getAs( index_t iSampleIndex, void *iIntoLocation,
                     Alembic::Util::PlainOldDataType iPod )
{
    UNIMPLEMENTED("AprImpl::getAs()");
    // size_t index = m_header->verifyIndex( iSampleIndex ) * 2;

    // StreamIDPtr streamId = Alembic::Util::dynamic_pointer_cast< ArImpl,
    //     AbcA::ArchiveReader > ( getObject()->getArchive() )->getStreamID();

    // std::size_t id = streamId->getID();
    // Git::IDataPtr data = m_group->getData( index, id );
    // ReadData( iIntoLocation, data, id, m_header->header.getDataType(), iPod );
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
