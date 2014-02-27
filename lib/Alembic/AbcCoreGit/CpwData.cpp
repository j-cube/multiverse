//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

#include <Alembic/AbcCoreGit/CpwData.h>
#include <Alembic/AbcCoreGit/WriteUtil.h>
#include <Alembic/AbcCoreGit/SpwImpl.h>
#include <Alembic/AbcCoreGit/ApwImpl.h>
#include <Alembic/AbcCoreGit/CpwImpl.h>
#include <Alembic/AbcCoreGit/Utils.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
CpwData::CpwData( const std::string & iName, GitGroupPtr iParentGroup )
    : m_parentGroup( iParentGroup )
    , m_name( iName )
{
    if (iParentGroup)
        TRACE("CpwData::CpwData(name:'" << iName << "', parentGroup:" << iParentGroup->repr() << ")");
    else
        TRACE("CpwData::CpwData(name:'" << iName << "', parentGroup: NULL)");

    // the "top" compound property has no name
    if ( m_name == "" )
    {
        m_group = m_parentGroup;
    }
}

//-*****************************************************************************
CpwData::~CpwData()
{
    if ( m_group )
    {
        bool ok = m_group->finalize();
        if (! ok)
            ABCA_THROW( "can't finalize Git group" );
        m_group.reset();
    }
}

//-*****************************************************************************
size_t CpwData::getNumProperties()
{
    return m_propertyHeaders.size();
}

//-*****************************************************************************
const AbcA::PropertyHeader &
CpwData::getPropertyHeader( size_t i )
{
    if ( i > m_propertyHeaders.size() )
    {
        ABCA_THROW( "Out of range index in " <<
                    "CpwImpl::getPropertyHeader: " << i );
    }

    PropertyHeaderPtr ptr = m_propertyHeaders[i];
    ABCA_ASSERT( ptr, "Invalid property header ptr in CpwImpl" );

    return ptr->header;
}

//-*****************************************************************************
const AbcA::PropertyHeader *
CpwData::getPropertyHeader( const std::string &iName )
{
    for ( PropertyHeaderPtrs::iterator piter = m_propertyHeaders.begin();
          piter != m_propertyHeaders.end(); ++piter )
    {
        if ( (*piter)->header.getName() == iName )
        {
            return &( (*piter)->header );
        }
    }
    return NULL;
}

//-*****************************************************************************
AbcA::BasePropertyWriterPtr
CpwData::getProperty( const std::string &iName )
{
    MadeProperties::iterator fiter = m_madeProperties.find( iName );
    if ( fiter == m_madeProperties.end() )
    {
        return AbcA::BasePropertyWriterPtr();
    }

    WeakBpwPtr wptr = (*fiter).second;

    return wptr.lock();
}

//-*****************************************************************************
GitGroupPtr CpwData::getGroup()
{
    // If we've already made it (or set it), return it!
    if ( m_group )
    {
        return m_group;
    }

    ABCA_ASSERT( m_parentGroup, "invalid parent group" );

    // Create the Git group corresponding to this property.
    if ( m_name != "" )
    {
        return m_parentGroup->addGroup( m_name );
    } else
    {
        m_group = m_parentGroup;
    }

    ABCA_ASSERT( m_group,
                 "Could not create compound property group named: "
                 << m_name );

    return m_group;
}

//-*****************************************************************************
AbcA::ScalarPropertyWriterPtr
CpwData::createScalarProperty( AbcA::CompoundPropertyWriterPtr iParent,
                               const std::string & iName,
                               const AbcA::MetaData & iMetaData,
                               const AbcA::DataType & iDataType,
                               uint32_t iTimeSamplingIndex )
{
    if ( m_madeProperties.count( iName ) )
    {
        ABCA_THROW( "Already have a property named: " << iName );
    }

    ABCA_ASSERT( iDataType.getExtent() != 0 &&
                 iDataType.getPod() != Alembic::Util::kNumPlainOldDataTypes &&
                 iDataType.getPod() != Alembic::Util::kUnknownPOD,
                 "createScalarProperty, illegal DataType provided.");

    // will assert if TimeSamplingPtr not found
    AbcA::TimeSamplingPtr ts =
        iParent->getObject()->getArchive()->getTimeSampling(
            iTimeSamplingIndex );

    PropertyHeaderPtr headerPtr( new PropertyHeaderAndFriends( iName,
        AbcA::kScalarProperty, iMetaData, iDataType, ts, iTimeSamplingIndex ) );

    GitGroupPtr myGroup = getGroup();

    Alembic::Util::shared_ptr<SpwImpl>
        ret( new SpwImpl( iParent, myGroup, headerPtr,
                          m_propertyHeaders.size() ) );

    m_propertyHeaders.push_back( headerPtr );
    m_madeProperties[iName] = WeakBpwPtr( ret );

    m_hashes.push_back(0);
    m_hashes.push_back(0);

    return ret;
}

//-*****************************************************************************
AbcA::ArrayPropertyWriterPtr
CpwData::createArrayProperty( AbcA::CompoundPropertyWriterPtr iParent,
                              const std::string & iName,
                              const AbcA::MetaData & iMetaData,
                              const AbcA::DataType & iDataType,
                              uint32_t iTimeSamplingIndex )
{
    if ( m_madeProperties.count( iName ) )
    {
        ABCA_THROW( "Already have a property named: " << iName );
    }

    ABCA_ASSERT( iDataType.getExtent() != 0 &&
                 iDataType.getPod() != Alembic::Util::kNumPlainOldDataTypes &&
                 iDataType.getPod() != Alembic::Util::kUnknownPOD,
                 "createArrayProperty, illegal DataType provided.");

    // will assert if TimeSamplingPtr not found
    AbcA::TimeSamplingPtr ts =
        iParent->getObject()->getArchive()->getTimeSampling(
            iTimeSamplingIndex );

    PropertyHeaderPtr headerPtr( new PropertyHeaderAndFriends( iName,
        AbcA::kArrayProperty, iMetaData, iDataType, ts, iTimeSamplingIndex ) );

    //GitGroupPtr myGroup = m_group->addGroup();    // Ogawa
    GitGroupPtr myGroup = getGroup();

    Alembic::Util::shared_ptr<ApwImpl>
        ret( new ApwImpl( iParent, myGroup, headerPtr,
                          m_propertyHeaders.size() ) );

    m_propertyHeaders.push_back( headerPtr );
    m_madeProperties[iName] = WeakBpwPtr( ret );

    return ret;
}

//-*****************************************************************************
AbcA::CompoundPropertyWriterPtr
CpwData::createCompoundProperty( AbcA::CompoundPropertyWriterPtr iParent,
                                 const std::string & iName,
                                 const AbcA::MetaData & iMetaData )
{
    TRACE("ENTER");
    if ( m_madeProperties.count( iName ) )
    {
        ABCA_THROW( "Already have a property named: " << iName );
    }

    GitGroupPtr myGroup = getGroup();

   PropertyHeaderPtr headerPtr( new PropertyHeaderAndFriends( iName,
                                iMetaData ) );

    Alembic::Util::shared_ptr<CpwImpl>
        ret( new CpwImpl( iParent, myGroup, headerPtr,
                          m_propertyHeaders.size() ) );

    m_propertyHeaders.push_back( headerPtr );
    m_madeProperties[iName] = WeakBpwPtr( ret );

    m_hashes.push_back(0);
    m_hashes.push_back(0);

    return ret;
}

void CpwData::writePropertyHeaders( MetaDataMapPtr iMetaDataMap )
{
    // pack in child header and other info
    std::vector< Util::uint8_t > data;
    for ( size_t i = 0; i < getNumProperties(); ++i )
    {
        PropertyHeaderPtr prop = m_propertyHeaders[i];
        WritePropertyInfo( data,
                           prop->header,
                           prop->isScalarLike,
                           prop->isHomogenous,
                           prop->timeSamplingIndex,
                           prop->nextSampleIndex,
                           prop->firstChangedIndex,
                           prop->lastChangedIndex,
                           iMetaDataMap );
    }

    if ( !data.empty() )
    {
        m_group->addData( data.size(), &( data.front() ) );
    }
}

//-*****************************************************************************
void CpwData::fillHash( size_t iIndex, Util::uint64_t iHash0,
    Util::uint64_t iHash1 )
{

    ABCA_ASSERT( iIndex < m_propertyHeaders.size() &&
                 iIndex * 2 < m_hashes.size(),
                 "Invalid property requested in CpwData::fillHash" );

    m_hashes[ iIndex * 2     ] = iHash0;
    m_hashes[ iIndex * 2 + 1 ] = iHash1;
}

//-*****************************************************************************
void CpwData::computeHash( Util::SpookyHash & ioHash )
{
    if ( !m_hashes.empty() )
    {
        ioHash.Update( &m_hashes.front(), m_hashes.size() * 8 );
    }
}

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreGit
} // End namespace Alembic
