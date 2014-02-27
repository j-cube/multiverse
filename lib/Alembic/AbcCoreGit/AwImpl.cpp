//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

#include <Alembic/AbcCoreGit/AwImpl.h>
#include <Alembic/AbcCoreGit/OwData.h>
#include <Alembic/AbcCoreGit/OwImpl.h>
#include <Alembic/AbcCoreGit/WriteUtil.h>
#include <Alembic/AbcCoreGit/Utils.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

#ifndef GIT_SUCCESS
#define GIT_SUCCESS 0
#endif /* GIT_SUCCESS */

static bool gitlib_initialized = false;

// TODO: find a better way to initialize and cleanup libgit2
static void gitlib_initialize()
{
    if (! gitlib_initialized)
    {
        git_threads_init();

        gitlib_initialized = true;
    }
}

// TODO: call gitlib_cleanup() somewhere
#if 0
static void gitlib_cleanup()
{
    if (gitlib_initialized)
    {
        git_threads_shutdown();

        gitlib_initialized = false;
    }
}
#endif /* 0 */

//-*****************************************************************************
AwImpl::AwImpl( const std::string &iFileName,
                const AbcA::MetaData &iMetaData )
  : m_fileName( iFileName )
  , m_metaData( iMetaData )
  , m_metaDataMap( new MetaDataMap() )
{
    int error;

    TRACE("AwImpl::AwImpl('" << iFileName << "')");

    // add default time sampling
    AbcA::TimeSamplingPtr ts( new AbcA::TimeSampling() );
    m_timeSamples.push_back(ts);
    m_maxSamples.push_back(0);

    git_repository *repo = NULL;
    git_config *cfg = NULL;

    gitlib_initialize();

    // open/create the archive (repo)
    // TODO: we'll need to open an existing repo and perform a commit with this new
    // version instead of "truncating" an existing one
    TODO("beware - saving to an existing repo is not supported");
    error = git_repository_init (&repo, m_fileName.c_str(), /* is_bare */ 0);
    git_check_error(error, "initializing git repository");
    if (( error != GIT_SUCCESS ) || ( !repo ))
    {
        ABCA_THROW( "Could not open file: " << m_fileName << " (git repo)");
    }
    git_repository_config(&cfg, repo /* , NULL, NULL */);

    git_config_set_int32 (cfg, "core.repositoryformatversion", 0);
    git_config_set_bool (cfg, "core.filemode", 1);
    git_config_set_bool (cfg, "core.bare", 0);
    git_config_set_bool (cfg, "core.logallrefupdates", 1);
    git_config_set_bool (cfg, "core.ignorecase", 1);

    // set the version of the Alembic git backend
    // This expresses the AbcCoreGit version - how properties, are stored within Git, etc.
    git_config_set_int32 (cfg, "alembic.formatversion", ALEMBIC_GIT_FILE_VERSION);
    // This is the Alembic library version XXYYZZ
    // Where XX is the major version, YY is the minor version
    // and ZZ is the patch version
    git_config_set_int32 (cfg, "alembic.libversion", ALEMBIC_LIBRARY_VERSION);

    //git_repository_set_workdir(repo, m_fileName);

    git_repository_free(repo);
    repo = NULL;
    //Sleep (1000);

    std::string git_dir = m_fileName + "/.git";
    error = git_repository_open (&repo, git_dir.c_str());
    git_check_error(error, "opening git repository");
    if (( error != GIT_SUCCESS ) || ( !repo ))
    {
        ABCA_THROW( "Could not open file: " << m_fileName << " (git repo)");
    }
    git_repository_config(&cfg, repo /* , NULL, NULL */);
    m_repo_ptr.reset( new GitRepo(m_fileName, repo, cfg) );

    // init the repo
    init();
}

//-*****************************************************************************
void AwImpl::init()
{
    m_metaData.set("_ai_AlembicVersion", AbcA::GetLibraryVersion());

    ABCA_ASSERT( m_repo_ptr, "invalid git repository object");
    //GitGroupPtr topGroupPtr(new GitGroup(m_repo_ptr, "/"));
    GitGroupPtr topGroupPtr = m_repo_ptr->addGroup("/");
    m_data.reset( new OwData( topGroupPtr, "ABC", m_metaData ) );

    // seed with the common empty keys
    AbcA::ArraySampleKey emptyKey;
    emptyKey.numBytes = 0;
    GitDataPtr emptyData( new GitData() );

    emptyKey.origPOD = Alembic::Util::kInt8POD;
    emptyKey.readPOD = Alembic::Util::kInt8POD;
    WrittenSampleIDPtr wsid( new WrittenSampleID( emptyKey, emptyData, 0 ) );
    m_writtenSampleMap.store( wsid );

    emptyKey.origPOD = Alembic::Util::kStringPOD;
    emptyKey.readPOD = Alembic::Util::kStringPOD;
    wsid.reset( new WrittenSampleID( emptyKey, emptyData, 0 ) );
    m_writtenSampleMap.store( wsid );

    emptyKey.origPOD = Alembic::Util::kWstringPOD;
    emptyKey.readPOD = Alembic::Util::kWstringPOD;
    wsid.reset( new WrittenSampleID( emptyKey, emptyData, 0 ) );
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

    TODO( "encode and write time samplings?");
#if 0
    // encode and write the time samplings and max samples into data
    if ( m_archive.isValid() )
    {
        // encode and write the Metadata for the archive, since the top level
        // meta data can be kinda big and is very specialized don't worry
        // about putting it into the meta data map
        std::string metaData = m_metaData.serialize();
        m_archive.getGroup()->addData( metaData.size(), metaData.c_str() );

        std::vector< Util::uint8_t > data;
        Util::uint32_t numSamplings = getNumTimeSamplings();
        for ( Util::uint32_t i = 0; i < numSamplings; ++i )
        {
            Util::uint32_t maxSample = m_maxSamples[i];
            AbcA::TimeSamplingPtr timePtr = getTimeSampling( i );
            WriteTimeSampling( data, maxSample, *timePtr );
        }

        m_archive.getGroup()->addData( data.size(), &( data.front() ) );
        m_metaDataMap->write( m_archive.getGroup() );
    }
#endif
}

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreGit
} // End namespace Alembic
