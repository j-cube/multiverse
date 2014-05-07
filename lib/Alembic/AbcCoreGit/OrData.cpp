//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

#include <Alembic/AbcCoreGit/OrData.h>
#include <Alembic/AbcCoreGit/OrImpl.h>
// #include <Alembic/AbcCoreGit/CprData.h>
// #include <Alembic/AbcCoreGit/CprImpl.h>
// #include <Alembic/AbcCoreGit/ReadUtil.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
OrData::OrData( GitGroupPtr iGroup,
                const std::string & iParentName,
                std::size_t iThreadId,
                AbcA::ArchiveReader & iArchive,
                const std::vector< AbcA::MetaData > & iIndexedMetaData )
    : m_children( NULL )
{
    ABCA_ASSERT( iGroup, "Invalid object data group" );

    m_group = iGroup;

    TODO("read object headers");
#if 0
    std::size_t numChildren = m_group->getNumChildren();

    if ( numChildren > 0 && m_group->isChildData( numChildren - 1 ) )
    {
        std::vector< ObjectHeaderPtr > headers;
        ReadObjectHeaders( m_group, numChildren - 1, iThreadId,
                           iParentName, iIndexedMetaData, headers );

        if ( !headers.empty() )
        {
            m_children = new Child[ headers.size() ];
        }

        for ( std::size_t i = 0; i < headers.size(); ++i )
        {
            m_childrenMap[headers[i]->getName()] = i;
            m_children[i].header = headers[i];
        }
    }
#endif

    TODO("create m_data (Cpr)");
#if 0
    if ( numChildren > 0 && m_group->isChildGroup( 0 ) )
    {
        GitGroupPtr group = m_group->getGroup( 0, false, iThreadId );
        m_data = Alembic::Util::shared_ptr<CprData>(
            new CprData( group, iThreadId, iArchive, iIndexedMetaData ) );
    }
#endif
}

//-*****************************************************************************
OrData::~OrData()
{
    if ( m_children )
    {
        delete [] m_children;
    }
}

//-*****************************************************************************
AbcA::CompoundPropertyReaderPtr
OrData::getProperties( AbcA::ObjectReaderPtr iParent )
{
    Alembic::Util::scoped_lock l( m_cprlock );
    AbcA::CompoundPropertyReaderPtr ret = m_top.lock();

    UNIMPLEMENTED("getProperties()");
    TODO("create CprImpl");
#if 0
    if ( ! ret )
    {
        // time to make a new one
        ret = Alembic::Util::shared_ptr<CprImpl>(
            new CprImpl( iParent, m_data ) );
        m_top = ret;
    }
#endif

    return ret;
}

//-*****************************************************************************
size_t OrData::getNumChildren()
{
    return m_childrenMap.size();
}

//-*****************************************************************************
const AbcA::ObjectHeader &
OrData::getChildHeader( AbcA::ObjectReaderPtr iParent, size_t i )
{
    ABCA_ASSERT( i < m_childrenMap.size(),
        "Out of range index in OrData::getChildHeader: " << i );

    return *( m_children[i].header );
}

//-*****************************************************************************
const AbcA::ObjectHeader *
OrData::getChildHeader( AbcA::ObjectReaderPtr iParent,
                        const std::string &iName )
{
    ChildrenMap::iterator fiter = m_childrenMap.find( iName );
    if ( fiter == m_childrenMap.end() )
    {
        return NULL;
    }

    return & getChildHeader( iParent, fiter->second );
}

//-*****************************************************************************
AbcA::ObjectReaderPtr
OrData::getChild( AbcA::ObjectReaderPtr iParent, const std::string &iName )
{
    ChildrenMap::iterator fiter = m_childrenMap.find( iName );
    if ( fiter == m_childrenMap.end() )
    {
        return AbcA::ObjectReaderPtr();
    }

    return getChild( iParent, fiter->second );
}

//-*****************************************************************************
AbcA::ObjectReaderPtr
OrData::getChild( AbcA::ObjectReaderPtr iParent, size_t i )
{
    ABCA_ASSERT( i < m_childrenMap.size(),
        "Out of range index in OrData::getChild: " << i );

    Alembic::Util::scoped_lock l( m_children[i].lock );
    AbcA::ObjectReaderPtr optr = m_children[i].made.lock();

    if ( ! optr )
    {
        // Make a new one.
        optr = Alembic::Util::shared_ptr<OrImpl>(
            new OrImpl( iParent, m_group, i + 1, m_children[i].header ) );
        m_children[i].made = optr;
    }

    return optr;
}

void OrData::getPropertiesHash( Util::Digest & oDigest, size_t iThreadId )
{
    UNIMPLEMENTED("OrData::getPropertiesHash()");
    TODO("use json to determine the number of children");
#if 0
    std::size_t numChildren = m_group->getNumChildren();
    Git::IDataPtr data = m_group->getData( numChildren - 1, iThreadId );
    if ( data && data->getSize() >= 32 )
    {
        // last 32 bytes are properties hash, followed by children hash
        data->read( 16, oDigest.d, data->getSize() - 32, iThreadId );
    }
#endif
}

void OrData::getChildrenHash( Util::Digest & oDigest, size_t iThreadId )
{
    UNIMPLEMENTED("OrData::getChildrenHash()");
    TODO("use json to get the children hash");
#if 0
    std::size_t numChildren = m_group->getNumChildren();
    Git::IDataPtr data = m_group->getData( numChildren - 1, iThreadId );
    if ( data && data->getSize() >= 32 )
    {
        // children hash is the last 16 bytes
        data->read( 16, oDigest.d, data->getSize() - 16, iThreadId );
    }
#endif
}

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreGit
} // End namespace Alembic
