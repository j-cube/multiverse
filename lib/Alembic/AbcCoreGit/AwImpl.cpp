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

#include <iostream>
#include <fstream>
#include <json/json.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
AwImpl::AwImpl( const std::string &iFileName,
                const AbcA::MetaData &iMetaData )
  : m_fileName( iFileName )
  , m_metaData( iMetaData )
  , m_metaDataMap( new MetaDataMap() )
  , m_written( false )
{
    TRACE("AwImpl::AwImpl('" << iFileName << "')");

    // add default time sampling
    AbcA::TimeSamplingPtr ts( new AbcA::TimeSampling() );
    m_timeSamples.push_back(ts);
    m_maxSamples.push_back(0);

    m_repo_ptr.reset( new GitRepo(m_fileName, GitMode::Write) );

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

    writeToDisk();
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

        Json::Value root( Json::objectValue );

        root["kind"] = "Archive";

        root["metadata"] = m_metaData.serialize();

        Json::Value jsonTimeSamplings( Json::arrayValue );

        Util::uint32_t numSamplings = getNumTimeSamplings();
        root["numTimeSamplings"] = numSamplings;
        for ( Util::uint32_t i = 0; i < numSamplings; ++i )
        {
            Util::uint32_t maxSample = m_maxSamples[i];
            AbcA::TimeSamplingPtr timePtr = getTimeSampling( i );
            jsonTimeSamplings.append( jsonWriteTimeSampling( maxSample, *timePtr ) );
        }
        root["timeSamplings"] = jsonTimeSamplings;

        Json::StyledWriter writer;
        std::string output = writer.write( root );

        std::string jsonPathname = absPathname() + "/archive.json";
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
