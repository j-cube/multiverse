##-*****************************************************************************
##
## Copyright (C) 2014
##
## All rights reserved.
##
##-*****************************************************************************

# Once done this will define
#  MSGPACK_FOUND - System has MessagePack library
#  MSGPACK_INCLUDE_DIR
#  MSGPACK_INCLUDE_DIRS - The MessagePack include directories
#  MSGPACK_LIBRARIES - The libraries to link against to use MessagePack
#  MSGPACK_DEFINITIONS - Compiler switches required for using MessagePack

# We shall worry about windowsification later.

IF (MSGPACK_LIBRARIES AND MSGPACK_INCLUDE_DIR)
    SET(MSGPACK_FIND_QUIETLY TRUE) # Already in cache, be silent
ENDIF (MSGPACK_LIBRARIES AND MSGPACK_INCLUDE_DIR)

find_package(PkgConfig)
pkg_check_modules(PC_MSGPACK QUIET msgpack)
set(MSGPACK_DEFINITIONS ${PC_MSGPACK_CFLAGS_OTHER})
set(MSGPACK_LINK ${PC_MSGPACK_LIBS})

find_path(MSGPACK_INCLUDE_DIR msgpack.hpp
          HINTS ${PC_MSGPACK_INCLUDEDIR} ${PC_MSGPACK_INCLUDE_DIRS}
          #PATH_SUFFIXES libgit2
          DOC "The directory where msgpack.hpp resides" )

find_library(MSGPACK_LIBRARY NAMES msgpack
             HINTS ${PC_MSGPACK_LIBDIR} ${PC_MSGPACK_LIBRARY_DIRS}
             DOC "The msgpack library" )

# FIND_PATH(MSGPACK_INCLUDE_DIR msgpack.hpp
#     /opt/jcube/include
#     /usr/include
#     /usr/include/msgpack
#     /usr/local/include
#     /usr/local/include/msgpack
# )

# FIND_LIBRARY(MSGPACK_LIBRARY NAMES msgpack PATHS
#     /opt/jcube/lib
#     /usr/lib
#     /usr/local/lib
#     /usr/local/lib/msgpack
# )

# Copy the results to the output variables.
IF (MSGPACK_INCLUDE_DIR AND MSGPACK_LIBRARY)
	SET(MSGPACK_FOUND 1)
	SET(MSGPACK_LIBRARIES ${MSGPACK_LIBRARY})
	SET(MSGPACK_INCLUDE_DIRS ${MSGPACK_INCLUDE_DIR})
	MESSAGE(STATUS "Found these MessagePack (msgpack) libs: ${MSGPACK_LIBRARIES}")
ELSE (MSGPACK_INCLUDE_DIR AND MSGPACK_LIBRARY)
	SET(MSGPACK_FOUND 0)
	SET(MSGPACK_LIBRARIES)
	SET(MSGPACK_INCLUDE_DIRS)
ENDIF (MSGPACK_INCLUDE_DIR AND MSGPACK_LIBRARY)

# Report the results.
IF (NOT MSGPACK_FOUND)
    SET(MSGPACK_DIR_MESSAGE "MessagePack (msgpack) was not found. Make sure MSGPACK_LIBRARY and MSGPACK_INCLUDE_DIR are set.")
	IF (NOT MSGPACK_FIND_QUIETLY)
		MESSAGE(STATUS "${MSGPACK_DIR_MESSAGE}")
	ELSE (NOT MSGPACK_FIND_QUIETLY)
		IF (MSGPACK_FIND_REQUIRED)
			MESSAGE(FATAL_ERROR "${MSGPACK_DIR_MESSAGE}")
		ENDIF (MSGPACK_FIND_REQUIRED)
	ENDIF (NOT MSGPACK_FIND_QUIETLY)
ENDIF (NOT MSGPACK_FOUND)

SET( ALEMBIC_MSGPACK_INCLUDE_PATH ${MSGPACK_INCLUDE_DIRS} )
SET( ALEMBIC_MSGPACK_LIBS ${MSGPACK_LIBRARIES} )

MARK_AS_ADVANCED(
    MSGPACK_INCLUDE_DIRS
    MSGPACK_LIBRARIES
)

IF (MSGPACK_FOUND)
  MESSAGE( STATUS "MSGPACK INCLUDE PATH: ${ALEMBIC_MSGPACK_INCLUDE_PATH}" )
  MESSAGE( STATUS "MSGPACK LIBRARIES: ${ALEMBIC_MSGPACK_LIBS}" )
ENDIF()
