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

#include <Alembic/AbcCoreGit/ReadWrite.h>
#include <Alembic/AbcCoreGit/Foundation.h>
#include <Alembic/AbcCoreGit/AwImpl.h>
#include <Alembic/AbcCoreGit/ArImpl.h>

#include <Alembic/AbcCoreGit/Git.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

WriteOptions::WriteOptions()
{
}


WriteOptions::WriteOptions(const std::string& commit_message) :
    m_commit_message(commit_message)
{
}

WriteOptions::WriteOptions(const WriteOptions& other)
{
    m_generic = other.m_generic;
    if (other.m_commit_message)
        m_commit_message = other.m_commit_message;
    else
        m_commit_message = boost::none;
}

WriteOptions::~WriteOptions()
{
}

WriteOptions& WriteOptions::operator= (const WriteOptions& rhs)
{
    m_generic = rhs.m_generic;
    if (rhs.m_commit_message)
        m_commit_message = rhs.m_commit_message;
    else
        m_commit_message = boost::none;

    return *this;
}

void WriteOptions::set(const std::string& key, const boost::any& value)
{
    m_generic[key] = value;
}

bool WriteOptions::has(const std::string& key) const
{
    return (m_generic.count(key) > 0);
}

boost::any WriteOptions::get(const std::string& key)
{
    return m_generic[key];
}

//-*****************************************************************************
WriteArchive::WriteArchive()
{
}

WriteArchive::WriteArchive(const WriteOptions& options)
{
    m_options = options;
}

//-*****************************************************************************
AbcA::ArchiveWriterPtr
WriteArchive::operator()( const std::string &iFileName,
                          const AbcA::MetaData &iMetaData ) const
{
    Alembic::Util::shared_ptr<AwImpl> archivePtr(
        new AwImpl( iFileName, iMetaData, m_options ) );

    return archivePtr;
}

//-*****************************************************************************
ReadArchive::ReadArchive()
{
}

ReadArchive::ReadArchive(const Alembic::AbcCoreFactory::IOptions& iOptions) :
    m_options(iOptions)
{
}

//-*****************************************************************************
AbcA::ArchiveReaderPtr
ReadArchive::operator()( const std::string &iFileName ) const
{
    AbcA::ArchiveReaderPtr archivePtr;

    archivePtr = Alembic::Util::shared_ptr<ArImpl>(
        new ArImpl( iFileName, m_options ) );

    return archivePtr;
}

AbcA::ArchiveReaderPtr
ReadArchive::operator()( const std::string &iFileName,
    const Alembic::AbcCoreFactory::IOptions& iOptions ) const
{
    AbcA::ArchiveReaderPtr archivePtr;

    archivePtr = Alembic::Util::shared_ptr<ArImpl>(
        new ArImpl( iFileName, iOptions ) );

    return archivePtr;
}

//-*****************************************************************************
// The given cache is ignored.
AbcA::ArchiveReaderPtr
ReadArchive::operator()( const std::string &iFileName,
            AbcA::ReadArraySampleCachePtr iCache ) const
{
    AbcA::ArchiveReaderPtr archivePtr;

    archivePtr = Alembic::Util::shared_ptr<ArImpl> (
        new ArImpl( iFileName, m_options ) );

    return archivePtr;
}

AbcA::ArchiveReaderPtr
ReadArchive::operator()( const std::string &iFileName,
            AbcA::ReadArraySampleCachePtr iCache,
            const Alembic::AbcCoreFactory::IOptions& iOptions ) const
{
    AbcA::ArchiveReaderPtr archivePtr;

    archivePtr = Alembic::Util::shared_ptr<ArImpl> (
        new ArImpl( iFileName, iOptions ) );

    return archivePtr;
}

/* History API */

bool trashHistory(const std::string& archivePathname, std::string& errorMessage, const std::string& branchName)
{
    Alembic::AbcCoreFactory::IOptions rOptions;

    GitRepoPtr repo_ptr( new GitRepo(archivePathname, rOptions, GitMode::Read) );
    return repo_ptr->trashHistory(errorMessage, branchName);
}

std::string getHistoryJSON(Alembic::Abc::IArchive& archive, bool& error)
{
    ArImplPtr arImplPtr = getArImplPtr(archive);
    return arImplPtr->getHistoryJSON(error);
}

std::string getHistoryJSON(const std::string& archivePathname, bool& error)
{
    Alembic::AbcCoreFactory::IOptions rOptions;

    GitRepoPtr repo_ptr( new GitRepo(archivePathname, rOptions, GitMode::Read) );
    return repo_ptr->getHistoryJSON(error);
}

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreGit
} // End namespace Alembic
