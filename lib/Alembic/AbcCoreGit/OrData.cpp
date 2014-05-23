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
#include <Alembic/AbcCoreGit/Utils.h>

#include <iostream>
#include <fstream>
#include <json/json.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
OrData::OrData( GitGroupPtr iGroup,
                const std::string & iParentName,
                std::size_t iThreadId,
                AbcA::ArchiveReader & iArchive,
                const std::vector< AbcA::MetaData > & iIndexedMetaData )
    : m_children( NULL ), m_read( false )
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

    readFromDisk();
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

bool OrData::readFromDisk()
{
    if (m_read)
    {
        TRACE("OrData::readFromDisk() path:'" << absPathname() << "' (skipping, already read)");
        return true;
    }

    ABCA_ASSERT( !m_read, "already read" );

    TRACE("[OrData " << *this << "] OrData::readFromDisk() path:'" << absPathname() << "' (READING)");
    ABCA_ASSERT( m_group, "invalid group" );

    m_group->readFromDisk();

    Json::Value root;
    Json::Reader reader;

    std::string jsonPathname = absPathname() + ".json";
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
    ABCA_ASSERT( (v_kind == "Object"), "invalid object kind" );

    // read number and names of properties
    //Util::uint32_t v_num_properties = root.get("num_properties", 0).asUInt();
    Json::Value v_properties = root["properties"];
    std::vector<std::string> properties;
    for (Json::Value::iterator it = v_properties.begin(); it != v_properties.end(); ++it)
    {
        properties.push_back( (*it).asString() );
    }

    // read number and names of children
    //Util::uint32_t v_num_children = root.get("num_children", 0).asUInt();
    Json::Value v_children = root["children"];
    std::vector<std::string> children;
    for (Json::Value::iterator it = v_children.begin(); it != v_children.end(); ++it)
    {
        children.push_back( (*it).asString() );
    }

    TODO("read properties");
    TRACE("[OrData " << *this << "] # properties: " << properties.size());
    for (std::vector<std::string>::const_iterator it = properties.begin(); it != properties.end(); ++it)
    {
        TRACE("  property: '" << *it << "'");
    }

    TODO("read children");
    TRACE("[OrData " << *this << "] # children: " << children.size());
    for (std::vector<std::string>::const_iterator it = children.begin(); it != children.end(); ++it)
    {
        TRACE("  child: '" << *it << "'");
    }

    m_read = true;

    ABCA_ASSERT( m_read, "data not read" );
    TRACE("[OrData " << *this << "]  completed read from disk");
    return true;
}

std::string OrData::repr(bool extended) const
{
    std::ostringstream ss;
    if (extended)
    {
        ss << "<OrData(name:'" << name() << "')>";
    } else
    {
        ss << "'" << name() << "'";
    }
    return ss.str();
}

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreGit
} // End namespace Alembic
