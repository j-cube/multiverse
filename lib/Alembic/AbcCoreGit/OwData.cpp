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

#include <Alembic/AbcCoreGit/OwData.h>
#include <Alembic/AbcCoreGit/OwImpl.h>
#include <Alembic/AbcCoreGit/CpwData.h>
#include <Alembic/AbcCoreGit/CpwImpl.h>
#include <Alembic/AbcCoreGit/AwImpl.h>
#include <Alembic/AbcCoreGit/ReadWriteUtil.h>
#include <Alembic/AbcCoreGit/Utils.h>
#include <iostream>
#include <fstream>
#include <string>

#include <Alembic/AbcCoreGit/JSON.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
OwData::OwData( GitGroupPtr iGroup,
                ObjectHeaderPtr iHeader ) :
      m_group( iGroup )
    , m_header( iHeader )
    , m_written( false )
{
    // Check validity of all inputs.
    ABCA_ASSERT( m_group, "Invalid group" );
    ABCA_ASSERT( m_header, "Invalid header" );

    TRACE("OwData::OwData(group:'" << m_group->fullname() << "', name:'" << m_header->getName() << "')");

    // Create the Git group corresponding to this object.
    //m_group.reset( new GitGroup( iParentGroup, m_header->getName() ) );
    GitGroupPtr cpw_group = m_group->addGroup( ".prop" );
    ABCA_ASSERT( cpw_group,
                 "Could not create group for top compound property '.prop' in object: " << m_header->getName() );
    m_data = Alembic::Util::shared_ptr<CpwData>(
        new CpwData( ".prop", cpw_group ) );

    AbcA::PropertyHeader topHeader( ".prop", m_header->getMetaData() );
    UNIMPLEMENTED("WritePropertyInfo()");
#if 0
    WritePropertyInfo( m_group, topHeader, false, 0, 0, 0, 0 );
#endif /* 0 */
}

//-*****************************************************************************
OwData::~OwData()
{
    writeToDisk();

	// not necessary actually
	m_group.reset();
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

Alembic::Util::shared_ptr< AwImpl > OwData::getArchiveImpl() const
{
    AbcA::CompoundPropertyWriterPtr top = m_top.lock();
    ABCA_ASSERT( top, "must have top CompoundPropertyWriter set" );

    Util::shared_ptr< CpwImpl > cpw =
       Alembic::Util::dynamic_pointer_cast< CpwImpl,
        AbcA::CompoundPropertyWriter > ( top );
    return cpw->getArchiveImpl();
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
                            //m_group,
                            m_group->addGroup( iHeader.getName() ),
                            header, m_childHeaders.size() ) );

    m_childHeaders.push_back( header );
    m_madeChildren[iHeader.getName()] = WeakOwPtr( ret );

    m_hashes.push_back(0);
    m_hashes.push_back(0);

    return ret;
}

//-*****************************************************************************
void OwData::writeHeaders( MetaDataMapPtr iMetaDataMap,
                           Util::SpookyHash & ioHash )
{
    std::vector< Util::uint8_t > data;

    // pack all object header into data here
    for ( size_t i = 0; i < m_childHeaders.size(); ++i )
    {
        WriteObjectHeader( data, *m_childHeaders[i], iMetaDataMap );
    }

    Util::SpookyHash dataHash;
    dataHash.Init( 0, 0 );
    m_data->computeHash( dataHash );

    Util::uint64_t hashes[4];
    dataHash.Final( &hashes[0], &hashes[1] );

    ioHash.Init( 0, 0 );

    if ( !m_hashes.empty() )
    {
        ioHash.Update( &m_hashes.front(), m_hashes.size() * 8 );
        ioHash.Final( &hashes[2], &hashes[3] );
    }
    else
    {
        hashes[2] = 0;
        hashes[3] = 0;
    }

    // add the  data hash and child hash for writing
    Util::uint8_t * hashData = ( Util::uint8_t * ) hashes;
    for ( size_t i = 0; i < 32; ++i )
    {
        data.push_back( hashData[i] );
    }

    // now update childHash with dataHash
    // SpookyHash has the nice property that Final doesn't invalidate the hash
    ioHash.Update( hashes, 16 );

    if ( !data.empty() )
    {
        m_group->addData( data.size(), &( data.front() ) );
    }

    m_data->writePropertyHeaders( iMetaDataMap );
}

void OwData::fillHash( std::size_t iIndex, Util::uint64_t iHash0,
                       Util::uint64_t iHash1 )
{
    ABCA_ASSERT( iIndex < m_childHeaders.size() &&
                 iIndex * 2 < m_hashes.size(),
                 "Invalid property index requested in OwData::fillHash" );

    m_hashes[ iIndex * 2     ] = iHash0;
    m_hashes[ iIndex * 2 + 1 ] = iHash1;
}

void OwData::writeToDisk()
{
    TRACE("OwData::writeToDisk() path:'" << absPathname() << "'");
    ABCA_ASSERT( m_group, "invalid group" );
    m_group->writeToDisk();

    if (! m_written)
    {
        TRACE("OwData::writeToDisk() path:'" << absPathname() << "' (WRITING)");

        TRACE("create '" << absPathname() << ".json'");

        double t_end, t_start = time_us();

         rapidjson::Document document;
        // define the document as an object rather than an array
        document.SetObject();
        // must pass an allocator when the object may need to allocate memory
        rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

        JsonSet(document, "name", name());
        JsonSet(document, "kind", "Object");

        assert(name() == m_header->getName());
        ABCA_ASSERT(name() == m_header->getName(), "group name differs from header name!");

        //JsonSet(document, "hName", m_header->getName());
        JsonSet(document, "fullName", m_header->getFullName());
        JsonSet(document, "metadata", m_header->getMetaData().serialize());

        rapidjson::Value jsonChildrenNames( rapidjson::kArrayType );

        Util::uint32_t numChildren = getNumChildren();
        JsonSet(document, "num_children", numChildren);
        for ( Util::uint32_t i = 0; i < numChildren; ++i )
        {
            const AbcA::ObjectHeader& childHeader = getChildHeader( i );

            std::string n = childHeader.getName();
            rapidjson::Value v(n.c_str(), n.length(), allocator);
            jsonChildrenNames.PushBack( v, allocator );
        }
        JsonSet(document, "children", jsonChildrenNames);

        if ( ! m_data )
        {
            TRACE("OwData::writeToDisk() top compound pointer not available!");

            JsonSet(document, "num_properties", 0);

            rapidjson::Value jsonPropertiesNames( rapidjson::kArrayType );
            JsonSet(document, "properties", jsonPropertiesNames);
        } else
        {
            JsonSet(document, "num_properties", 1);

            rapidjson::Value jsonPropertiesNames( rapidjson::kArrayType );

            std::string n = m_data->name();
            rapidjson::Value v(n.c_str(), n.length(), allocator);
            jsonPropertiesNames.PushBack( v, allocator );

            JsonSet(document, "properties", jsonPropertiesNames);
        }
        t_end = time_us();
        Profile::add_json_creation(t_end - t_start);

        t_start = time_us();
        std::string output = JsonWrite(document);
        t_end = time_us();
        Profile::add_json_output(t_end - t_start);

#if JSON_TO_DISK
        t_start = time_us();
        std::string jsonPathname = absPathname() + ".json";
        std::ofstream jsonFile;
        jsonFile.open(jsonPathname.c_str(), std::ios::out | std::ios::trunc);
        jsonFile << output;
        jsonFile.close();
        t_end = time_us();
        Profile::add_disk_write(t_end - t_start);

        GitRepoPtr repo_ptr = m_group->repo();
        ABCA_ASSERT( repo_ptr, "invalid git repository object");

        t_start = time_us();
        std::string jsonRelPathname = repo_ptr->relpath(jsonPathname);
        repo_ptr->add(jsonRelPathname);
        t_end = time_us();
        Profile::add_git(t_end - t_start);
#else
        t_start = time_us();
        GitGroupPtr parentGroup = m_group->parent();
        parentGroup->add_file_from_memory(name() + ".json", output);
        t_end = time_us();
        Profile::add_git(t_end - t_start);
#endif

        m_written = true;
    } else
    {
        TRACE("OwData::writeToDisk() path:'" << absPathname() << "' (skipping, already written)");
    }

    ABCA_ASSERT( m_written, "data not written" );
}


} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreGit
} // End namespace Alembic
