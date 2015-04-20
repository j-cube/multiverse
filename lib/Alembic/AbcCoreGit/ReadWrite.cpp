//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

#include <Alembic/AbcCoreGit/ReadWrite.h>
#include <Alembic/AbcCoreGit/Foundation.h>
#include <Alembic/AbcCoreGit/AwImpl.h>
#include <Alembic/AbcCoreGit/ArImpl.h>

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
    generic = other.generic;
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
    generic = rhs.generic;
    if (rhs.m_commit_message)
        m_commit_message = rhs.m_commit_message;
    else
        m_commit_message = boost::none;

    return *this;
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

//-*****************************************************************************
AbcA::ArchiveReaderPtr
ReadArchive::operator()( const std::string &iFileName ) const
{
    AbcA::ArchiveReaderPtr archivePtr;

    archivePtr = Alembic::Util::shared_ptr<ArImpl>(
        new ArImpl( iFileName ) );

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
        new ArImpl( iFileName ) );

    return archivePtr;
}

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreGit
} // End namespace Alembic
