##-*****************************************************************************
##
## Copyright (C) 2014
##
## All rights reserved.
##
##-*****************************************************************************

# Once done this will define
#  LIBGIT2_FOUND - System has libgit2
#  LIBGIT2_INCLUDE_DIRS - The libgit2 include directories
#  LIBGIT2_LIBRARIES - The libraries needed to use libgit2
#  LIBGIT2_DEFINITIONS - Compiler switches required for using libgit2

# We shall worry about windowsification later.

find_package(PkgConfig)
pkg_check_modules(PC_LIBGIT2 QUIET libgit2)
set(LIBGIT2_DEFINITIONS ${PC_LIBGIT2_CFLAGS_OTHER})
set(LIBGIT2_LINK ${PC_LIBGIT2_LIBS})

# MESSAGE( STATUS "LIBGIT2 PC PC_LIBGIT2_INCLUDEDIR: ${PC_LIBGIT2_INCLUDEDIR}")
# MESSAGE( STATUS "LIBGIT2 PC PC_LIBGIT2_INCLUDE_DIRS: ${PC_LIBGIT2_INCLUDE_DIRS}")
# MESSAGE( STATUS "LIBGIT2 PC PC_LIBGIT2_CFLAGS: ${PC_LIBGIT2_CFLAGS}")
# MESSAGE( STATUS "LIBGIT2 PC PC_LIBGIT2_CFLAGS_OTHER: ${PC_LIBGIT2_CFLAGS_OTHER}")
# MESSAGE( STATUS "LIBGIT2 PC PC_LIBGIT2_LIBRARIES: ${PC_LIBGIT2_LIBRARIES}")
# MESSAGE( STATUS "LIBGIT2 PC PC_LIBGIT2_LIBRARY_DIRS: ${PC_LIBGIT2_LIBRARY_DIRS}")
# MESSAGE( STATUS "LIBGIT2 PC PC_LIBGIT2_LIBDIR: ${PC_LIBGIT2_LIBDIR}")
# MESSAGE( STATUS "LIBGIT2 PC PC_LIBGIT2_LDFLAGS: ${PC_LIBGIT2_LDFLAGS}")
# MESSAGE( STATUS "LIBGIT2 PC PC_LIBGIT2_LDFLAGS_OTHER: ${PC_LIBGIT2_LDFLAGS_OTHER}")

find_path(LIBGIT2_INCLUDE_DIR git2.h
          HINTS ${PC_LIBGIT2_INCLUDEDIR} ${PC_LIBGIT2_INCLUDE_DIRS}
          #PATH_SUFFIXES libgit2
          DOC "The directory where git2.h resides" )

find_library(LIBGIT2_LIBRARY NAMES git2 libgit2
             HINTS ${PC_LIBGIT2_LIBDIR} ${PC_LIBGIT2_LIBRARY_DIRS}
             DOC "The libgit2 library" )

set(LIBGIT2_LIBRARIES ${LIBGIT2_LIBRARY} )
set(LIBGIT2_INCLUDE_DIRS ${LIBGIT2_INCLUDE_DIR} )
set(LIBGIT2_INCLUDE_PATHS ${LIBGIT2_INCLUDE_DIR} )
set(LIBGIT2_CFLAGS ${PC_LIBGIT2_CFLAGS} )
set(LIBGIT2_CFLAGS_OTHER ${PC_LIBGIT2_CFLAGS_OTHER} )
set(LIBGIT2_LDFLAGS ${PC_LIBGIT2_LDFLAGS} )

SET( ALEMBIC_LIBGIT2_INCLUDE_PATH ${LIBGIT2_INCLUDE_DIR} )
SET( ALEMBIC_LIBGIT2_LIBS ${LIBGIT2_LIBRARY} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBGIT2_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(libgit2 DEFAULT_MSG
                                  LIBGIT2_LIBRARY LIBGIT2_INCLUDE_DIR)

mark_as_advanced( LIBGIT2_INCLUDE_DIR LIBGIT2_LIBRARY )

IF (LIBGIT2_FOUND)
  MESSAGE( STATUS "LIBGIT2 INCLUDE PATH: ${ALEMBIC_LIBGIT2_INCLUDE_PATH}" )
  MESSAGE( STATUS "LIBGIT2 LIBRARIES: ${ALEMBIC_LIBGIT2_LIBS}" )
ENDIF()

# IF (NOT $ENV{LIBGIT2_ROOT}x STREQUAL "x")
#   SET (LIBGIT2_ROOT $ENV{LIBGIT2_ROOT})
# ELSE()
#   SET (ENV{LIBGIT2_ROOT} ${LIBGIT2_ROOT})
# ENDIF()

# IF (NOT DEFINED LIBGIT2_ROOT)
#     IF ( ${CMAKE_HOST_UNIX} )
#         IF( ${DARWIN} )
#           # TODO: set to default install path when shipping out
#           SET (LIBGIT2_ROOT NOTFOUND )
#         ELSE()
#           # TODO: set to default install path when shipping out
#           SET (LIBGIT2_ROOT "/opt/jcube" )
#         ENDIF()
#     ELSE()
#         IF ( ${WINDOWS} )
#           # TODO: set to 32-bit or 64-bit path
#           SET (LIBGIT2_ROOT NOTFOUND )
#         ELSE()
#           SET (LIBGIT2_ROOT NOTFOUND )
#         ENDIF()
#     ENDIF()
# ELSE()
#   SET (LIBGIT2_ROOT ${LIBGIT2_ROOT} )
# ENDIF()

# SET (ALEMBIC_LIBGIT2_ROOT ${LIBGIT2_ROOT})

# SET (LIBRARY_PATHS 
#     ${ALEMBIC_LIBGIT2_ROOT}/lib
#     /opt/jcube/lib
#     ~/Library/Frameworks
#     /Library/Frameworks
#     /usr/local/lib
#     /usr/lib
#     /sw/lib
#     /opt/local/lib
#     /opt/csw/lib
#     /opt/lib
#     /usr/freeware/lib64
# )

# SET (INCLUDE_PATHS 
#     ${ALEMBIC_LIBGIT2_ROOT}/include
#     ${ALEMBIC_LIBGIT2_ROOT}/include/git2
#     /opt/jcube/include
#     ~/Library/Frameworks
#     /Library/Frameworks
#     /usr/local/include
#     /usr/include
#     /sw/include # Fink
#     /opt/local/include # DarwinPorts
#     /opt/csw/include # Blastwave
#     /opt/include
#     /usr/freeware/include
# )

# FIND_PATH (ALEMBIC_LIBGIT2_INCLUDE_PATH git2.h
#            PATHS
#            ${INCLUDE_PATHS}
#            NO_DEFAULT_PATH
#            NO_CMAKE_ENVIRONMENT_PATH
#            NO_CMAKE_PATH
#            NO_SYSTEM_ENVIRONMENT_PATH
#            NO_CMAKE_SYSTEM_PATH
#            DOC "The directory where git2.h resides" )

# FIND_LIBRARY ( ALEMBIC_LIBGIT2_LIB libgit2
#               PATHS 
#               ${LIBRARY_PATHS}
#               NO_DEFAULT_PATH
#               NO_CMAKE_ENVIRONMENT_PATH
#               NO_CMAKE_PATH
#               NO_SYSTEM_ENVIRONMENT_PATH
#               NO_CMAKE_SYSTEM_PATH
#               DOC "The libgit2 library" )


# SET ( LIBGIT2_FOUND TRUE )

# IF ( ${ALEMBIC_LIBGIT2_INCLUDE_PATH} STREQUAL "ALEMBIC_LIBGIT2_INCLUDE_PATH-NOTFOUND" ) 
#   MESSAGE( STATUS "libgit2 include path not found, disabling" )
#   SET ( LIBGIT2_FOUND FALSE )
# ENDIF()

# IF ( ${ALEMBIC_LIBGIT2_LIB} STREQUAL "ALEMBIC_LIBGIT2_LIB-NOTFOUND" ) 
#   MESSAGE( STATUS "libgit2 libraries not found, disabling" )
#   SET ( LIBGIT2_FOUND FALSE )
#   SET ( ALEMBIC_LIBGIT2_LIBS NOTFOUND )
# ENDIF()

# IF (LIBGIT2_FOUND)
#   MESSAGE( STATUS "LIBGIT2 INCLUDE PATH: ${ALEMBIC_LIBGIT2_INCLUDE_PATH}" )
#   SET ( ALEMBIC_LIBGIT2_LIBS ${ALEMBIC_LIBGIT2_LIB} )
# ENDIF()
