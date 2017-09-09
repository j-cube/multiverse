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

#include <Alembic/AbcCoreGit/CpwImpl.h>
#include <Alembic/AbcCoreGit/AwImpl.h>
#include <Alembic/AbcCoreGit/ReadWriteUtil.h>
#include <Alembic/AbcCoreGit/Utils.h>
#include <Alembic/AbcCoreGit/SampleStore.h>

#include <iostream>
#include <fstream>

#include <Alembic/AbcCoreGit/JSON.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************

// "top" compound creation called by object writers.
CpwImpl::CpwImpl( AbcA::ObjectWriterPtr iParent,
                  CpwDataPtr iData,
                  const AbcA::MetaData & iMeta )
    : m_object( iParent)
    , m_header( new PropertyHeaderAndFriends( "", iMeta ) )
    , m_data( iData )
    , m_index( 0 )
    , m_written( false )
{
    // TRACE("CpwImpl::CpwImpl(ObjectWriterPtr) TOP");

    {
        GitGroupPtr ourGroupPtr;
        if (m_data)
            ourGroupPtr = m_data->getGroup();

        if (iParent && ourGroupPtr)
            TRACE("CpwImpl::CpwImpl(ObjectWriterPtr - parent:" << CONCRETE_OWPTR(iParent)->repr(true) << ", ourGroup:" << ourGroupPtr->repr() << ", header:'" << m_header->name() << "', index:" << m_index << ")");
        else if (iParent)
            TRACE("CpwImpl::CpwImpl(ObjectWriterPtr - parent:" << CONCRETE_OWPTR(iParent)->repr(true) << ", ourGroup:--, header:'" << m_header->name() << "', index:" << m_index << ")");
        else if (ourGroupPtr)
            TRACE("CpwImpl::CpwImpl(ObjectWriterPtr - parent:--, ourGroup:" << ourGroupPtr->repr() << ", header:'" << m_header->name() << "', index:" << m_index << ")");
        else
            TRACE("CpwImpl::CpwImpl(ObjectWriterPtr - parent:--, ourGroup:--, header:'" << m_header->name() << "', index:" << m_index << ")");
    }

    // always set isScalarLike to false for compound objects
    ABCA_ASSERT( m_header, "Header not allocated" );
    m_header->isScalarLike = false;

    // we don't need to write the property info, the object has done it already

    ABCA_ASSERT( m_object, "Invalid object" );
    ABCA_ASSERT( m_data, "Invalid compound data" );
}

// With the compound property writer as an input.
CpwImpl::CpwImpl( AbcA::CompoundPropertyWriterPtr iParent,
                  GitGroupPtr iParentGroup,
                  PropertyHeaderPtr iHeader,
                  size_t iIndex )
  : m_parent( iParent )
  , m_header( iHeader )
  , m_index( iIndex )
  , m_written( false )
{
    // Check the validity of all inputs.
    ABCA_ASSERT( m_parent, "Invalid parent" );
    ABCA_ASSERT( m_header, "Invalid header" );

    if (iParentGroup)
        TRACE("CpwImpl::CpwImpl(parent:" << CONCRETE_CPWPTR(m_parent)->repr(true) << ", parentGroup:" << iParentGroup->repr() << ", header:'" << m_header->name() << "', index:" << m_index << ")");
    else
        TRACE("CpwImpl::CpwImpl(parent:" << CONCRETE_CPWPTR(m_parent)->repr(true) << ", parentGroup:NULL, name:'" << m_header->name() << "', index:" << m_index << ")");

    // Set the object.
    m_object = m_parent->getObject();
    ABCA_ASSERT( m_object, "Invalid parent object" );

    ABCA_ASSERT( m_header->header.getName() != "" &&
                 m_header->header.getName().find('/') == std::string::npos,
                 "Invalid name" );

    GitGroupPtr iGroup = iParentGroup->addGroup( m_header->name() );

    m_data.reset( new CpwData( m_header->name(), iGroup ) );

    // always set isScalarLike to false for compound objects
    m_header->isScalarLike = false;

    // Write the property header.
    UNIMPLEMENTED("WritePropertyInfo()");
#if 0
    WritePropertyInfo( iParentGroup, m_header, false, 0, 0, 0, 0 );
#endif
}

//-*****************************************************************************
CpwImpl::~CpwImpl()
{
    // objects are responsible for calling this on the CpWData they own
    // as part of their "top" compound
    if ( m_parent )
    {
        MetaDataMapPtr mdMap = Alembic::Util::dynamic_pointer_cast<
            AwImpl, AbcA::ArchiveWriter >(
                getObject()->getArchive() )->getMetaDataMap();
        m_data->writePropertyHeaders( mdMap );

        Util::SpookyHash hash;
        hash.Init( 0, 0 );
        m_data->computeHash( hash );
        HashPropertyHeader( m_header->header, hash );

        Util::uint64_t hash0, hash1;
        hash.Final( &hash0, &hash1 );
        Util::shared_ptr< CpwImpl > parent =
            Alembic::Util::dynamic_pointer_cast< CpwImpl,
                AbcA::CompoundPropertyWriter > ( m_parent );
        parent->fillHash( m_index, hash0, hash1 );
    }

    writeToDisk();
}

//-*****************************************************************************
const AbcA::PropertyHeader &CpwImpl::getHeader() const
{
    return m_header->header;
}

//-*****************************************************************************
AbcA::ObjectWriterPtr CpwImpl::getObject()
{
    return m_object;
}

//-*****************************************************************************
AbcA::CompoundPropertyWriterPtr CpwImpl::getParent()
{
    // this will be NULL for "top" compound properties
    return m_parent;
}

//-*****************************************************************************
AbcA::CompoundPropertyWriterPtr CpwImpl::asCompoundPtr()
{
    return shared_from_this();
}

//-*****************************************************************************
size_t CpwImpl::getNumProperties()
{
    return m_data->getNumProperties();
}

//-*****************************************************************************
const AbcA::PropertyHeader & CpwImpl::getPropertyHeader( size_t i )
{
    return m_data->getPropertyHeader( i );
}

//-*****************************************************************************
const AbcA::PropertyHeader *
CpwImpl::getPropertyHeader( const std::string &iName )
{
    return m_data->getPropertyHeader( iName );
}

//-*****************************************************************************
AbcA::BasePropertyWriterPtr CpwImpl::getProperty( const std::string & iName )
{
    return m_data->getProperty( iName );
}

//-*****************************************************************************
AbcA::ScalarPropertyWriterPtr
CpwImpl::createScalarProperty( const std::string & iName,
                      const AbcA::MetaData & iMetaData,
                      const AbcA::DataType & iDataType,
                      uint32_t iTimeSamplingIndex )
{
    TRACE("call m_data->createScalarProperty()");
    return m_data->createScalarProperty( asCompoundPtr(), iName, iMetaData,
                                         iDataType, iTimeSamplingIndex );
}

//-*****************************************************************************
AbcA::ArrayPropertyWriterPtr
CpwImpl::createArrayProperty( const std::string & iName,
                              const AbcA::MetaData & iMetaData,
                              const AbcA::DataType & iDataType,
                              uint32_t iTimeSamplingIndex )
{
    TRACE("call m_data->createArrayProperty()");
    return m_data->createArrayProperty( asCompoundPtr(), iName, iMetaData,
                                        iDataType, iTimeSamplingIndex );
}

//-*****************************************************************************
AbcA::CompoundPropertyWriterPtr
CpwImpl::createCompoundProperty( const std::string & iName,
                                 const AbcA::MetaData & iMetaData )
{
    TRACE("call m_data->createCompoundProperty");
    return m_data->createCompoundProperty( asCompoundPtr(), iName, iMetaData );
}

void CpwImpl::fillHash( std::size_t iIndex, Util::uint64_t iHash0,
                        Util::uint64_t iHash1 )
{
    m_data->fillHash( iIndex, iHash0, iHash1 );
}

CpwImplPtr CpwImpl::getTParent() const
{
    Util::shared_ptr< CpwImpl > parent =
       Alembic::Util::dynamic_pointer_cast< CpwImpl,
        AbcA::CompoundPropertyWriter > ( m_parent );
    return parent;
}

std::string CpwImpl::repr(bool extended) const
{
    std::ostringstream ss;
    if (extended)
    {
        if (m_parent)
        {
            CpwImplPtr parentPtr = getTParent();

            ss << "<CpwImpl(parent:" << parentPtr->repr() << ", header:'" << m_header->name() << "')>";
        } else
        {
            ss << "<CpwImpl(TOP, " << ", header:'" << m_header->name() << "')>";
        }
    } else
    {
        ss << "'" << m_header->name() << "'";
    }
    return ss.str();
}

void CpwImpl::writeToDisk()
{
    TRACE("CpwImpl::writeToDisk() path:'" << absPathname() << "'");
    ABCA_ASSERT( m_data, "invalid data" );
    m_data->writeToDisk();

    if (! m_written)
    {
        TRACE("CpwImpl::writeToDisk() path:'" << absPathname() << "' (WRITING)");

        TRACE("create '" << absPathname() << ".json'");

        double t_end, t_start = time_us();

        rapidjson::Document document;
        // define the document as an object rather than an array
        document.SetObject();
        // must pass an allocator when the object may need to allocate memory
        rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

//        const AbcA::MetaData& metaData = m_header->metadata();

        JsonSet(document, "name", m_header->name());
        JsonSet(document, "kind", "CompoundProperty");

        JsonSet(document, "index", m_index);

        const AbcA::DataType& dataType = m_header->datatype();
        {
            std::ostringstream ss;
            ss << PODName( dataType.getPod() );
            JsonSet(document, "typename", ss.str());
        }

        JsonSet(document, "extent", dataType.getExtent());

        {
            std::ostringstream ss;
            ss << dataType;
            JsonSet(document, "type", ss.str());
        }

        size_t numProperties = getNumProperties();
        JsonSet(document, "num_properties", numProperties);

        rapidjson::Value jsonPropertiesNames( rapidjson::kArrayType );
        for ( size_t i = 0; i < numProperties; ++i )
        {
            const AbcA::PropertyHeader& propHeader = getPropertyHeader( i );

            std::string n = propHeader.getName();
            rapidjson::Value v(n.c_str(), n.length(), allocator);
            jsonPropertiesNames.PushBack( v, allocator );
        }
        JsonSet(document, "properties", jsonPropertiesNames);

        rapidjson::Value propInfo( rapidjson::kObjectType );
        JsonSet(document, propInfo, "isScalarLike", m_header->isScalarLike);
        JsonSet(document, propInfo, "isHomogenous", m_header->isHomogenous);
        JsonSet(document, propInfo, "timeSamplingIndex", m_header->timeSamplingIndex);
        JsonSet(document, propInfo, "numSamples", m_header->nextSampleIndex);
        JsonSet(document, propInfo, "firstChangedIndex", m_header->firstChangedIndex);
        JsonSet(document, propInfo, "lastChangedIndex", m_header->lastChangedIndex);
        JsonSet(document, propInfo, "metadata", m_header->header.getMetaData().serialize());

        JsonSet(document, "info", propInfo);

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

        GitRepoPtr repo_ptr = m_data->getGroup()->repo();
        ABCA_ASSERT( repo_ptr, "invalid git repository object");

        t_start = time_us();
        std::string jsonRelPathname = repo_ptr->relpath(jsonPathname);
        repo_ptr->add(jsonRelPathname);
        t_end = time_us();
        Profile::add_git(t_end - t_start);
#else
        t_start = time_us();
        GitGroupPtr parentGroup = m_data->getGroup()->parent();
        parentGroup->add_file_from_memory(name() + ".json", output);
        t_end = time_us();
        Profile::add_git(t_end - t_start);
#endif

        m_written = true;
    } else
    {
        TRACE("CpwImpl::writeToDisk() path:'" << absPathname() << "' (skipping, already written)");
    }

    ABCA_ASSERT( m_written, "data not written" );
}

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreGit
} // End namespace Alembic

