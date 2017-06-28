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

#include <Alembic/AbcCoreGit/SprImpl.h>
//#include <Alembic/AbcCoreGit/ReadUtil.h>
#include <Alembic/AbcCoreGit/ReadWriteUtil.h>
//#include <Alembic/AbcCoreGit/StreamManager.h>
#include <Alembic/AbcCoreGit/OrImpl.h>
#include <Alembic/AbcCoreGit/Utils.h>

#include "Utils.h"

#include <iostream>
#include <fstream>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
SprImpl::SprImpl( AbcA::CompoundPropertyReaderPtr iParent,
                  GitGroupPtr iParentGroup,
                  PropertyHeaderPtr iHeader )
  : m_parent( iParent )
  , m_group( iParentGroup )
  , m_header( iHeader ),
  m_store( BuildSampleStore( getCprImplPtr(iParent)->getArchiveImpl(), iHeader->datatype(), /* rank-0 */ AbcA::Dimensions() ) ),
  m_read( false )
{
    TRACE("SprImpl::SprImpl(parent:" << CONCRETE_CPRPTR(m_parent)->repr() << ", parent group:" << m_group->repr() << ", header:'" << m_header->name() << "')");

    // Validate all inputs.
    ABCA_ASSERT( m_parent, "Invalid parent" );
    ABCA_ASSERT( m_group, "Invalid scalar property group" );
    ABCA_ASSERT( m_header, "Invalid header" );

    if ( m_header->header.getPropertyType() != AbcA::kScalarProperty )
    {
        ABCA_THROW( "Attempted to create a ScalarPropertyReader from a "
                    "non-array property type" );
    }

    readFromDisk();
}

//-*****************************************************************************
const AbcA::PropertyHeader & SprImpl::getHeader() const
{
    return m_header->header;
}

//-*****************************************************************************
AbcA::ObjectReaderPtr SprImpl::getObject()
{
    return m_parent->getObject();
}

//-*****************************************************************************
AbcA::CompoundPropertyReaderPtr SprImpl::getParent()
{
    return m_parent;
}

//-*****************************************************************************
AbcA::ScalarPropertyReaderPtr SprImpl::asScalarPtr()
{
    return shared_from_this();
}

//-*****************************************************************************
size_t SprImpl::getNumSamples()
{
    return m_header->nextSampleIndex;
}

//-*****************************************************************************
bool SprImpl::isConstant()
{
    return ( m_header->firstChangedIndex == 0 );
}

//-*****************************************************************************
void SprImpl::getSample( index_t iSampleIndex, void * iIntoLocation )
{
    TRACE( "SprImpl::getSample(iSampleIndex:" << iSampleIndex << ")");
    // size_t index = m_header->verifyIndex( iSampleIndex );

    m_store->getSample( iIntoLocation, iSampleIndex );
}

//-*****************************************************************************
std::pair<index_t, chrono_t> SprImpl::getFloorIndex( chrono_t iTime )
{
    return m_header->header.getTimeSampling()->getFloorIndex( iTime,
        m_header->nextSampleIndex );
}

//-*****************************************************************************
std::pair<index_t, chrono_t> SprImpl::getCeilIndex( chrono_t iTime )
{
    return m_header->header.getTimeSampling()->getCeilIndex( iTime,
        m_header->nextSampleIndex );
}

//-*****************************************************************************
std::pair<index_t, chrono_t> SprImpl::getNearIndex( chrono_t iTime )
{
    return m_header->header.getTimeSampling()->getNearIndex( iTime,
        m_header->nextSampleIndex );
}

CprImplPtr SprImpl::getTParent() const
{
    Util::shared_ptr< CprImpl > parent =
       Alembic::Util::dynamic_pointer_cast< CprImpl,
        AbcA::CompoundPropertyReader > ( m_parent );
    return parent;
}

std::string SprImpl::repr(bool extended) const
{
    std::ostringstream ss;
    if (extended)
    {
        CprImplPtr parentPtr = getTParent();

        ss << "<SprImpl(parent:" << parentPtr->repr() << ", header:'" << m_header->name() << "', parent-group:" << m_group->repr() << ")>";
    } else
    {
        ss << "'" << m_header->name() << "'";
    }
    return ss.str();
}

std::string SprImpl::relPathname() const
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

std::string SprImpl::absPathname() const
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

bool SprImpl::readFromDisk()
{
    if (m_read)
    {
        TRACE("SprImpl::readFromDisk() path:'" << absPathname() << "' (skipping, already read)");
        return true;
    }

    ABCA_ASSERT( !m_read, "already read" );

    TRACE("[SprImpl " << *this << "] SprImpl::readFromDisk() path:'" << absPathname() << "' (READING)");
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
#if !MSGPACK_SAMPLES
    rapidjson::Document& document = json.document;
#endif

    // TRACE( "SprImpl::readFromDisk - read JSON:" << jsonBuffer.str() );
    TODO("add SprImpl core read functionality");

#if MSGPACK_SAMPLES
    m_store->unpack( *optBinContents );
#else
    m_store->fromJson( document["data"] );
#endif

    m_read = true;

    ABCA_ASSERT( m_read, "data not read" );
    TRACE("[SprImpl " << *this << "]  completed read from disk");
    return true;
}

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreGit
} // End namespace Alembic
