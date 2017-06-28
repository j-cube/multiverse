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

#include <Alembic/AbcCoreGit/OrImpl.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
//-*****************************************************************************
// OBJECT READER IMPLEMENTATION
//-*****************************************************************************
//-*****************************************************************************

//-*****************************************************************************
// Reading as a child of a parent.
OrImpl::OrImpl( AbcA::ObjectReaderPtr iParent,
                GitGroupPtr iParentGroup,
                std::size_t iGroupIndex,
                ObjectHeaderPtr iHeader )
    : m_header( iHeader )
{
    m_parent = Alembic::Util::dynamic_pointer_cast< OrImpl,
        AbcA::ObjectReader > (iParent);

    // Check validity of all inputs.
    ABCA_ASSERT( m_parent, "Invalid parent in OrImpl(Object)" );
    ABCA_ASSERT( m_header, "Invalid header in OrImpl(Object)" );

    m_archive = m_parent->getArchiveImpl();
    ABCA_ASSERT( m_archive, "Invalid archive in OrImpl(Object)" );

    TRACE("OrImpl::OrImpl() from parent - header:" << iHeader->getFullName() << " parent:" << iParentGroup->fullname() << " (" << iParentGroup->pathname() << ")");
#if 0
    StreamIDPtr streamId = m_archive->getStreamID();
    std::size_t id = streamId->getID();
#endif
    // NOTE: we could create groups for OrImpl by saving children names on write side
    // and reading back them from json
    TODO("eliminate OrData iThreadId");

    // Create the Git group corresponding to this object and corresponding OrData
    GitGroupPtr group = iParentGroup->addGroup( iHeader->getName() );
    ABCA_ASSERT( group,
                 "Could not create group for OrImpl " << iHeader->getFullName() );
    m_data.reset( new OrData( group, iHeader->getFullName(), 0,
        *m_archive, m_archive->getIndexedMetaData() ) );
}

//-*****************************************************************************
OrImpl::OrImpl( Alembic::Util::shared_ptr< ArImpl > iArchive,
                OrDataPtr iData,
                ObjectHeaderPtr iHeader )
    : m_archive( iArchive )
    , m_data( iData )
    , m_header( iHeader )
{

    ABCA_ASSERT( m_archive, "Invalid archive in OrImpl(Archive)" );
    ABCA_ASSERT( m_data, "Invalid data in OrImpl(Archive)" );
    ABCA_ASSERT( m_header, "Invalid header in OrImpl(Archive)" );
}

//-*****************************************************************************
OrImpl::~OrImpl()
{
    // Nothing.
}

//-*****************************************************************************
const AbcA::ObjectHeader & OrImpl::getHeader() const
{
    return *m_header;
}

//-*****************************************************************************
AbcA::ArchiveReaderPtr OrImpl::getArchive()
{
    return m_archive;
}

//-*****************************************************************************
AbcA::ObjectReaderPtr OrImpl::getParent()
{
    return m_parent;
}

//-*****************************************************************************
AbcA::CompoundPropertyReaderPtr OrImpl::getProperties()
{
    return m_data->getProperties( asObjectPtr() );
}

//-*****************************************************************************
size_t OrImpl::getNumChildren()
{
    ABCA_ASSERT( m_data, "Invalid data in OrImpl::getNumChildren()" );
    return m_data->getNumChildren();
}

//-*****************************************************************************
const AbcA::ObjectHeader & OrImpl::getChildHeader( size_t i )
{
    ABCA_ASSERT( m_data, "Invalid data in OrImpl::getChildHeader()" );
    return m_data->getChildHeader( asObjectPtr(), i );
}

//-*****************************************************************************
const AbcA::ObjectHeader * OrImpl::getChildHeader( const std::string &iName )
{
    ABCA_ASSERT( m_data, "Invalid data in OrImpl::getChildHeader()" );
    return m_data->getChildHeader( asObjectPtr(), iName );
}

//-*****************************************************************************
AbcA::ObjectReaderPtr OrImpl::getChild( const std::string &iName )
{
    ABCA_ASSERT( m_data, "Invalid data in OrImpl::getChild()" );
    return m_data->getChild( asObjectPtr(), iName );
}

AbcA::ObjectReaderPtr OrImpl::getChild( size_t i )
{
    ABCA_ASSERT( m_data, "Invalid data in OrImpl::getChild()" );
    return m_data->getChild( asObjectPtr(), i );
}

//-*****************************************************************************
AbcA::ObjectReaderPtr OrImpl::asObjectPtr()
{
    return shared_from_this();
}

//-*****************************************************************************
bool OrImpl::getPropertiesHash( Util::Digest & oDigest )
{
    UNIMPLEMENTED("OrImpl::getPropertiesHash()");
#if 0
    StreamIDPtr streamId = m_archive->getStreamID();
    std::size_t id = streamId->getID();
    m_data->getPropertiesHash( oDigest, id );
    return true;
#endif
    return false;
}

//-*****************************************************************************
bool OrImpl::getChildrenHash( Util::Digest & oDigest )
{
    UNIMPLEMENTED("OrImpl::getChildrenHash()");
#if 0
    StreamIDPtr streamId = m_archive->getStreamID();
    std::size_t id = streamId->getID();
    m_data->getChildrenHash( oDigest, id );
    return true;
#endif
    return false;
}

//-*****************************************************************************
Alembic::Util::shared_ptr< ArImpl > OrImpl::getArchiveImpl() const
{
    return m_archive;
}

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreGit
} // End namespace Alembic
