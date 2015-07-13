##-*****************************************************************************
##
## Copyright (c) 2014,
##
## All rights reserved.
##
##-*****************************************************************************

# Choose custom backend
SET( USE_CUSTOM_BACKEND "NO" CACHE STRING "Choose custom backend" )
SET_PROPERTY( CACHE USE_CUSTOM_BACKEND PROPERTY STRINGS NO SQLITE3 MEMCACHED )

# C++ files for this project
SET( CXX_FILES
  AprImpl.cpp
  ApwImpl.cpp
  ArImpl.cpp
  AwImpl.cpp
  # CacheImpl.cpp
  CprData.cpp
  CprImpl.cpp
  CpwData.cpp
  CpwImpl.cpp
  # DataTypeRegistry.cpp
  # # GitHierarchy.cpp
  # # GitHierarchyReader.cpp
  # # GitHierarchyWriter.cpp
  Git.cpp
  # GitUtil.cpp
  MetaDataMap.cpp
  OrData.cpp
  OrImpl.cpp
  OwData.cpp
  OwImpl.cpp
  # ReadUtil.cpp
  ReadWrite.cpp
  KeyStore.cpp
  SampleStore.cpp
  SprImpl.cpp
  SpwImpl.cpp
  # StringReadUtil.cpp
  # StringWriteUtil.cpp
  Utils.cpp
  ReadWriteUtil.cpp
  msgpack_support.cpp
  JSON.cpp
)

SET( C_FILES
  git-memcached.c
  git-sqlite.c
)

SET( H_FILES
  All.h
  AprImpl.h
  ApwImpl.h
  ArImpl.h
  AwImpl.h
  # CacheImpl.h
  CprData.h
  CprImpl.h
  CpwData.h
  CpwImpl.h
  # DataTypeRegistry.h
  # # GitHierarchy.h
  # # GitHierarchyReader.h
  # # GitHierarchyWriter.h
  Git.h
  # GitUtil.h
  # Foundation.h
  MetaDataMap.h
  OrData.h
  OrImpl.h
  OwData.h
  OwImpl.h
  # ReadUtil.h
  ReadWrite.h
  KeyStore.h
  SampleStore.h
  # SimplePrImpl.h
  # SimplePwImpl.h
  # SprImpl.h
  SpwImpl.h
  # StringReadUtil.h
  # StringWriteUtil.h
  Utils.h
  ReadWriteUtil.h
  WrittenSampleMap.h
  # WrittenArraySampleMap.h
  msgpack_support.h
  JSON.h
)

IF( USE_CUSTOM_BACKEND STREQUAL "SQLITE3" )
  ADD_DEFINITIONS( -DUSE_SQLITE_BACKEND )

  LIST( APPEND CXX_FILES git-sqlite.c )
  LIST( APPEND H_FILES   git-sqlite.h )
ELSEIF( USE_CUSTOM_BACKEND STREQUAL "MEMCACHED" )
  ADD_DEFINITIONS( -DUSE_MEMCACHED_BACKEND )

  LIST( APPEND CXX_FILES git-memcached.c )
  LIST( APPEND H_FILES   git-memcached.h )
ENDIF()

SET( SOURCE_FILES ${CXX_FILES} ${H_FILES} )

ADD_LIBRARY( AlembicAbcCoreGit ${SOURCE_FILES} )

IF( USE_CUSTOM_BACKEND STREQUAL "SQLITE3" )
  TARGET_LINK_LIBRARIES( AlembicAbcCoreGit ${ALEMBIC_LIBGIT2_LIBS} ${ALEMBIC_MSGPACK_LIBS} ${ALEMBIC_JSONCPP_LIBS} ${ALEMBIC_SQLITE3_LIBS} )
ELSEIF( USE_CUSTOM_BACKEND STREQUAL "MEMCACHED" )
  TARGET_LINK_LIBRARIES( AlembicAbcCoreGit ${ALEMBIC_LIBGIT2_LIBS} ${ALEMBIC_MSGPACK_LIBS} ${ALEMBIC_JSONCPP_LIBS} ${ALEMBIC_LIBMEMCACHED_LIBS} )
ELSE()
  TARGET_LINK_LIBRARIES( AlembicAbcCoreGit ${ALEMBIC_LIBGIT2_LIBS} ${ALEMBIC_MSGPACK_LIBS} ${ALEMBIC_JSONCPP_LIBS} )
ENDIF()

INSTALL( TARGETS AlembicAbcCoreGit
         LIBRARY DESTINATION lib
         ARCHIVE DESTINATION lib/static )

# Only install AbcCoreGit.h and ReadArraySampleCache
INSTALL( FILES
         All.h
         ReadWrite.h
         DESTINATION include/Alembic/AbcCoreGit
         PERMISSIONS OWNER_READ GROUP_READ WORLD_READ )

IF( NOT ALEMBIC_NO_TESTS )
	ADD_SUBDIRECTORY( Tests )
ENDIF()

