//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

#include <Alembic/AbcCoreGit/OwData.h>
#include <Alembic/AbcCoreGit/OwImpl.h>
#include <Alembic/AbcCoreGit/CpwImpl.h>
#include <Alembic/AbcCoreGit/Utils.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

OwImpl::OwImpl( AbcA::ArchiveWriterPtr iArchive,
                OwDataPtr iData,
                const AbcA::MetaData & iMetaData )
    : m_archive( iArchive )
    , m_header( new AbcA::ObjectHeader( "ABC", "/", iMetaData ) )
    , m_data( iData )
{
    if (iData)
        TRACE("OwImpl::OwImpl(iArchive" << ", group:'" << iData->getGroup()->fullname() << "')");
    else
        TRACE("OwImpl::OwImpl(iArchive" << ", group:NULL)");

    ABCA_ASSERT( m_archive, "Invalid archive" );
    ABCA_ASSERT( m_data, "Invalid data" );
}

//-*****************************************************************************
OwImpl::OwImpl( AbcA::ObjectWriterPtr iParent,
                GitGroupPtr iParentGroup,
                ObjectHeaderPtr iHeader )
  : m_parent( iParent )
  , m_header( iHeader )
{
    if (iParentGroup)
        TRACE("OwImpl::OwImpl(iParent" << ", parentGroup:'" << iParentGroup->fullname() << "')");
    else
        TRACE("OwImpl::OwImpl(iParent, parentGroup:NULL)");

    // Check validity of all inputs.
    ABCA_ASSERT( m_parent, "Invalid parent" );
    ABCA_ASSERT( m_header, "Invalid header" );

    m_archive = m_parent->getArchive();
    ABCA_ASSERT( m_archive, "Invalid archive" );

    m_data.reset( new OwData( iParentGroup, m_header->getName(),
                              m_header->getMetaData() ) );
}

//-*****************************************************************************
OwImpl::~OwImpl()
{
}

//-*****************************************************************************
const AbcA::ObjectHeader & OwImpl::getHeader() const
{
    ABCA_ASSERT( m_header, "Invalid header" );
    return *m_header;
}

//-*****************************************************************************
AbcA::ArchiveWriterPtr OwImpl::getArchive()
{
    return m_archive;
}

//-*****************************************************************************
AbcA::ObjectWriterPtr OwImpl::getParent()
{
    return m_parent;
}

//-*****************************************************************************
AbcA::CompoundPropertyWriterPtr OwImpl::getProperties()
{
    return m_data->getProperties( asObjectPtr() );
}

//-*****************************************************************************
size_t OwImpl::getNumChildren()
{
    return m_data->getNumChildren();
}

//-*****************************************************************************
const AbcA::ObjectHeader & OwImpl::getChildHeader( size_t i )
{
    return m_data->getChildHeader( i );
}

const AbcA::ObjectHeader * OwImpl::getChildHeader( const std::string &iName )
{
    return m_data->getChildHeader( iName );
}

//-*****************************************************************************
AbcA::ObjectWriterPtr OwImpl::getChild( const std::string &iName )
{
    return m_data->getChild( iName );
}

//-*****************************************************************************
AbcA::ObjectWriterPtr OwImpl::createChild( const AbcA::ObjectHeader &iHeader )
{
    return m_data->createChild( asObjectPtr(), m_header->getFullName(),
                                iHeader );
}

//-*****************************************************************************
AbcA::ObjectWriterPtr OwImpl::asObjectPtr()
{
    return shared_from_this();
}

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreGit
} // End namespace Alembic
