##-*****************************************************************************
##
## Copyright (c) 2014,
##
## All rights reserved.
##
##-*****************************************************************************

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
  SampleStore.cpp
  SprImpl.cpp
  SpwImpl.cpp
  # StringReadUtil.cpp
  # StringWriteUtil.cpp
  Utils.cpp
  ReadWriteUtil.cpp
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
)

SET( SOURCE_FILES ${CXX_FILES} ${H_FILES} )

ADD_LIBRARY( AlembicAbcCoreGit ${SOURCE_FILES} )
TARGET_LINK_LIBRARIES( AlembicAbcCoreGit ${ALEMBIC_LIBGIT2_LIBS} ${ALEMBIC_MSGPACK_LIBS} ${ALEMBIC_JSONCPP_LIBS} )

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

