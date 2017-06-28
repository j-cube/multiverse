# -*- cmake -*-

# - Find JsonCpp library
# Find the JSONCpp includes and library
# This module defines
#  JsonCpp_INCLUDE_DIR, where to find json.h, etc.
#  JsonCpp_INCLUDE_DIRS - The JsonCpp include directories
#  JsonCpp_LIBRARIES - The libraries to link against to use JsonCpp
#  JsonCpp_FOUND

# We shall worry about windowsification later.

FIND_PATH(JsonCpp_INCLUDE_DIR json/json.h
/opt/jcube/include/jsoncpp
/opt/jcube/include
/usr/local/include/jsoncpp
/usr/local/include
/usr/include/jsoncpp
/usr/include
)

# Get the GCC compiler version
EXEC_PROGRAM(${CMAKE_CXX_COMPILER}
            ARGS ${CMAKE_CXX_COMPILER_ARG1} -dumpversion
            OUTPUT_VARIABLE _gcc_COMPILER_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
            )

SET(JsonCpp_NAMES ${JsonCpp_NAMES} libjsoncpp.so libjson.so libjson_linux-gcc-${_gcc_COMPILER_VERSION}_libmt.so libjsoncpp.dylib libjson.dylib libjsoncpp.a libjson.a)
FIND_LIBRARY(JsonCpp_LIBRARY
  NAMES ${JsonCpp_NAMES}
  PATHS /opt/jcube/lib /usr/local/lib /usr/lib
  )

IF (JsonCpp_LIBRARY AND JsonCpp_INCLUDE_DIR)
    SET(JsonCpp_FOUND "YES")
    SET(JsonCpp_LIBRARIES ${JsonCpp_LIBRARY})
    SET(JsonCpp_INCLUDE_DIRS ${JsonCpp_INCLUDE_DIR})
ELSE (JsonCpp_LIBRARY AND JsonCpp_INCLUDE_DIR)
  SET(JsonCpp_FOUND "NO")
ENDIF (JsonCpp_LIBRARY AND JsonCpp_INCLUDE_DIR)

IF (JsonCpp_FOUND)
   IF (NOT JsonCpp_FIND_QUIETLY)
      MESSAGE(STATUS "Found JsonCpp: ${JsonCpp_LIBRARIES}")
   ENDIF (NOT JsonCpp_FIND_QUIETLY)
ELSE (JsonCpp_FOUND)
   IF (JsonCpp_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find JsonCpp library")
   ENDIF (JsonCpp_FIND_REQUIRED)
ENDIF (JsonCpp_FOUND)

# Deprecated declarations.
SET (NATIVE_JsonCpp_INCLUDE_PATH ${JsonCpp_INCLUDE_DIR} )
GET_FILENAME_COMPONENT (NATIVE_JsonCpp_LIB_PATH ${JsonCpp_LIBRARY} PATH)

SET( JSONCPP_FOUND ${JsonCpp_FOUND} )
SET( ALEMBIC_JSONCPP_INCLUDE_PATH ${JsonCpp_INCLUDE_DIRS} )
SET( ALEMBIC_JSONCPP_LIBS ${JsonCpp_LIBRARIES} )

MARK_AS_ADVANCED(
  JsonCpp_INCLUDE_DIRS
  JsonCpp_LIBRARIES
)

IF (JSONCPP_FOUND)
  MESSAGE( STATUS "JSONCPP INCLUDE PATH: ${ALEMBIC_JSONCPP_INCLUDE_PATH}" )
  MESSAGE( STATUS "JSONCPP LIBRARIES: ${ALEMBIC_JSONCPP_LIBS}" )
ENDIF()
