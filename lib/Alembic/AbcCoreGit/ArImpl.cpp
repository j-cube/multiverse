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

#include <Alembic/AbcCoreGit/ArImpl.h>
#include <Alembic/AbcCoreGit/OrData.h>
#include <Alembic/AbcCoreGit/OrImpl.h>
#include <Alembic/AbcCoreGit/ReadWriteUtil.h>
#include <Alembic/AbcCoreGit/Utils.h>

#include <iostream>
#include <fstream>

#include <Alembic/AbcCoreGit/JSON.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
ArImpl::ArImpl( const std::string &iFileName, const Alembic::AbcCoreFactory::IOptions& iOptions )
  : m_fileName( iFileName )
  , m_header( new AbcA::ObjectHeader() )
  , m_options( iOptions )
  , m_repo_ptr( new GitRepo(m_fileName, m_options, GitMode::Read) )
  , m_ksm( m_repo_ptr->rootGroup(), READ )
  , m_read( false )
{
    TRACE("ArImpl::ArImpl('" << iFileName << "')");

    // m_repo_ptr.reset( new GitRepo(m_fileName, m_options, GitMode::Read) );

    // since the GitRepo could have normalized/altered the actual path used, use that
    m_fileName = m_repo_ptr->pathname();

    TRACE("repo valid:" << (m_repo_ptr->isValid() ? "TRUE" : "FALSE"));

    ABCA_ASSERT( m_repo_ptr->isValid(),
                 "Could not open as Git repository: " << m_fileName );

    ABCA_ASSERT( m_repo_ptr->isFrozen(),
        "Git repository not cleanly closed while being written: " << m_fileName );

    init();
}

//-*****************************************************************************
void ArImpl::init()
{
    int32_t formatversion = m_repo_ptr->formatVersion();
    int32_t libversion = m_repo_ptr->libVersion();

    ABCA_ASSERT( formatversion >= 0 && formatversion <= ALEMBIC_GIT_FILE_VERSION,
        "Unsupported Alembic Git repository version detected: " << formatversion );

    m_archiveVersion = libversion;

    GitGroupPtr group = m_repo_ptr->rootGroup();

#if 0
    ReadTimeSamplesAndMax( group->getData( 4, 0 ),
                           m_timeSamples, m_maxSamples );

    ReadIndexedMetaData( group->getData( 5, 0 ), m_indexMetaData );

    m_data.reset( new OrData( group->getGroup( 2, false, 0 ), "", 0, *this,
                              m_indexMetaData ) );
#endif /* 0 */

    m_header->setName( "ABC" );
    m_header->setFullName( "/" );

    readFromDisk();
}

//-*****************************************************************************
const std::string &ArImpl::getName() const
{
    return m_fileName;
}

//-*****************************************************************************
const AbcA::MetaData &ArImpl::getMetaData() const
{
    return m_header->getMetaData();
}

//-*****************************************************************************
AbcA::ObjectReaderPtr ArImpl::getTop()
{
    Alembic::Util::scoped_lock l( m_orlock );

    AbcA::ObjectReaderPtr ret = m_top.lock();
    if ( ! ret )
    {
        // time to make a new one
        ret = Alembic::Util::shared_ptr<OrImpl>(
            new OrImpl( shared_from_this(), m_data, m_header ) );
        m_top = ret;
    }

    return ret;
}

//-*****************************************************************************
AbcA::TimeSamplingPtr ArImpl::getTimeSampling( Util::uint32_t iIndex )
{
    ABCA_ASSERT( iIndex < m_timeSamples.size(),
        "Invalid index provided to getTimeSampling." );

    return m_timeSamples[iIndex];
}

//-*****************************************************************************
AbcA::ArchiveReaderPtr ArImpl::asArchivePtr()
{
    return shared_from_this();
}

//-*****************************************************************************
AbcA::index_t
ArImpl::getMaxNumSamplesForTimeSamplingIndex( Util::uint32_t iIndex )
{
    if ( iIndex < m_maxSamples.size() )
    {
        return m_maxSamples[iIndex];
    }

    return INDEX_UNKNOWN;
}

//-*****************************************************************************
ArImpl::~ArImpl()
{
    TRACE("AR CALLING CLEANUP on m_repo_ptr");
    if (m_repo_ptr)
        m_repo_ptr->cleanup();
    TRACE("AR CALLING RESET on m_repo_ptr");
    m_repo_ptr.reset();
}

//-*****************************************************************************
const std::vector< AbcA::MetaData > & ArImpl::getIndexedMetaData()
{
    return m_indexMetaData;
}

std::string ArImpl::relPathname() const
{
    return m_repo_ptr->rootGroup()->relPathname();
}

std::string ArImpl::absPathname() const
{
    return m_repo_ptr->rootGroup()->absPathname();
}

static void ReadIndexedMetaData( const rapidjson::Value& json,
                     std::vector< AbcA::MetaData > & oMetaDataVec )
{
    // add the default empty meta data
    oMetaDataVec.push_back( AbcA::MetaData() );

    std::vector<std::string> children;

    for (rapidjson::Value::ConstValueIterator it = json.Begin(); it != json.End(); ++it)
    {
        const rapidjson::Value& el = (*it);

        std::string metaData(el.GetString(), el.GetStringLength());
        AbcA::MetaData md;
        md.deserialize( metaData );
        oMetaDataVec.push_back( md );
    }
}

bool ArImpl::hasRevisionSpec() const
{
    return m_options.has("revision");
}

std::string ArImpl::revisionString()
{
    if (m_options.has("revision"))
        return boost::any_cast<std::string>(m_options["revision"]);
    return std::string();
}

bool ArImpl::ignoreWrongRevision()
{
    if (m_options.has("ignoreNonexistentRevision"))
        return boost::any_cast<bool>(m_options["ignoreNonexistentRevision"]);
    return GitRepo::DEFAULT_IGNORE_WRONG_REV;
}

bool ArImpl::milliwaysEnabled()
{
    if (m_options.has("milliways"))
        return boost::any_cast<bool>(m_options["milliways"]);
    return GitRepo::DEFAULT_MILLIWAYS_ENABLED;
}

bool ArImpl::readFromDisk()
{
    if (m_read)
    {
        TRACE("ArImpl::readFromDisk() path:'" << absPathname() << "' (skipping, already read)");
        return true;
    }

    ABCA_ASSERT( !m_read, "already read" );

    TRACE("[ArImpl " << *this << "] ArImpl::readFromDisk() path:'" << absPathname() << "' (READING)");

    GitGroupPtr topGroupPtr = m_repo_ptr->rootGroup();
    GitGroupPtr abcGroupPtr = topGroupPtr->addGroup("ABC");

    std::string jsonPathname = absPathname() + "/archive.json.abc";

#if JSON_TO_DISK
    std::ifstream jsonFile(jsonPathname.c_str());
    std::stringstream jsonBuffer;
    jsonBuffer << jsonFile.rdbuf();
    jsonFile.close();

    std::string jsonContents = jsonBuffer.str();
#else
    boost::optional<std::string> optJsonContents = topGroupPtr->tree()->getChildFile("archive.json.abc");
    if (! optJsonContents)
    {
        ABCA_THROW( "can't read git blob '" << jsonPathname << "'" );
        return false;
    }
    std::string jsonContents = *optJsonContents;
#endif

    JSONParser json(jsonPathname, jsonContents);
    rapidjson::Document& document = json.document;

    std::string v_kind = JsonGetString(document, "kind").get_value_or("UNKNOWN");
    ABCA_ASSERT( (v_kind == "Archive"), "invalid archive kind" );

    // read archive metadata
    std::string v_metadata = JsonGetString(document, "metadata").get_value_or("");
    m_header->getMetaData().deserialize( v_metadata );

    TRACE("[ArImpl " << *this << "] kind:" << v_kind);
    TRACE("[ArImpl " << *this << "] metadata:" << v_metadata);

    //Util::uint32_t v_numTimeSamplings = root.get("numTimeSamplings", 0).asUInt();
    const rapidjson::Value& v_timeSamplings = document["timeSamplings"];
    jsonReadTimeSamples( v_timeSamplings, m_timeSamples, m_maxSamples );

    // TODO: read other json fields from archive
    TODO("read other archive data etc...");

    m_read = true;

    // read top object ("/ABC")

    TODO( "read time samples and max");
#if 0
    ReadTimeSamplesAndMax( group->getData( 4, 0 ),
                           m_timeSamples, m_maxSamples );
#endif /* 0 */

    ReadIndexedMetaData( document["indexedMetaData"], m_indexMetaData );

    ABCA_ASSERT( !m_data.get(), "OrData exists already" );
    assert( !m_data.get() );
    m_data.reset( new OrData( /* GitGroupPtr */ abcGroupPtr,
            /* parent name */ "",
            /* thread id */ 0,
            /* archive reader */ *this,
            /* indexed metadata */ m_indexMetaData ) );

    ABCA_ASSERT( m_read, "data not read" );
    TRACE("[ArImpl " << *this << "] completed read from disk");
    return true;
}

std::string ArImpl::repr(bool extended) const
{
    std::ostringstream ss;
    if (extended)
    {
        ss << "<ArImpl(name:'" << getName() << "')>";
    } else
    {
        ss << "'" << getName() << "'";
    }
    return ss.str();
}

/* History API */

std::vector<GitCommitInfo> getHistory(const std::string& archivePathname, bool& error)
{
    Alembic::AbcCoreFactory::IOptions rOptions;

    GitRepoPtr repo_ptr( new GitRepo(archivePathname, rOptions, GitMode::Read) );
    return repo_ptr->getHistory(error);
}

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreGit
} // End namespace Alembic
