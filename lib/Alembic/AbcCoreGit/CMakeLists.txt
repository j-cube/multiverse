##-****************************************************************************
##  multiverse - a next generation storage back-end for Alembic
##  Copyright 2015 J CUBE Inc. Tokyo, Japan.             
##
##  Licensed under the Apache License, Version 2.0 (the "License");
##  you may not use this file except in compliance with the License.
##  You may obtain a copy of the License at
##                                                                          
##      http://www.apache.org/licenses/LICENSE-2.0
##
##  Unless required by applicable law or agreed to in writing, software
##  distributed under the License is distributed on an "AS IS" BASIS,
##  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
##  See the License for the specific language governing permissions and
##  limitations under the License.
##-****************************************************************************

# Choose custom backend
SET( USE_CUSTOM_BACKEND "NO" CACHE STRING "Choose custom backend" )
SET_PROPERTY( CACHE USE_CUSTOM_BACKEND PROPERTY STRINGS NO SQLITE3 MEMCACHED MILLIWAYS )

ADD_DEFINITIONS( -DMILLIWAYS_DISABLE_COMPRESSION=0 -DMILLIWAYS_LZ4_ACCELERATION=1 )
# ADD_DEFINITIONS( -DMILLIWAYS_DISABLE_COMPRESSION=1 -DMILLIWAYS_LZ4_ACCELERATION=0 )

# C++ files for this project
LIST(APPEND CXX_FILES
  AbcCoreGit/AprImpl.cpp
  AbcCoreGit/ApwImpl.cpp
  AbcCoreGit/ArImpl.cpp
  AbcCoreGit/AwImpl.cpp
  # AbcCoreGit/CacheImpl.cpp
  AbcCoreGit/CprData.cpp
  AbcCoreGit/CprImpl.cpp
  AbcCoreGit/CpwData.cpp
  AbcCoreGit/CpwImpl.cpp
  # AbcCoreGit/DataTypeRegistry.cpp
  # # AbcCoreGit/GitHierarchy.cpp
  # # AbcCoreGit/GitHierarchyReader.cpp
  # # AbcCoreGit/GitHierarchyWriter.cpp
  AbcCoreGit/Git.cpp
  # AbcCoreGit/GitUtil.cpp
  AbcCoreGit/MetaDataMap.cpp
  AbcCoreGit/OrData.cpp
  AbcCoreGit/OrImpl.cpp
  AbcCoreGit/OwData.cpp
  AbcCoreGit/OwImpl.cpp
  # AbcCoreGit/ReadUtil.cpp
  AbcCoreGit/ReadWrite.cpp
  AbcCoreGit/KeyStore.cpp
  AbcCoreGit/SampleStore.cpp
  AbcCoreGit/SprImpl.cpp
  AbcCoreGit/SpwImpl.cpp
  # AbcCoreGit/StringReadUtil.cpp
  # AbcCoreGit/StringWriteUtil.cpp
  AbcCoreGit/Utils.cpp
  AbcCoreGit/ReadWriteUtil.cpp
  AbcCoreGit/msgpack_support.cpp
  AbcCoreGit/JSON.cpp
  AbcCoreGit/git-milliways.cpp
  AbcCoreGit/milliways/lz4.c
)
SET(CXX_FILES "${CXX_FILES}" PARENT_SCOPE)

# LIST(APPEND C_FILES
#   git-memcached.c
#   git-sqlite.c
# )
# SET(C "${C_FILES}" PARENT_SCOPE)

# SET( H_FILES
#   All.h
#   AprImpl.h
#   ApwImpl.h
#   ArImpl.h
#   AwImpl.h
#   # CacheImpl.h
#   CprData.h
#   CprImpl.h
#   CpwData.h
#   CpwImpl.h
#   # DataTypeRegistry.h
#   # # GitHierarchy.h
#   # # GitHierarchyReader.h
#   # # GitHierarchyWriter.h
#   Git.h
#   # GitUtil.h
#   # Foundation.h
#   MetaDataMap.h
#   OrData.h
#   OrImpl.h
#   OwData.h
#   OwImpl.h
#   # ReadUtil.h
#   ReadWrite.h
#   KeyStore.h
#   SampleStore.h
#   # SimplePrImpl.h
#   # SimplePwImpl.h
#   # SprImpl.h
#   SpwImpl.h
#   # StringReadUtil.h
#   # StringWriteUtil.h
#   Utils.h
#   ReadWriteUtil.h
#   WrittenSampleMap.h
#   # WrittenArraySampleMap.h
#   msgpack_support.h
#   JSON.h
# )

# SET( SOURCE_FILES ${CXX_FILES} ${H_FILES} )

# ADD_LIBRARY( AlembicAbcCoreGit ${SOURCE_FILES} )

# IF( USE_CUSTOM_BACKEND STREQUAL "SQLITE3" )
#   TARGET_LINK_LIBRARIES( AlembicAbcCoreGit ${ALEMBIC_LIBGIT2_LIBS} ${ALEMBIC_MSGPACK_LIBS} ${ALEMBIC_JSONCPP_LIBS} ${ALEMBIC_SQLITE3_LIBS} )
# ELSEIF( USE_CUSTOM_BACKEND STREQUAL "MEMCACHED" )
#   TARGET_LINK_LIBRARIES( AlembicAbcCoreGit ${ALEMBIC_LIBGIT2_LIBS} ${ALEMBIC_MSGPACK_LIBS} ${ALEMBIC_JSONCPP_LIBS} ${ALEMBIC_LIBMEMCACHED_LIBS} )
# ELSEIF( USE_CUSTOM_BACKEND STREQUAL "MILLIWAYS" )
#   TARGET_LINK_LIBRARIES( AlembicAbcCoreGit ${ALEMBIC_LIBGIT2_LIBS} ${ALEMBIC_MSGPACK_LIBS} ${ALEMBIC_JSONCPP_LIBS} )
# ELSE()
#   TARGET_LINK_LIBRARIES( AlembicAbcCoreGit ${ALEMBIC_LIBGIT2_LIBS} ${ALEMBIC_MSGPACK_LIBS} ${ALEMBIC_JSONCPP_LIBS} )
# ENDIF()

# INSTALL( TARGETS AlembicAbcCoreGit
#          LIBRARY DESTINATION lib
#          ARCHIVE DESTINATION lib/static )

INSTALL( FILES
         All.h
         ReadWrite.h
         DESTINATION include/Alembic/AbcCoreGit
         PERMISSIONS OWNER_READ GROUP_READ WORLD_READ )

ADD_SUBDIRECTORY( milliways )

IF (USE_TESTS)
    ADD_SUBDIRECTORY(Tests)
ENDIF()
