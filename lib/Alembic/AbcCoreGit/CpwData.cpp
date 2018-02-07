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

#include <Alembic/AbcCoreGit/CpwData.h>
#include <Alembic/AbcCoreGit/ReadWriteUtil.h>
#include <Alembic/AbcCoreGit/SpwImpl.h>
#include <Alembic/AbcCoreGit/ApwImpl.h>
#include <Alembic/AbcCoreGit/CpwImpl.h>
#include <Alembic/AbcCoreGit/Utils.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
CpwData::CpwData( const std::string & iName, GitGroupPtr iGroup )
    : m_group( iGroup )
    , m_name( iName )
{
    ABCA_ASSERT( m_group, "Invalid group" );

    TRACE("CpwData::CpwData(name:'" << iName << "', group:" << m_group->repr() << ")");
}

//-*****************************************************************************
CpwData::~CpwData()
{
    ABCA_ASSERT( m_group, "Invalid group" );

    bool ok = m_group->finalize();
    if (! ok)
        ABCA_THROW( "can't finalize Git group" );

    writeToDisk();

    m_group.reset();
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

    m_hashes.push_back(0);
    m_hashes.push_back(0);

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

void CpwData::writeToDisk()
{
    TRACE("CpwData::writeToDisk() path:'" << absPathname() << "'");

    GitGroupPtr group = getGroup(); 
    ABCA_ASSERT( group, "invalid group" );
    group->writeToDisk();
}

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreGit
} // End namespace Alembic
