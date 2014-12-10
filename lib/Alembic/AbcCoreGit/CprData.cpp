//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

#include <Alembic/AbcCoreGit/CprData.h>
#include <Alembic/AbcCoreGit/CprImpl.h>
#include <Alembic/AbcCoreGit/SprImpl.h>
#include <Alembic/AbcCoreGit/AprImpl.h>
#include <Alembic/AbcCoreGit/ArImpl.h>
#include <Alembic/AbcCoreGit/Utils.h>

#include <Alembic/Util/PlainOldDataType.h>

#include "Utils.h"

#include <iostream>
#include <fstream>
#include <json/json.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
CprData::CprData( GitGroupPtr iGroup,
                  std::size_t iThreadId,
                  AbcA::ArchiveReader & iArchive,
                  const std::vector< AbcA::MetaData > & iIndexedMetaData )
    : m_archive(iArchive), m_subProperties( NULL ), m_read( false )
{
    ABCA_ASSERT( iGroup, "invalid compound data group" );

    m_group = iGroup;

    TODO("read sup-properties headers");
#if 0
    std::size_t numChildren = m_group->getNumChildren();

    if ( numChildren > 0 && m_group->isChildData( numChildren - 1 ) )
    {
        PropertyHeaderPtrs headers;
        ReadPropertyHeaders( m_group, numChildren - 1, iThreadId,
                             iArchive, iIndexedMetaData, headers );

        m_subProperties = new SubProperty[ headers.size() ];
        for ( std::size_t i = 0; i < headers.size(); ++i )
        {
            m_subPropertiesMap[headers[i]->header.getName()] = i;
            m_subProperties[i].header = headers[i];
        }
    }
#endif /* 0 */

    readFromDisk();
}

//-*****************************************************************************
CprData::~CprData()
{
    if ( m_subProperties )
    {
        delete [] m_subProperties;
    }
}

//-*****************************************************************************
size_t CprData::getNumProperties()
{
    // fixed length and resize called in ctor, so multithread safe.
    return m_subPropertiesMap.size();
}

//-*****************************************************************************
const AbcA::PropertyHeader &
CprData::getPropertyHeader( AbcA::CompoundPropertyReaderPtr iParent, size_t i )
{
    // fixed length and resize called in ctor, so multithread safe.
    if ( i > m_subPropertiesMap.size() )
    {
        ABCA_THROW( "Out of range index in "
                    << "CprData::getPropertyHeader: " << i );
    }

    if (! m_subProperties[i].read)
        readFromDiskSubHeader( i );

    assert( m_subProperties[i].read );
    assert( m_subProperties[i].header );

    return m_subProperties[i].header->header;
}

//-*****************************************************************************
const AbcA::PropertyHeader *
CprData::getPropertyHeader( AbcA::CompoundPropertyReaderPtr iParent,
                            const std::string &iName )
{
    // map of names to indexes filled by ctor (CprAttrVistor),
    // so multithread safe.
    SubPropertiesMap::iterator fiter = m_subPropertiesMap.find( iName );
    if ( fiter == m_subPropertiesMap.end() )
    {
        return NULL;
    }

    return &(getPropertyHeader(iParent, fiter->second));
}

//-*****************************************************************************
AbcA::ScalarPropertyReaderPtr
CprData::getScalarProperty( AbcA::CompoundPropertyReaderPtr iParent,
                            const std::string &iName )
{
    SubPropertiesMap::iterator fiter = m_subPropertiesMap.find( iName );
    if ( fiter == m_subPropertiesMap.end() )
    {
        return AbcA::ScalarPropertyReaderPtr();
    }

    size_t propIndex = fiter->second;

    if (! m_subProperties[propIndex].read)
        readFromDiskSubHeader( propIndex );

    assert( m_subProperties[propIndex].read );
    assert( m_subProperties[propIndex].header );

    SubProperty & sub = m_subProperties[propIndex];

    if ( !(sub.header->header.isScalar()) )
    {
        ABCA_THROW( "Tried to read a scalar property from a non-scalar: "
                    << iName << ", type: "
                    << sub.header->header.getPropertyType() );
    }

    Alembic::Util::scoped_lock l( sub.lock );
    AbcA::BasePropertyReaderPtr bptr = sub.made.lock();
    if ( ! bptr )
    {
        TRACE("Going to read SprImpl for parent group: " << getGroup()->name() << " sub-property: " << sub.name << "  header: " << sub.header->name());
        //GitGroupPtr group = Alembic::Util::shared_ptr<GitGroup>( new GitGroup( m_group, sub.name ) );
        //ABCA_ASSERT( group, "Scalar Property not backed by a valid group.");

        // Make a new one.
        TRACE("CprData::getScalarProperty() create a new SprImpl");
        bptr = Alembic::Util::shared_ptr<SprImpl>(
            new SprImpl( iParent, getGroup(), sub.header ) );
        sub.made = bptr;
    }

    AbcA::ScalarPropertyReaderPtr ret =
        Alembic::Util::dynamic_pointer_cast<AbcA::ScalarPropertyReader,
        AbcA::BasePropertyReader>( bptr );
    return ret;
}

//-*****************************************************************************
AbcA::ArrayPropertyReaderPtr
CprData::getArrayProperty( AbcA::CompoundPropertyReaderPtr iParent,
                           const std::string &iName )
{
    // map of names to indexes filled by ctor (CprAttrVistor),
    // so multithread safe.
    SubPropertiesMap::iterator fiter = m_subPropertiesMap.find( iName );
    if ( fiter == m_subPropertiesMap.end() )
    {
        return AbcA::ArrayPropertyReaderPtr();
    }

    size_t propIndex = fiter->second;

    if (! m_subProperties[propIndex].read)
        readFromDiskSubHeader( propIndex );

    assert( m_subProperties[propIndex].read );
    assert( m_subProperties[propIndex].header );

    SubProperty & sub = m_subProperties[propIndex];

    if ( !(sub.header->header.isArray()) )
    {
        ABCA_THROW( "Tried to read an array property from a non-array: "
                    << iName << ", type: "
                    << sub.header->header.getPropertyType() );
    }

    Alembic::Util::scoped_lock l( sub.lock );
    AbcA::BasePropertyReaderPtr bptr = sub.made.lock();
    if ( ! bptr )
    {
        // Ogawa
        //GitGroupPtr group = Alembic::Util::shared_ptr<GitGroup>( new GitGroup( m_group, sub.name ) );
        GitGroupPtr myGroup = getGroup();

        ABCA_ASSERT( myGroup, "Array Property not backed by a valid group.");

        // Make a new one.
        bptr = Alembic::Util::shared_ptr<AprImpl>(
            new AprImpl( iParent, myGroup, sub.header ) );

        sub.made = bptr;
    }

    AbcA::ArrayPropertyReaderPtr ret =
        Alembic::Util::dynamic_pointer_cast<AbcA::ArrayPropertyReader,
        AbcA::BasePropertyReader>( bptr );
    return ret;
}

//-*****************************************************************************
AbcA::CompoundPropertyReaderPtr
CprData::getCompoundProperty( AbcA::CompoundPropertyReaderPtr iParent,
                              const std::string &iName )
{
    // map of names to indexes filled by ctor (CprAttrVistor),
    // so multithread safe.
    SubPropertiesMap::iterator fiter = m_subPropertiesMap.find( iName );
    if ( fiter == m_subPropertiesMap.end() )
    {
        return AbcA::CompoundPropertyReaderPtr();
    }

    size_t propIndex = fiter->second;

    if (! m_subProperties[propIndex].read)
        readFromDiskSubHeader( propIndex );

    assert( m_subProperties[propIndex].read );
    assert( m_subProperties[propIndex].header );

    SubProperty & sub = m_subProperties[propIndex];

    if ( !(sub.header->header.isCompound()) )
    {
        ABCA_THROW( "Tried to read a compound property from a non-compound: "
                    << iName << ", type: "
                    << sub.header->header.getPropertyType() );
    }

    Alembic::Util::scoped_lock l( sub.lock );
    AbcA::BasePropertyReaderPtr bptr = sub.made.lock();
    if ( ! bptr )
    {
        Alembic::Util::shared_ptr<  ArImpl > implPtr =
            Alembic::Util::dynamic_pointer_cast< ArImpl, AbcA::ArchiveReader > (
                iParent->getObject()->getArchive() );

        GitGroupPtr group = Alembic::Util::shared_ptr<GitGroup>( new GitGroup( m_group, sub.name ) );

        ABCA_ASSERT( group, "Compound Property not backed by a valid group.");

        // Make a new one.
        bptr = Alembic::Util::shared_ptr<CprImpl>(
            new CprImpl( iParent, group, sub.header, /* iThreadId */0,
                         implPtr->getIndexedMetaData() ) );

        sub.made = bptr;
    }

    AbcA::CompoundPropertyReaderPtr ret =
        Alembic::Util::dynamic_pointer_cast<AbcA::CompoundPropertyReader,
        AbcA::BasePropertyReader>( bptr );
    return ret;
}

bool CprData::readFromDisk()
{
    if (m_read)
    {
        TRACE("CprData::readFromDisk() path:'" << absPathname() << "' (skipping, already read)");
        return true;
    }

    ABCA_ASSERT( !m_read, "already read" );

    TRACE("[CprData " << *this << "] CprData::readFromDisk() path:'" << absPathname() << "' (READING)");
    ABCA_ASSERT( m_group, "invalid group" );

    m_group->readFromDisk();

    Json::Value root;
    Json::Reader reader;

    std::string jsonPathname = absPathname() + ".json";
    if (! file_exists(jsonPathname))
    {
        TRACE("[CprData " << *this << "]  no '" << jsonPathname << "' present, assuming no properties do exist...");
        return true;
    }

    std::ifstream jsonFile(jsonPathname.c_str());
    std::stringstream jsonBuffer;
    jsonBuffer << jsonFile.rdbuf();
    jsonFile.close();

    bool parsingSuccessful = reader.parse(jsonBuffer.str(), root);
    if (! parsingSuccessful)
    {
        ABCA_THROW( "format error while parsing '" << jsonPathname << "': " << reader.getFormatedErrorMessages() );
        return false;
    }

    std::string v_kind = root.get("kind", "UNKNOWN").asString();
    ABCA_ASSERT( (v_kind == "CompoundProperty"), "invalid object kind" );

    // read number and names of (sub) properties
    //Util::uint32_t v_num_properties = root.get("num_properties", 0).asUInt();
    Json::Value v_properties = root["properties"];
    std::vector<std::string> properties;
    for (Json::Value::iterator it = v_properties.begin(); it != v_properties.end(); ++it)
    {
        properties.push_back( (*it).asString() );
    }

    TODO("read properties");
    TRACE("[CprData " << *this << "] # (sub-)properties: " << properties.size());
    for (std::vector<std::string>::const_iterator it = properties.begin(); it != properties.end(); ++it)
    {
        TRACE("  property: '" << *it << "'");
    }

    assert(! m_subProperties);
    ABCA_ASSERT( !m_subProperties, "m_subProperties already set" );
    m_subProperties = new SubProperty[ properties.size() ];
    std::size_t i = 0;
    for (std::vector<std::string>::const_iterator it = properties.begin(); it != properties.end(); ++it)
    {
        TRACE("  sub-property: '" << *it << "'");
        m_subPropertiesMap[*it] = i;
        m_subProperties[i].index = i;
        m_subProperties[i].name = *it;
        m_subProperties[i].read = false;

        i++;
    }

    m_read = true;

    ABCA_ASSERT( m_read, "data not read" );
    TRACE("[CprData " << *this << "]  completed read from disk");
    return true;
}

bool CprData::readFromDiskSubHeader(size_t i)
{
    ABCA_ASSERT( m_read, "data not read" );
    assert( m_subProperties );
    ABCA_ASSERT( m_subProperties, "sub-properties info not yet read from disk" );

    ABCA_ASSERT( i < m_subPropertiesMap.size(),
        "Out of range index in CprData::readFromDiskSubHeader: " << i );

    if (m_subProperties[i].read)
    {
        TRACE("CprData::readFromDiskSubHeader(" << i << ") name:'" << m_subProperties[i].name << "' (skipping, already read)");
        return true;
    }

    ABCA_ASSERT( !m_subProperties[i].read, "sub-property data already read" );

    ABCA_ASSERT( m_group, "invalid group" );

    std::string subAbsPathname = pathjoin(absPathname(), m_subProperties[i].name);

    TRACE("[CprData " << *this << "] CprData::readFromDiskSubHeader(" << i << ") name:'" << m_subProperties[i].name << "' path:'" << subAbsPathname << "' (READING)");

    Json::Value root;
    Json::Reader reader;

    std::string jsonPathname = subAbsPathname + ".json";
    std::ifstream jsonFile(jsonPathname.c_str());
    std::stringstream jsonBuffer;
    jsonBuffer << jsonFile.rdbuf();
    jsonFile.close();

    bool parsingSuccessful = reader.parse(jsonBuffer.str(), root);
    if (! parsingSuccessful)
    {
        ABCA_THROW( "format error while parsing '" << jsonPathname << "': " << reader.getFormatedErrorMessages() );
        return false;
    }

    std::string v_name = root.get("name", "UNKNOWN").asString();
    ABCA_ASSERT( (v_name == m_subProperties[i].name), "actual sub-property name differs from value stored in parent" );

    //Util::uint32_t v_index = root.get("index", 0).asUInt();

    //std::string v_fullName = root.get("fullName", "UNKNOWN").asString();

    std::string v_kind = root.get("kind", "UNKNOWN").asString();

    std::string v_type = root.get("type", "").asString();
    std::string v_typename = root.get("typename", "").asString();
    int v_extent = root.get("extent", 0).asInt();

    Json::Value v_propInfo = root["info"];

    bool v_isScalarLike = v_propInfo.get("isScalarLike", false).asBool();
    bool v_isHomogenous = v_propInfo.get("isHomogenous", false).asBool();
    Util::uint32_t v_timeSamplingIndex = v_propInfo.get("timeSamplingIndex", 0).asUInt();
    Util::uint32_t v_numSamples = v_propInfo.get("numSamples", 0).asUInt();
    Util::uint32_t v_firstChangedIndex = v_propInfo.get("firstChangedIndex", 0).asUInt();
    Util::uint32_t v_lastChangedIndex = v_propInfo.get("lastChangedIndex", 0).asUInt();
    std::string v_metadata = v_propInfo.get("metadata", "").asString();

    AbcA::MetaData metadata;
    metadata.deserialize( v_metadata );

    assert( !m_subProperties[i].header );

    PropertyHeaderPtr header( new PropertyHeaderAndFriends() );
    header->header.setName( v_name );

    header->isScalarLike = v_isScalarLike;

    if (v_kind == "CompoundProperty")
    {
        header->header.setPropertyType( AbcA::kCompoundProperty );
    } else if (v_kind == "ScalarProperty")
    {
        header->header.setPropertyType( AbcA::kScalarProperty );
    } else if (v_kind == "ArrayProperty")
    {
        header->header.setPropertyType( AbcA::kArrayProperty );
    }

    if (!header->header.isCompound())
    {
        header->header.setDataType( AbcA::DataType( Alembic::Util::PODFromName(v_typename), v_extent ) );

        header->isHomogenous = v_isHomogenous;

        header->nextSampleIndex = v_numSamples;

        header->firstChangedIndex = v_firstChangedIndex;
        header->lastChangedIndex = v_lastChangedIndex;

        header->timeSamplingIndex = v_timeSamplingIndex;

        TRACE("[CprData " << *this << "]  timeSamplingIndex:" << v_timeSamplingIndex);
        header->header.setTimeSampling( m_archive.getTimeSampling( header->timeSamplingIndex ) );
    }

    header->header.setMetaData( metadata );

    m_subProperties[i].header = header;

    m_subProperties[i].read = true;

    ABCA_ASSERT( m_subProperties[i].read, "child data not read" );
    TRACE("[CprData " << *this << "]  completed read child " << i << " (name:'" << m_subProperties[i].name << "') data from disk");
    return true;
}

std::string CprData::repr(bool extended) const
{
    std::ostringstream ss;
    if (extended)
    {
        ss << "<CprData(name:'" << name() << "')>";
    } else
    {
        ss << "'" << name() << "'";
    }
    return ss.str();
}

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreGit
} // End namespace Alembic
