//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

#ifndef _Alembic_AbcCoreGit_AwImpl_h_
#define _Alembic_AbcCoreGit_AwImpl_h_

#include <Alembic/AbcCoreGit/Foundation.h>
#include <Alembic/AbcCoreGit/WrittenSampleMap.h>
#include <Alembic/AbcCoreGit/WriteUtil.h>
#include <Alembic/AbcCoreGit/Git.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
class OwData;

//-*****************************************************************************
class AwImpl : public AbcA::ArchiveWriter
             , public Alembic::Util::enable_shared_from_this<AwImpl>
{
private:
    friend class WriteArchive;

    AwImpl( const std::string &iFileName,
            const AbcA::MetaData &iMetaData );

public:
    virtual ~AwImpl();

    //-*************************************************************************
    // ABSTRACT FUNCTIONS
    //-*************************************************************************
    virtual const std::string &getName() const;

    virtual const AbcA::MetaData &getMetaData() const;

    virtual AbcA::ObjectWriterPtr getTop();

    virtual AbcA::ArchiveWriterPtr asArchivePtr();

    //-*************************************************************************
    // GLOBAL FILE CONTEXT STUFF.
    //-*************************************************************************
    WrittenSampleMap &getWrittenSampleMap()
    {
        return m_writtenSampleMap;
    }
    MetaDataMapPtr getMetaDataMap()
    {
        return m_metaDataMap;
    }

    virtual Util::uint32_t addTimeSampling( const AbcA::TimeSampling & iTs );

    virtual AbcA::TimeSamplingPtr getTimeSampling( Util::uint32_t iIndex );

    virtual Util::uint32_t getNumTimeSamplings()
    { return m_timeSamples.size(); }

    virtual AbcA::index_t getMaxNumSamplesForTimeSamplingIndex(
        Util::uint32_t iIndex );

    virtual void setMaxNumSamplesForTimeSamplingIndex( Util::uint32_t iIndex,
                                                      AbcA::index_t iMaxIndex );

    std::string relPathname() const;
    std::string absPathname() const;

    void writeToDisk();

private:
    void init();
    std::string m_fileName;
    AbcA::MetaData m_metaData;
    // libgit2 repo/wt info
    GitRepoPtr m_repo_ptr;

    Alembic::Util::weak_ptr< AbcA::ObjectWriter > m_top;    // concretely an OwImpl
    Alembic::Util::shared_ptr < OwData > m_data;

    std::vector < AbcA::TimeSamplingPtr > m_timeSamples;

    std::vector < AbcA::index_t > m_maxSamples;

    WrittenSampleMap m_writtenSampleMap;
    MetaDataMapPtr m_metaDataMap;

    bool m_written;
};

} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif
