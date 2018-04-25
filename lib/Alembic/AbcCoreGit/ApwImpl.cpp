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

#include <Alembic/AbcCoreGit/ApwImpl.h>
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
ApwImpl::ApwImpl( AbcA::CompoundPropertyWriterPtr iParent,
                  GitGroupPtr iGroup,
                  PropertyHeaderPtr iHeader,
                  size_t iIndex ) :
    m_parent( iParent ), m_header( iHeader ), m_group( iGroup ), m_dims( 1 ),
    m_index( iIndex ),
    m_written(false)
{
    ABCA_ASSERT( m_parent, "Invalid parent" );
    ABCA_ASSERT( m_header, "Invalid property header" );
    ABCA_ASSERT( m_group, "Invalid group" );

    TRACE("ApwImpl::ApwImpl(parent:" << CONCRETE_CPWPTR(m_parent)->repr() << ", group:" << m_group->repr() << ", header:'" << m_header->name() << "', index:" << m_index << ")");

    if ( m_header->header.getPropertyType() != AbcA::kArrayProperty )
    {
        ABCA_THROW( "Attempted to create a ArrayPropertyWriter from a "
                    "non-array property type" );
    }
}


//-*****************************************************************************
ApwImpl::~ApwImpl()
{
    AbcA::ArchiveWriterPtr archive = m_parent->getObject()->getArchive();

    index_t maxSamples = archive->getMaxNumSamplesForTimeSamplingIndex(
            m_header->timeSamplingIndex );

    Util::uint32_t numSamples = m_header->nextSampleIndex;

    // a constant property, we wrote the same sample over and over
    if ( m_header->lastChangedIndex == 0 && m_header->nextSampleIndex > 0 )
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
void ApwImpl::setFromPreviousSample()
{
    // Set the next sample to equal the previous sample. An important feature!

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

    ABCA_ASSERT(m_store,
        "Can't set from previous sample before any samples have been written" );

    m_store->setFromPreviousSample();

    // Util::Digest digest = m_previousWrittenSampleID->getKey().digest;
    // HashDimensions( m_dims, digest );
    // Util::SpookyHash::ShortEnd(m_hash.words[0], m_hash.words[1],
    //                           digest.words[0], digest.words[1]);
    m_header->nextSampleIndex ++;
}

//-*****************************************************************************
void ApwImpl::setSample( const AbcA::ArraySample & iSamp )
{
    // Make sure we aren't writing more samples than we have times for
    // This applies to acyclic sampling only
    ABCA_ASSERT(
        !m_header->header.getTimeSampling()->getTimeSamplingType().isAcyclic()
        || m_header->header.getTimeSampling()->getNumStoredTimes() >
        m_header->nextSampleIndex,
        "Can not write more samples than we have times for when using "
        "Acyclic sampling." );

    ABCA_ASSERT( iSamp.getDataType() == m_header->header.getDataType(),
        "DataType on ArraySample iSamp: " << iSamp.getDataType() <<
        ", does not match the DataType of the Array property: " <<
        m_header->header.getDataType() );

    // The Key helps us analyze the sample.
    AbcA::ArraySample::Key key = iSamp.getKey();

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

    AbcA::Dimensions dims = iSamp.getDimensions();
    if (dims.numPoints() == 0)
        dims = m_dims;

    // TRACE("ApwImpl::setSample() n:" << m_header->name() << " idx:" << m_header->nextSampleIndex << " key:" << key.digest.str() << " dims:" << dims << " (m_dims:" << m_dims << ") type:" << m_header->datatype());

#if 0
    // We need to write the sample
    if ( m_header->nextSampleIndex == 0  ||
         !( m_previousWrittenSampleID &&
            key == m_previousWrittenSampleID->getKey() ) )
    {
        m_dims = iSamp.getDimensions();

        // if we haven't written this already, isScalarLike will be true
        if ( m_header->isScalarLike && m_dims.numPoints() != 1 )
        {
            m_header->isScalarLike = false;
        }

#if 0
        // if ( m_header->isHomogenous && m_previousWrittenSampleID &&
        //      (m_dims.numPoints() != m_previousWrittenSampleID->getNumPoints()) )
        // {
        //     m_header->isHomogenous = false;
        // }

        // if (m_header->firstChangedIndex == 0)
        // {
        //     m_header->firstChangedIndex = m_header->nextSampleIndex;
        // }
#endif /* 0 */
    }

    if (! m_store.get())
    {
        m_store.reset( BuildSampleStore( getArchiveImpl(), m_header->datatype(), dims ) );
    }
    m_store->addSample( iSamp );
#endif

    // write the sample
    // (for some reason, we must always do it unconditionally here, to prevent
    //  an assertion in samplestore and/or discrepancies in json metadata)
    if (! m_store.get())
    {
        m_store.reset( BuildSampleStore( getArchiveImpl(), m_header->datatype(), dims ) );
    }
    size_t where = m_store->addSample( iSamp );

    if ( (m_header->nextSampleIndex == 0) ||
         (! (m_previousWrittenSampleID && (key == m_previousWrittenSampleID->getKey()))) )
    {
        // We need to write the sample

        // store the previous written sample id
        AbcA::ArchiveWriterPtr awp = this->getObject()->getArchive();
        m_previousWrittenSampleID = getWrittenSampleID( GetWrittenSampleMap(awp), iSamp, key, where );

        m_dims = iSamp.getDimensions();

        // if we haven't written this already, isScalarLike will be true
        if (m_header->isScalarLike && (m_dims.numPoints() != 1))
        {
            m_header->isScalarLike = false;
        }

        if ( m_header->isHomogenous && m_previousWrittenSampleID &&
             (m_dims.numPoints() != m_previousWrittenSampleID->getNumPoints()) )
        {
            m_header->isHomogenous = false;
        }

        if (m_header->firstChangedIndex == 0)
        {
            m_header->firstChangedIndex = m_header->nextSampleIndex;
        }

        // this index is now the last change
        m_header->lastChangedIndex = m_header->nextSampleIndex;
    }

#if 0
    Util::Digest digest = m_previousWrittenSampleID->getKey().digest;
    HashDimensions( m_dims, digest );
    if ( m_header->nextSampleIndex == 0 )
    {
        m_hash = digest;
    }
    else
    {
        Util::SpookyHash::ShortEnd(m_hash.words[0], m_hash.words[1],
                                   digest.words[0], digest.words[1]);
    }
#endif /* 0 */

    // m_header->lastChangedIndex = m_header->nextSampleIndex;
    m_header->nextSampleIndex ++;
}

//-*****************************************************************************
AbcA::ArrayPropertyWriterPtr ApwImpl::asArrayPtr()
{
    return shared_from_this();
}

//-*****************************************************************************
size_t ApwImpl::getNumSamples()
{
    return ( size_t )m_header->nextSampleIndex;
}

//-*****************************************************************************
void ApwImpl::setTimeSamplingIndex( Util::uint32_t iIndex )
{
    // will assert if TimeSamplingPtr not found
    AbcA::TimeSamplingPtr ts =
        m_parent->getObject()->getArchive()->getTimeSampling(
            iIndex );

    ABCA_ASSERT( !ts->getTimeSamplingType().isAcyclic() ||
        ts->getNumStoredTimes() >= m_header->nextSampleIndex,
        "Already have written more samples than we have times for when using "
        "Acyclic sampling." );

    m_header->header.setTimeSampling(ts);
    m_header->timeSamplingIndex = iIndex;
}

//-*****************************************************************************
const AbcA::PropertyHeader & ApwImpl::getHeader() const
{
    ABCA_ASSERT( m_header, "Invalid header" );
    return m_header->header;
}

//-*****************************************************************************
AbcA::ObjectWriterPtr ApwImpl::getObject()
{
    ABCA_ASSERT( m_parent, "Invalid parent" );
    return m_parent->getObject();
}

//-*****************************************************************************
AbcA::CompoundPropertyWriterPtr ApwImpl::getParent()
{
    ABCA_ASSERT( m_parent, "Invalid parent" );
    return m_parent;
}

CpwImplPtr ApwImpl::getTParent() const
{
    Util::shared_ptr< CpwImpl > parent =
       Alembic::Util::dynamic_pointer_cast< CpwImpl,
        AbcA::CompoundPropertyWriter > ( m_parent );
    return parent;
}

std::string ApwImpl::repr(bool extended) const
{
    std::ostringstream ss;
    if (extended)
    {
        CpwImplPtr parentPtr = getTParent();

        ss << "<ApwImpl(parent:" << parentPtr->repr() << ", header:'" << m_header->name() << "', group:" << m_group->repr() << ")>";
    } else
    {
        ss << "'" << m_header->name() << "'";
    }
    return ss.str();
}

std::string ApwImpl::relPathname() const
{
    ABCA_ASSERT(m_group, "invalid group");

    std::string parent_path = m_group->relPathname();
    if (parent_path == "/")
    {
        return parent_path + name();
    } else
    {
        return parent_path + "/" + name();
    }
}

std::string ApwImpl::absPathname() const
{
    ABCA_ASSERT(m_group, "invalid group");

    std::string parent_path = m_group->absPathname();
    if (parent_path == "/")
    {
        return parent_path + name();
    } else
    {
        return parent_path + "/" + name();
    }
}

void ApwImpl::writeToDisk()
{
    if (! m_written)
    {
        TRACE("ApwImpl::writeToDisk() path:'" << absPathname() << "' (WRITING)");

        ABCA_ASSERT( m_group, "invalid group" );
        m_group->writeToDisk();

        TRACE("create '" << absPathname() << "'");

        ensureSampleStore();

        ABCA_ASSERT( m_store.get(),
            "SampleStore not present!" );

        ABCA_ASSERT( m_header->nextSampleIndex == m_store->getNumSamples(),
                     "invalid number of samples in SampleStore!" );

        double t_end, t_start = time_us();

        rapidjson::Document document;
        // define the document as an object rather than an array
        document.SetObject();


//        const AbcA::MetaData& metaData = m_header->metadata();
        const AbcA::DataType& dataType = m_header->datatype();

        JsonSet(document, "name", m_header->name());
        JsonSet(document, "kind", "ArrayProperty");

        {
            std::ostringstream ss;
            ss << PODName( dataType.getPod() );
            JsonSet(document, "typename", ss.str());
        }

        assert( dataType.getExtent() == m_store->extent() );

        JsonSet(document, "extent", dataType.getExtent());
#if 0
        JsonSet(document, "rank", m_store->rank());
#endif

        {
            std::ostringstream ss;
            ss << dataType;
            JsonSet(document, "type", ss.str());
        }

        JsonSet(document, "num_samples", m_store->getNumSamples());

        // JsonSet(document, "dimensions", m_store->getDimensions());

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
        TRACE("ApwImpl::writeToDisk() samples-to-msgpack: " << (t_end - t_start) << "us");
        t_start = time_us();
        m_group->add_file_from_memory(name() + ".bin", packedSamples);
        t_end = time_us();
        Profile::add_git(t_end - t_start);
#endif /* MSGPACK_SAMPLES */
#endif

        m_written = true;
    } else
    {
        TRACE("ApwImpl::writeToDisk() path:'" << absPathname() << "' (skipping, already written)");
    }

    ABCA_ASSERT( m_written, "data not written" );
}

Alembic::Util::shared_ptr< AwImpl > ApwImpl::getArchiveImpl() const
{
    ABCA_ASSERT( m_parent, "must have parent CompoundPropertyWriter set" );

    Util::shared_ptr< CpwImpl > cpw =
       Alembic::Util::dynamic_pointer_cast< CpwImpl,
        AbcA::CompoundPropertyWriter > ( m_parent );
    return cpw->getArchiveImpl();
}

void ApwImpl::ensureSampleStore()
{
    if (! m_store.get())
    {
        m_store.reset( BuildSampleStore( getArchiveImpl(), m_header->datatype(), AbcA::Dimensions() ) );
    }
}

void ApwImpl::ensureSampleStore(const AbcA::Dimensions &iDims)
{
    if (! m_store.get())
    {
        m_store.reset( BuildSampleStore( getArchiveImpl(), m_header->datatype(), iDims ) );
    }
}

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreGit
} // End namespace Alembic
