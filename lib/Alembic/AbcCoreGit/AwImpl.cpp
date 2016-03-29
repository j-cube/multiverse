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

#include <Alembic/AbcCoreGit/AwImpl.h>
#include <Alembic/AbcCoreGit/OwData.h>
#include <Alembic/AbcCoreGit/OwImpl.h>
#include <Alembic/AbcCoreGit/ReadWriteUtil.h>
#include <Alembic/AbcCoreGit/Utils.h>

#include <iostream>
#include <fstream>

#include <Alembic/AbcCoreGit/JSON.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
AwImpl::AwImpl( const std::string &iFileName,
                const AbcA::MetaData &iMetaData,
                const WriteOptions &options )
  : m_fileName( iFileName )
  , m_metaData( iMetaData )
  , m_metaDataMap( new MetaDataMap() )
  , m_options( options )
  , m_repo_ptr( new GitRepo(m_fileName, GitMode::Write, milliwaysEnabled()) )
  , m_ksm( m_repo_ptr->rootGroup(), WRITE )
  , m_written( false )
{
    TRACE("AwImpl::AwImpl('" << iFileName << "')");

    // since the GitRepo could have normalized/altered the actual path used, use that
    m_fileName = m_repo_ptr->pathname();

    // add default time sampling
    AbcA::TimeSamplingPtr ts( new AbcA::TimeSampling() );
    m_timeSamples.push_back(ts);
    m_maxSamples.push_back(0);

    // m_repo_ptr.reset( new GitRepo(m_fileName, GitMode::Write) );

    // init the repo
    init();
}

//-*****************************************************************************
void AwImpl::init()
{
    m_metaData.set("_ai_AlembicVersion", AbcA::GetLibraryVersion());

    ABCA_ASSERT( m_repo_ptr, "invalid git repository object");

    //GitGroupPtr topGroupPtr(new GitGroup(m_repo_ptr, "/"));
    GitGroupPtr topGroupPtr = m_repo_ptr->rootGroup();
    GitGroupPtr abcGroupPtr = topGroupPtr->addGroup("ABC");

    //m_data.reset( new OwData( abcGroupPtr, "ABC", m_metaData ) );
    TODO("pass also `fullName` to AwImpl OwData ObjectHeader or not?!?");
    m_data.reset( new OwData( abcGroupPtr,
        Alembic::Util::shared_ptr<AbcA::ObjectHeader>(new AbcA::ObjectHeader( "ABC", "/", m_metaData )) ) );     // TODO: pass also fullName or not?

    // seed with the common empty keys
    AbcA::ArraySampleKey emptyKey;
    emptyKey.numBytes = 0;
    size_t emptyPos = static_cast<size_t>(-1);

    emptyKey.origPOD = Alembic::Util::kInt8POD;
    emptyKey.readPOD = Alembic::Util::kInt8POD;
    WrittenSampleIDPtr wsid( new WrittenSampleID( emptyKey, emptyPos, 0 ) );
    m_writtenSampleMap.store( wsid );

    emptyKey.origPOD = Alembic::Util::kStringPOD;
    emptyKey.readPOD = Alembic::Util::kStringPOD;
    wsid.reset( new WrittenSampleID( emptyKey, emptyPos, 0 ) );
    m_writtenSampleMap.store( wsid );

    emptyKey.origPOD = Alembic::Util::kWstringPOD;
    emptyKey.readPOD = Alembic::Util::kWstringPOD;
    wsid.reset( new WrittenSampleID( emptyKey, emptyPos, 0 ) );
    m_writtenSampleMap.store( wsid );
}

//-*****************************************************************************
const std::string &AwImpl::getName() const
{
    return m_fileName;
}

//-*****************************************************************************
const AbcA::MetaData &AwImpl::getMetaData() const
{
    return m_metaData;
}

//-*****************************************************************************
AbcA::ArchiveWriterPtr AwImpl::asArchivePtr()
{
    return shared_from_this();
}

//-*****************************************************************************
AbcA::ObjectWriterPtr AwImpl::getTop()
{
    AbcA::ObjectWriterPtr ret = m_top.lock();
    if ( ! ret )
    {
        // time to make a new one
        ret = Alembic::Util::shared_ptr<OwImpl>(
            new OwImpl( asArchivePtr(), m_data, m_metaData ) );
        m_top = ret;
    }

    return ret;
}

//-*****************************************************************************
Util::uint32_t AwImpl::addTimeSampling( const AbcA::TimeSampling & iTs )
{
    index_t numTS = m_timeSamples.size();
    for (index_t i = 0; i < numTS; ++i)
    {
        if (iTs == *(m_timeSamples[i]))
            return i;
    }

    // we've got a new TimeSampling, write it and add it to our vector
    AbcA::TimeSamplingPtr ts( new AbcA::TimeSampling(iTs) );
    m_timeSamples.push_back(ts);
    m_maxSamples.push_back(0);

    index_t latestSample = m_timeSamples.size() - 1;

    std::stringstream strm;
    strm << latestSample;
    std::string name = strm.str();

    return latestSample;
}

//-*****************************************************************************
AbcA::TimeSamplingPtr AwImpl::getTimeSampling( Util::uint32_t iIndex )
{
    ABCA_ASSERT( iIndex < m_timeSamples.size(),
        "Invalid index provided to getTimeSampling." );

    return m_timeSamples[iIndex];
}

//-*****************************************************************************
AbcA::index_t
AwImpl::getMaxNumSamplesForTimeSamplingIndex( Util::uint32_t iIndex )
{
    if ( iIndex < m_maxSamples.size() )
    {
        return m_maxSamples[iIndex];
    }
    return INDEX_UNKNOWN;
}

//-*****************************************************************************
void AwImpl::setMaxNumSamplesForTimeSamplingIndex( Util::uint32_t iIndex,
                                                   AbcA::index_t iMaxIndex )
{
    if ( iIndex < m_maxSamples.size() )
    {
        m_maxSamples[iIndex] = iMaxIndex;
    }
}

//-*****************************************************************************
AwImpl::~AwImpl()
{
    // empty out the map so any dataset IDs will be freed up
    m_writtenSampleMap.clear();

    TODO( "write out child headers");
#if 0
    // write out our child headers
    if ( m_data )
    {
        Util::SpookyHash hash;
        m_data->writeHeaders( m_metaDataMap, hash );
    }
#endif

    // let go of our reference to the data for the top object
    m_data.reset();

    writeToDisk();

    TRACE("CALLING CLEANUP on m_repo_ptr");
    if (m_repo_ptr)
        m_repo_ptr->cleanup();
    TRACE("CALLING RESET on m_repo_ptr");
    m_repo_ptr.reset();

    TRACE(Profile());
}

bool AwImpl::milliwaysEnabled()
{
    if (m_options.has("milliways"))
        return boost::any_cast<bool>(m_options["milliways"]);
    return GitRepo::DEFAULT_MILLIWAYS_ENABLED;
}

std::string AwImpl::relPathname() const
{
    return m_repo_ptr->rootGroup()->relPathname();
}

std::string AwImpl::absPathname() const
{
    return m_repo_ptr->rootGroup()->absPathname();
}

void AwImpl::writeToDisk()
{
    if (! m_written)
    {
        TRACE("AwImpl::writeToDisk() path:'" << absPathname() << "' (WRITING)");

        double t_end, t_start = time_us();

        rapidjson::Document document;
        // define the document as an object rather than an array
        document.SetObject();
        // must pass an allocator when the object may need to allocate memory
        rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

        JsonSet(document, "kind", "Archive");

        JsonSet(document, "metadata", m_metaData.serialize());

        rapidjson::Value jsonTimeSamplings(rapidjson::kArrayType);

        Util::uint32_t numSamplings = getNumTimeSamplings();
        JsonSet(document, "numTimeSamplings", numSamplings);
        for ( Util::uint32_t i = 0; i < numSamplings; ++i )
        {
            Util::uint32_t maxSample = m_maxSamples[i];
            AbcA::TimeSamplingPtr timePtr = getTimeSampling( i );

            rapidjson::Value object(rapidjson::kObjectType);
            jsonWriteTimeSampling( document, object, maxSample, *timePtr );

            jsonTimeSamplings.PushBack( object, allocator );
        }
        JsonSet(document, "timeSamplings", jsonTimeSamplings);

        rapidjson::Value indexedMetaDataObject( rapidjson::kArrayType );
        m_metaDataMap->toJSON(document, indexedMetaDataObject);
        JsonSet(document, "indexedMetaData", indexedMetaDataObject);

        t_end = time_us();
        Profile::add_json_creation(t_end - t_start);

        t_start = time_us();
        std::string output = JsonWrite(document);
        t_end = time_us();
        Profile::add_json_output(t_end - t_start);

        ABCA_ASSERT( m_repo_ptr, "invalid git repository object");

#if JSON_TO_DISK
        t_start = time_us();
        std::string jsonPathname = absPathname() + "/archive.json.abc";
        std::ofstream jsonFile;
        jsonFile.open(jsonPathname.c_str(), std::ios::out | std::ios::trunc);
        jsonFile << output;
        jsonFile.close();
        t_end = time_us();
        Profile::add_disk_write(t_end - t_start);

        t_start = time_us();

        std::string jsonRelPathname = m_repo_ptr->relpath(jsonPathname);

        m_repo_ptr->add(jsonRelPathname);
        m_repo_ptr->write_index();

        boost::optional<std::string> commitMessageOpt = m_options.getCommitMessage();
        if (commitMessageOpt)
            m_repo_ptr->commit_index(*commitMessageOpt);
        else
            m_repo_ptr->commit_index("commit new version to alembic file");

        t_end = time_us();
        Profile::add_git(t_end - t_start);
#else
        t_start = time_us();

        // this must be before treebuilder()->write()
        ksm().writeToDisk();

        m_repo_ptr->rootGroup()->add_file_from_memory("archive.json.abc", output);
        m_repo_ptr->rootGroup()->treebuilder()->write();

        TRACE("committing...");
        boost::optional<std::string> commitMessageOpt = m_options.getCommitMessage();
        if (commitMessageOpt)
            m_repo_ptr->commit_treebuilder(*commitMessageOpt);
        else
            m_repo_ptr->commit_treebuilder("commit new version to alembic file");

        t_end = time_us();
        Profile::add_git(t_end - t_start);
#endif

        // Write archive.json.abc as the entry file for reading.
        //
        std::string jsonPathname = absPathname() + "/archive.json.abc";
        std::ofstream jsonFile;
        jsonFile.open(jsonPathname.c_str(), std::ios::out | std::ios::trunc);
        jsonFile << output;
        jsonFile.close();

        m_written = true;
    } else
    {
        TRACE("AwImpl::writeToDisk() path:'" << absPathname() << "' (skipping, already written)");
    }

    ABCA_ASSERT( m_written, "data not written" );
}

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreGit
} // End namespace Alembic
