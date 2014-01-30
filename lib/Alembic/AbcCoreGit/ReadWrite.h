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

// //-*****************************************************************************
// //! Will return a shared pointer to the archive writer
// //! There is only one way to create an archive writer in AbcCoreGit.
// class WriteArchive
// {
// public:
//     WriteArchive();
//     explicit WriteArchive( bool iCacheHierarchy );

//     ::Alembic::AbcCoreAbstract::ArchiveWriterPtr
//     operator()( const std::string &iFileName,
//                 const ::Alembic::AbcCoreAbstract::MetaData &iMetaData ) const;
// private:
//     bool m_cacheHierarchy;
// };

// //-*****************************************************************************
// //! AbcCoreGit Provides a Cache implementation, that we expose here.
// //! It takes no arguments.
// //! This would only be used if you wished to create a global cache separately
// //! from an archive - this is actually fairly common, though, which is why
// //! it is exposed here.
// ::Alembic::AbcCoreAbstract::ReadArraySampleCachePtr
// CreateCache( void );

// //-*****************************************************************************
// //! Will return a shared pointer to the archive reader
// //! This version creates a cache associated with the archive.
// class ReadArchive
// {
// public:
//     ReadArchive();
//     explicit ReadArchive( bool iCacheHierarchy );

//     // Make our own cache.
//     ::Alembic::AbcCoreAbstract::ArchiveReaderPtr
//     operator()( const std::string &iFileName ) const;

//     // Take the given cache.
//     ::Alembic::AbcCoreAbstract::ArchiveReaderPtr
//     operator()( const std::string &iFileName,
//                 ::Alembic::AbcCoreAbstract::ReadArraySampleCachePtr iCache
//               ) const;
// private:
//     bool m_cacheHierarchy;
// };

} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif

