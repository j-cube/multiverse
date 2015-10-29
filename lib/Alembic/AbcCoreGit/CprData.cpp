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

#include <Alembic/AbcCoreGit/JSON.h>

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

    std::string jsonPathname = absPathname() + ".json";

#if JSON_TO_DISK
    if (! file_exists(jsonPathname))
    {
        TRACE("[CprData " << *this << "]  no '" << jsonPathname << "' present, assuming no properties do exist...");
        return true;
    }

    std::ifstream jsonFile(jsonPathname.c_str());
    std::stringstream jsonBuffer;
    jsonBuffer << jsonFile.rdbuf();
    jsonFile.close();

    std::string jsonContents = jsonBuffer.str();
#else
    GitGroupPtr parentGroup = m_group->parent();
    boost::optional<std::string> optJsonContents = parentGroup->tree()->getChildFile(name() + ".json");
    if (! optJsonContents)
    {
        TRACE("[CprData " << *this << "] can't read git blob '" << jsonPathname << "' (Ignoring...)");
        m_read = true;
        return true;

        // ABCA_THROW( "can't read git blob '" << jsonPathname << "'" );
        // return false;
    }
    std::string jsonContents = *optJsonContents;
#endif

    JSONParser json(jsonPathname, jsonContents);
    rapidjson::Document& document = json.document;

    std::string v_kind = JsonGetString(document, "kind").get_value_or("UNKNOWN");
    ABCA_ASSERT( (v_kind == "CompoundProperty"), "invalid object kind" );

    // read number and names of (sub) properties
    //Util::uint32_t v_num_properties = root.get("num_properties", 0).asUInt();
    std::vector<std::string> properties;
    const rapidjson::Value& v_properties = document["properties"];
    for (rapidjson::Value::ConstValueIterator it = v_properties.Begin(); it != v_properties.End(); ++it)
    {
        properties.push_back( *JsonGetString(*it) );
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

    std::string subName = m_subProperties[i].name;
    std::string subAbsPathname = pathjoin(absPathname(), m_subProperties[i].name);

    TRACE("[CprData " << *this << "] CprData::readFromDiskSubHeader(" << i << ") name:'" << m_subProperties[i].name << "' path:'" << subAbsPathname << "' (READING)");

    std::string jsonPathname = subAbsPathname + ".json";

#if JSON_TO_DISK
    std::ifstream jsonFile(jsonPathname.c_str());
    std::stringstream jsonBuffer;
    jsonBuffer << jsonFile.rdbuf();
    jsonFile.close();

    std::string jsonContents = jsonBuffer.str();
#else
    GitGroupPtr parentGroup = m_group;
    boost::optional<std::string> optJsonContents = parentGroup->tree()->getChildFile(subName + ".json");
    if (! optJsonContents)
    {
        TRACE("[CprData " << *this << "] readFromDiskSubHeader(" << i << ") can't read git blob '" << jsonPathname << "'");
        ABCA_THROW( "can't read git blob '" << jsonPathname << "'" );
        return false;
    }
    std::string jsonContents = *optJsonContents;
#endif

    JSONParser json(jsonPathname, jsonContents);
    rapidjson::Document& document = json.document;

    std::string v_name = JsonGetString(document, "name").get_value_or("UNKNOWN");
    ABCA_ASSERT( (v_name == m_subProperties[i].name), "actual sub-property name differs from value stored in parent" );

    //Util::uint32_t v_index = root.get("index", 0).asUInt();

    //std::string v_fullName = root.get("fullName", "UNKNOWN").asString();

    std::string v_kind = JsonGetString(document, "kind").get_value_or("UNKNOWN");

    std::string v_type     = JsonGetString(document, "type").get_value_or("");
    std::string v_typename = JsonGetString(document, "typename").get_value_or("");
    int v_extent = JsonGetUint(document, "extent").get_value_or(0);

    const rapidjson::Value& v_propInfo = document["info"];

    bool v_isScalarLike = JsonGetBool(v_propInfo, "isScalarLike").get_value_or(false);
    bool v_isHomogenous = JsonGetBool(v_propInfo, "isHomogenous").get_value_or(false);
    Util::uint32_t v_timeSamplingIndex = JsonGetUint(v_propInfo, "timeSamplingIndex").get_value_or(0);
    Util::uint32_t v_numSamples = JsonGetUint(v_propInfo, "numSamples").get_value_or(0);
    Util::uint32_t v_firstChangedIndex = JsonGetUint(v_propInfo, "firstChangedIndex").get_value_or(0);
    Util::uint32_t v_lastChangedIndex = JsonGetUint(v_propInfo, "lastChangedIndex").get_value_or(0);
    std::string v_metadata = JsonGetString(v_propInfo, "metadata").get_value_or("");

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
