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

#include <Alembic/AbcCoreGit/SpwImpl.h>
#include <Alembic/AbcCoreGit/CpwImpl.h>
#include <Alembic/AbcCoreGit/ReadWriteUtil.h>
#include <Alembic/AbcCoreGit/Utils.h>

#include <iostream>
#include <fstream>

#include <Alembic/AbcCoreGit/JSON.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
SpwImpl::SpwImpl( AbcA::CompoundPropertyWriterPtr iParent,
                  GitGroupPtr iGroup,
                  PropertyHeaderPtr iHeader,
                  size_t iIndex ) :
    m_parent( iParent ), m_header( iHeader ),
    m_rank0_dims( /* rank-0 */ AbcA::Dimensions() ),
    m_store( BuildSampleStore( getCpwImplPtr(iParent)->getArchiveImpl(), iHeader->datatype(), /* rank-0 */ AbcA::Dimensions() ) ),
    m_group( iGroup ),
    m_index( iIndex ),
    m_written(false)
{
    ABCA_ASSERT( m_parent, "Invalid parent" );
    ABCA_ASSERT( m_header, "Invalid property header" );
    ABCA_ASSERT( m_group, "Invalid group" );

    TRACE("SpwImpl::SpwImpl(parent:" << CONCRETE_CPWPTR(m_parent)->repr() << ", group:" << m_group->repr() << ", header:'" << m_header->name() << "', index:" << m_index << ")");

    if ( m_header->header.getPropertyType() != AbcA::kScalarProperty )
    {
        ABCA_THROW( "Attempted to create a ScalarPropertyWriter from a "
                    "non-scalar property type" );
    }
}


//-*****************************************************************************
SpwImpl::~SpwImpl()
{
    AbcA::ArchiveWriterPtr archive = m_parent->getObject()->getArchive();

    index_t maxSamples = archive->getMaxNumSamplesForTimeSamplingIndex(
            m_header->timeSamplingIndex );

    Util::uint32_t numSamples = m_header->nextSampleIndex;

    // a constant property, we wrote the same sample over and over
    if ( m_header->lastChangedIndex == 0 && numSamples > 0 )
    {
        numSamples = 1;
    }

    if ( maxSamples < numSamples )
    {
        archive->setMaxNumSamplesForTimeSamplingIndex(
            m_header->timeSamplingIndex, numSamples );
    }

    Util::SpookyHash hash;
    hash.Init(0, 0);
    HashPropertyHeader( m_header->header, hash );

    // mix in the accumulated hash if we have samples
    if ( numSamples != 0 )
    {
        hash.Update( m_hash.d, 16 );
    }

    Util::uint64_t hash0, hash1;
    hash.Final( &hash0, &hash1 );
    Util::shared_ptr< CpwImpl > parent =
        Alembic::Util::dynamic_pointer_cast< CpwImpl,
            AbcA::CompoundPropertyWriter > ( m_parent );
    parent->fillHash( m_index, hash0, hash1 );

    writeToDisk();
}

//-*****************************************************************************
void SpwImpl::setFromPreviousSample()
{

    // Make sure we aren't writing more samples than we have times for
    // This applies to acyclic sampling only
    ABCA_ASSERT(
        !m_header->header.getTimeSampling()->getTimeSamplingType().isAcyclic()
        || m_header->header.getTimeSampling()->getNumStoredTimes() >
        m_header->nextSampleIndex,
        "Can not set more samples than we have times for when using "
        "Acyclic sampling." );

    ABCA_ASSERT( m_header->nextSampleIndex > 0,
        "Can't set from previous sample before any samples have been written" );

    ABCA_ASSERT( m_store.get(),
        "Can't set from previous sample before any samples have been written" );

    m_store->setFromPreviousSample();

    // Util::Digest digest = m_previousWrittenSampleID->getKey().digest;
    // Util::SpookyHash::ShortEnd(m_hash.words[0], m_hash.words[1],
    //                            digest.words[0], digest.words[1]);
    m_header->nextSampleIndex ++;
}

//-*****************************************************************************
void SpwImpl::setSample( const void *iSamp )
{
    // Make sure we aren't writing more samples than we have times for
    // This applies to acyclic sampling only
    ABCA_ASSERT(
        !m_header->header.getTimeSampling()->getTimeSamplingType().isAcyclic()
        || m_header->header.getTimeSampling()->getNumStoredTimes() >
        m_header->nextSampleIndex,
        "Can not write more samples than we have times for when using "
        "Acyclic sampling." );

    AbcA::ArraySample samp( iSamp, m_header->header.getDataType(),
                            AbcA::Dimensions(1) );

     // The Key helps us analyze the sample.
     AbcA::ArraySample::Key key = samp.getKey();

     // mask out the non-string POD since Git can safely share the same data
     // even if it originated from a different POD
     // the non-fixed sizes of our strings (plus added null characters) makes
     // determing the sie harder so strings are handled seperately
    if ( key.origPOD != Alembic::Util::kStringPOD &&
         key.origPOD != Alembic::Util::kWstringPOD )
    {
        key.origPOD = Alembic::Util::kInt8POD;
        key.readPOD = Alembic::Util::kInt8POD;
    }

#if 0
    m_store->addSample( iSamp, key );
#endif

    // write the sample
    // (for some reason, we must always do it unconditionally here, to prevent
    //  an assertion in samplestore)
    // as an optimization, we re-use our rank-0 AbcA::Dimensions() copy
    // instead of re-creating one each time
    size_t where = m_store->addSample( iSamp, key, m_rank0_dims );

    if ( (m_header->nextSampleIndex == 0)  ||
         (! (m_previousWrittenSampleID && (key == m_previousWrittenSampleID->getKey()))) )
    {
        // We need to write the sample

        // store the previous written sample id
        AbcA::ArchiveWriterPtr awp = this->getObject()->getArchive();
        m_previousWrittenSampleID = getWrittenSampleID( GetWrittenSampleMap(awp), samp, key, where );

        if (m_header->firstChangedIndex == 0)
        {
            m_header->firstChangedIndex = m_header->nextSampleIndex;
        }

        // this index is now the last change
        m_header->lastChangedIndex = m_header->nextSampleIndex;
    }

    if ( m_header->nextSampleIndex == 0 )
    {
        m_hash = m_previousWrittenSampleID->getKey().digest;
    }
    else
    {
        Util::Digest digest = m_previousWrittenSampleID->getKey().digest;
        Util::SpookyHash::ShortEnd( m_hash.words[0], m_hash.words[1],
                                    digest.words[0], digest.words[1] );
    }

    // m_header->lastChangedIndex = m_header->nextSampleIndex;
    m_header->nextSampleIndex ++;
}

//-*****************************************************************************
AbcA::ScalarPropertyWriterPtr SpwImpl::asScalarPtr()
{
    return shared_from_this();
}

//-*****************************************************************************
size_t SpwImpl::getNumSamples()
{
    return ( size_t )m_header->nextSampleIndex;
}

//-*****************************************************************************
void SpwImpl::setTimeSamplingIndex( Util::uint32_t iIndex )
{
    // will assert if TimeSamplingPtr not found
    AbcA::TimeSamplingPtr ts =
        m_parent->getObject()->getArchive()->getTimeSampling( iIndex );

    ABCA_ASSERT( !ts->getTimeSamplingType().isAcyclic() ||
        ts->getNumStoredTimes() >= m_header->nextSampleIndex,
        "Already have written more samples than we have times for when using "
        "Acyclic sampling." );

    m_header->header.setTimeSampling(ts);
    m_header->timeSamplingIndex = iIndex;
}

//-*****************************************************************************
const AbcA::PropertyHeader & SpwImpl::getHeader() const
{
    ABCA_ASSERT( m_header, "Invalid header" );
    return m_header->header;
}

//-*****************************************************************************
AbcA::ObjectWriterPtr SpwImpl::getObject()
{
    ABCA_ASSERT( m_parent, "Invalid parent" );
    return m_parent->getObject();
}

//-*****************************************************************************
AbcA::CompoundPropertyWriterPtr SpwImpl::getParent()
{
    ABCA_ASSERT( m_parent, "Invalid parent" );
    return m_parent;
}

CpwImplPtr SpwImpl::getTParent() const
{
    Util::shared_ptr< CpwImpl > parent =
       Alembic::Util::dynamic_pointer_cast< CpwImpl,
        AbcA::CompoundPropertyWriter > ( m_parent );
    return parent;
}

std::string SpwImpl::repr(bool extended) const
{
    std::ostringstream ss;
    if (extended)
    {
        CpwImplPtr parentPtr = getTParent();

        ss << "<SpwImpl(parent:" << parentPtr->repr() << ", header:'" << m_header->name() << "', group:" << m_group->repr() << ")>";
    } else
    {
        ss << "'" << m_header->name() << "'";
    }
    return ss.str();
}

std::string SpwImpl::relPathname() const
{
    ABCA_ASSERT(m_group, "invalid group");

    std::string rel_path_name;
    std::string parent_path = m_group->relPathname();
    if (parent_path == "/")
    {
        rel_path_name = parent_path + name();
    } else
    {
        rel_path_name = parent_path + "/" + name();
    }
    std::replace(rel_path_name.begin() , rel_path_name.end() , '\\' , '/');
    return rel_path_name;
}

std::string SpwImpl::absPathname() const
{
    ABCA_ASSERT(m_group, "invalid group");

    std::string abs_path_name;
    std::string parent_path = m_group->absPathname();
    if (parent_path == "/")
    {
        abs_path_name = parent_path + name();
    } else
    {
        abs_path_name = parent_path + "/" + name();
    }
    std::replace(abs_path_name.begin() , abs_path_name.end() , '\\' , '/');
    return abs_path_name;
}

void SpwImpl::writeToDisk()
{
    if (! m_written)
    {
        TRACE("SpwImpl::writeToDisk() path:'" << absPathname() << "' (WRITING)");

        ABCA_ASSERT( m_group, "invalid group" );
        m_group->writeToDisk();

        TRACE("create '" << absPathname() << "'");

        ABCA_ASSERT( m_header->nextSampleIndex == m_store->getNumSamples(),
                     "invalid number of samples in SampleStore!" );

        double t_end, t_start = time_us();

         rapidjson::Document document;
        // define the document as an object rather than an array
        document.SetObject();
        // must pass an allocator when the object may need to allocate memory
        // rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

//        const AbcA::MetaData& metaData = m_header->metadata();
        const AbcA::DataType& dataType = m_header->datatype();

        JsonSet(document, "name", m_header->name());
        JsonSet(document, "kind", "ScalarProperty");

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

        JsonSet(document, "num_samples", m_store->getNumSamples());

        //JsonSet(document, "dimensions", m_store->getDimensions());

#if !(MSGPACK_SAMPLES)
        root["data"] = m_store->json();
#endif

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

        GitRepoPtr repo_ptr = m_group->repo();
        ABCA_ASSERT( repo_ptr, "invalid git repository object");

        t_start = time_us();
        std::string jsonRelPathname = repo_ptr->relpath(jsonPathname);
        repo_ptr->add(jsonRelPathname);
        t_end = time_us();
        Profile::add_git(t_end - t_start);
#else
        t_start = time_us();
        m_group->add_file_from_memory(name() + ".json", output);
        t_end = time_us();
        Profile::add_git(t_end - t_start);

#if MSGPACK_SAMPLES
        t_start = time_us();
        std::string packedSamples = m_store->pack();
        t_end = time_us();
        TRACE("SpwImpl::writeToDisk() samples-to-msgpack: " << (t_end - t_start) << "us");
        t_start = time_us();
        m_group->add_file_from_memory(name() + ".bin", packedSamples);
        t_end = time_us();
        Profile::add_git(t_end - t_start);
#endif /* MSGPACK_SAMPLES */
#endif

        m_written = true;
    } else
    {
        TRACE("SpwImpl::writeToDisk() path:'" << absPathname() << "' (skipping, already written)");
    }

    ABCA_ASSERT( m_written, "data not written" );
}

Alembic::Util::shared_ptr< AwImpl > SpwImpl::getArchiveImpl() const
{
    ABCA_ASSERT( m_parent, "must have parent CompoundPropertyWriter set" );

    Util::shared_ptr< CpwImpl > cpw =
       Alembic::Util::dynamic_pointer_cast< CpwImpl,
        AbcA::CompoundPropertyWriter > ( m_parent );
    return cpw->getArchiveImpl();
}

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreGit
} // End namespace Alembic
