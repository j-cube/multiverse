//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

#ifndef _Alembic_AbcCoreGit_ReadWrite_h_
#define _Alembic_AbcCoreGit_ReadWrite_h_

#include <Alembic/AbcCoreAbstract/All.h>

#include <map>
#include <boost/any.hpp>
#include <boost/optional.hpp>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

class WriteOptions
{
public:
    WriteOptions();
    WriteOptions(const std::string& commit_message);
    WriteOptions(const WriteOptions& other);
    ~WriteOptions();

    WriteOptions& operator= (const WriteOptions& rhs);

    void setCommitMessage(const std::string& message) { m_commit_message = message; }
    boost::optional<std::string> getCommitMessage() const { return m_commit_message; }

    std::map< std::string, boost::any > generic;

private:
    boost::optional<std::string> m_commit_message;
};

//-*****************************************************************************
//! Will return a shared pointer to the archive writer
//! There is only one way to create an archive writer in AbcCoreGit.
class WriteArchive
{
public:
    WriteArchive();
    WriteArchive(const WriteOptions& options);

    ::Alembic::AbcCoreAbstract::ArchiveWriterPtr
    operator()( const std::string &iFileName,
                const ::Alembic::AbcCoreAbstract::MetaData &iMetaData ) const;

    const WriteOptions& getOptions() const { return m_options; }
    WriteOptions& getOptions() { return m_options; }

private:
    WriteOptions m_options;
};

//-*****************************************************************************
//! Will return a shared pointer to the archive reader
//! This version creates a cache associated with the archive.
class ReadArchive
{
public:
    ReadArchive();

    // open the file
    ::Alembic::AbcCoreAbstract::ArchiveReaderPtr
    operator()( const std::string &iFileName ) const;

    // The given cache is ignored.
    ::Alembic::AbcCoreAbstract::ArchiveReaderPtr
    operator()( const std::string &iFileName,
                ::Alembic::AbcCoreAbstract::ReadArraySampleCachePtr iCache
              ) const;
};

} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif
