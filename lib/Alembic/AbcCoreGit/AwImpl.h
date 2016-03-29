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

#ifndef _Alembic_AbcCoreGit_AwImpl_h_
#define _Alembic_AbcCoreGit_AwImpl_h_

#include <Alembic/AbcCoreGit/Foundation.h>
#include <Alembic/AbcCoreGit/WrittenSampleMap.h>
#include <Alembic/AbcCoreGit/ReadWriteUtil.h>
#include <Alembic/AbcCoreGit/ReadWrite.h>
#include <Alembic/AbcCoreGit/Git.h>
#include <Alembic/AbcCoreGit/KeyStore.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
class OwData;

class AwImpl;
typedef Util::shared_ptr<AwImpl> AwImplPtr;

//-*****************************************************************************
class AwImpl : public AbcA::ArchiveWriter
             , public Alembic::Util::enable_shared_from_this<AwImpl>
{
private:
    friend class WriteArchive;

    AwImpl( const std::string &iFileName,
            const AbcA::MetaData &iMetaData,
            const WriteOptions &options );

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

    GitRepoPtr repo()           { return m_repo_ptr; }
    GitGroupPtr group()         { return repo()->rootGroup(); }

    bool milliwaysEnabled();

    std::string relPathname() const;
    std::string absPathname() const;

    void writeToDisk();

    KeyStoreMap& ksm() { return m_ksm; }

private:
    void init();
    std::string m_fileName;
    AbcA::MetaData m_metaData;

    Alembic::Util::weak_ptr< AbcA::ObjectWriter > m_top;    // concretely an OwImpl
    Alembic::Util::shared_ptr < OwData > m_data;

    std::vector < AbcA::TimeSamplingPtr > m_timeSamples;

    std::vector < AbcA::index_t > m_maxSamples;

    WrittenSampleMap m_writtenSampleMap;
    MetaDataMapPtr m_metaDataMap;

    WriteOptions m_options;

    // libgit2 repo/wt info
    GitRepoPtr m_repo_ptr;

    KeyStoreMap m_ksm;

    bool m_written;
};

} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif
