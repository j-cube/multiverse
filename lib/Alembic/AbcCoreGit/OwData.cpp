//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

#include <Alembic/AbcCoreGit/OwData.h>
#include <Alembic/AbcCoreGit/OwImpl.h>
#include <Alembic/AbcCoreGit/CpwData.h>
#include <Alembic/AbcCoreGit/CpwImpl.h>
#include <Alembic/AbcCoreGit/WriteUtil.h>
#include <Alembic/AbcCoreGit/Utils.h>
#include <iostream>
#include <string>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
OwData::OwData( GitGroupPtr iParentGroup,
                const std::string &iName,
                const AbcA::MetaData &iMetaData )
{
	if (iParentGroup)
		TRACE("OwData::OwData(parentGroup:'" << iParentGroup->fullname() << "', '" << iName << "')");
	else
		TRACE("OwData::OwData(parentGroup:NULL, '" << iName << "')");

    // Check validity of all inputs.
    ABCA_ASSERT( iParentGroup, "Invalid parent group" );

    // Create the Git group corresponding to this object.
    //m_group_ptr.reset( new GitGroup( iParentGroup, iName ) );
    m_group_ptr = iParentGroup->addGroup( iName );
    ABCA_ASSERT( m_group_ptr,
                 "Could not create group for object: " << iName );
    m_data = Alembic::Util::shared_ptr<CpwData>(
        new CpwData( ".prop", m_group_ptr ) );

    AbcA::PropertyHeader topHeader( ".prop", iMetaData );
    UNIMPLEMENTED("WritePropertyInfo()");
#if 0
    WritePropertyInfo( m_group_ptr, topHeader, false, 0, 0, 0, 0 );
#endif /* 0 */
}

//-*****************************************************************************
OwData::~OwData()
{
	// not necessary actually
	m_group_ptr.reset();
}

//-*****************************************************************************
AbcA::CompoundPropertyWriterPtr
OwData::getProperties( AbcA::ObjectWriterPtr iParent )
{
    AbcA::CompoundPropertyWriterPtr ret = m_top.lock();
    if ( ! ret )
    {
        // time to make a new one
        ret = Alembic::Util::shared_ptr<CpwImpl>( new CpwImpl( iParent,
            m_data, iParent->getMetaData() ) );
        m_top = ret;
    }
    return ret;
}

//-*****************************************************************************
size_t OwData::getNumChildren()
{
    return m_childHeaders.size();
}

//-*****************************************************************************
const AbcA::ObjectHeader & OwData::getChildHeader( size_t i )
{
    if ( i >= m_childHeaders.size() )
    {
        ABCA_THROW( "Out of range index in OwImpl::getChildHeader: "
                     << i );
    }

    ABCA_ASSERT( m_childHeaders[i], "Invalid child header: " << i );

    return *(m_childHeaders[i]);
}

//-*****************************************************************************
const AbcA::ObjectHeader * OwData::getChildHeader( const std::string &iName )
{
    size_t numChildren = m_childHeaders.size();
    for ( size_t i = 0; i < numChildren; ++i )
    {
        if ( m_childHeaders[i]->getName() == iName )
        {
            return m_childHeaders[i].get();
        }
    }

    return NULL;
}

//-*****************************************************************************
AbcA::ObjectWriterPtr OwData::getChild( const std::string &iName )
{
    MadeChildren::iterator fiter = m_madeChildren.find( iName );
    if ( fiter == m_madeChildren.end() )
    {
        return AbcA::ObjectWriterPtr();
    }

    WeakOwPtr wptr = (*fiter).second;
    return wptr.lock();
}

AbcA::ObjectWriterPtr OwData::createChild( AbcA::ObjectWriterPtr iParent,
                                           const std::string & iFullName,
                                           const AbcA::ObjectHeader &iHeader )
{
    std::string name = iHeader.getName();

    if ( m_madeChildren.count( name ) )
    {
        ABCA_THROW( "Already have an Object named: "
                     << name );
    }

    if ( name.empty() )
    {
        ABCA_THROW( "Object not given a name, parent is: " <<
                    iFullName );
    }
    else if ( iHeader.getName().find('/') != std::string::npos )
    {
        ABCA_THROW( "Object has illegal name: "
                     << iHeader.getName() );
    }

    std::string parentName = iFullName;
    if ( parentName != "/" )
    {
        parentName += "/";
    }

    ObjectHeaderPtr header(
        new AbcA::ObjectHeader( iHeader.getName(),
                                parentName + iHeader.getName(),
                                iHeader.getMetaData() ) );

    Alembic::Util::shared_ptr<OwImpl> ret( new OwImpl( iParent,
                            m_group_ptr,
                            header ) );

    m_childHeaders.push_back( header );
    m_madeChildren[iHeader.getName()] = WeakOwPtr( ret );

    return ret;
}

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreGit
} // End namespace Alembic
