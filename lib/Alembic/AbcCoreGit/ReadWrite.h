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

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
//! Will return a shared pointer to the archive writer
//! There is only one way to create an archive writer in AbcCoreGit.
class WriteArchive
{
public:
    WriteArchive();

    ::Alembic::AbcCoreAbstract::ArchiveWriterPtr
    operator()( const std::string &iFileName,
                const ::Alembic::AbcCoreAbstract::MetaData &iMetaData ) const;
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
