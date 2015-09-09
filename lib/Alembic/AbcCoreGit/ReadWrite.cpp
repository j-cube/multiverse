/*****************************************************************************/
/*  multiverse - a next generation storage back-end for Alembic              */
/*  Copyright (C) 2015 J CUBE Inc. Tokyo, Japan. All Rights Reserved.        */
/*                                                                           */
/*  This program is free software: you can redistribute it and/or modify     */
/*  it under the terms of the GNU General Public License as published by     */
/*  the Free Software Foundation, either version 3 of the License, or        */
/*  (at your option) any later version.                                      */
/*                                                                           */
/*  This program is distributed in the hope that it will be useful,          */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of           */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            */
/*  GNU General Public License for more details.                             */
/*                                                                           */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    */
/*                                                                           */
/*    J CUBE Inc.                                                            */
/*    6F Azabu Green Terrace                                                 */
/*    3-20-1 Minami-Azabu, Minato-ku, Tokyo                                  */
/*    info@-jcube.jp                                                         */
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
