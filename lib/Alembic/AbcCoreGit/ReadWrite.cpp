//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

#include <Alembic/AbcCoreGit/ReadWrite.h>
// #include <Alembic/AbcCoreGit/Foundation.h>
// #include <Alembic/AbcCoreGit/AwImpl.h>
// #include <Alembic/AbcCoreGit/ArImpl.h>
// #include <Alembic/AbcCoreGit/CacheImpl.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

// //-*****************************************************************************
// WriteArchive::WriteArchive()
// {
//     m_cacheHierarchy = false;
// }

// //-*****************************************************************************
// WriteArchive::WriteArchive( bool iCacheHierarchy )
// {
//     m_cacheHierarchy = iCacheHierarchy;
// }

// //-*****************************************************************************
// AbcA::ArchiveWriterPtr
// WriteArchive::operator()( const std::string &iFileName,
//                           const AbcA::MetaData &iMetaData ) const
// {
//     Alembic::Util::shared_ptr<AwImpl> archivePtr(
//         new AwImpl( iFileName, iMetaData, m_cacheHierarchy ) );

//     return archivePtr;
// }

// //-*****************************************************************************
// AbcA::ReadArraySampleCachePtr
// CreateCache()
// {
//     AbcA::ReadArraySampleCachePtr cachePtr( new CacheImpl() );
//     return cachePtr;
// }


// //-*****************************************************************************
// ReadArchive::ReadArchive()
// {
//     m_cacheHierarchy = false;
// }

// //-*****************************************************************************
// ReadArchive::ReadArchive( bool iCacheHierarchy )
// {
//     m_cacheHierarchy = iCacheHierarchy;
// }

// //-*****************************************************************************
// // This version creates a cache.
// AbcA::ArchiveReaderPtr
// ReadArchive::operator()( const std::string &iFileName ) const
// {
//     AbcA::ReadArraySampleCachePtr cachePtr = CreateCache();
//     Alembic::Util::shared_ptr<ArImpl> archivePtr(
//         new ArImpl( iFileName, cachePtr, m_cacheHierarchy ) );

//     return archivePtr;
// }

// //-*****************************************************************************
// // This version takes a cache from outside.
// AbcA::ArchiveReaderPtr
// ReadArchive::operator()( const std::string &iFileName,
//                          AbcA::ReadArraySampleCachePtr iCachePtr ) const
// {
//     Alembic::Util::shared_ptr<ArImpl> archivePtr(
//         new ArImpl( iFileName, iCachePtr, m_cacheHierarchy ) );
//     return archivePtr;
// }

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreGit
} // End namespace Alembic
