//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

#include <Alembic/AbcCoreGit/ArImpl.h>
#include <Alembic/AbcCoreGit/OrData.h>
#include <Alembic/AbcCoreGit/OrImpl.h>
//#include <Alembic/AbcCoreGit/ReadUtil.h>
#include <Alembic/AbcCoreGit/Utils.h>

#include <iostream>
#include <fstream>
#include <json/json.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
ArImpl::ArImpl( const std::string &iFileName )
  : m_fileName( iFileName )
  , m_header( new AbcA::ObjectHeader() )
  , m_read( false )
{
    TRACE("ArImpl::ArImpl('" << iFileName << "')");

    m_repo_ptr.reset( new GitRepo(m_fileName, GitMode::Read) );

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

    ABCA_ASSERT( formatversion >= 0 && formatversion <= ALEMBIC_GIT_FILE_VERSION,
        "Unsupported Alembic Git repository version detected: " << formatversion );

    m_archiveVersion = formatversion;

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

static void ReadIndexedMetaData( Json::Value json,
                     std::vector< AbcA::MetaData > & oMetaDataVec )
{
    // add the default empty meta data
    oMetaDataVec.push_back( AbcA::MetaData() );

    std::vector<std::string> children;
    for (Json::Value::iterator it = json.begin(); it != json.end(); ++it)
    {
        std::string metaData = (*it).asString();
        AbcA::MetaData md;
        md.deserialize( metaData );
        oMetaDataVec.push_back( md );
    }
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

    Json::Value root;
    Json::Reader reader;

    std::string jsonPathname = absPathname() + "/archive.json";
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
    ABCA_ASSERT( (v_kind == "Archive"), "invalid archive kind" );

    // read archive metadata
    std::string v_metadata = root.get("metadata", "").asString();
    m_header->getMetaData().deserialize( v_metadata );

    TRACE("[ArImpl " << *this << "] kind:" << v_kind);
    TRACE("[ArImpl " << *this << "] metadata:" << v_metadata);

    // TODO: read other json fields from archive
    TODO("read time samplings, etc...");

    m_read = true;

    // read top object ("/ABC")

    GitGroupPtr topGroupPtr = m_repo_ptr->rootGroup();
    GitGroupPtr abcGroupPtr = topGroupPtr->addGroup("ABC");

    TODO( "read time samples and max");
#if 0
    ReadTimeSamplesAndMax( group->getData( 4, 0 ),
                           m_timeSamples, m_maxSamples );
#endif /* 0 */

    ReadIndexedMetaData( root["indexedMetaData"], m_indexMetaData );

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

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreGit
} // End namespace Alembic
