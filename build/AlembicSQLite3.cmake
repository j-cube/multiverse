##-*****************************************************************************
##
## Copyright (C) 2014
##
## All rights reserved.
##
##-*****************************************************************************

# FIND_PACKAGE( SQLite3 REQUIRED )
FIND_PACKAGE( SQLite3 )

IF( SQLITE3_FOUND )
  SET( ALEMBIC_SQLITE3_FOUND 1 CACHE STRING "Set to 1 if SQLite3 is found, 0 otherwise" )
ELSE()
  SET( ALEMBIC_SQLITE3_FOUND 0 CACHE STRING "Set to 1 if SQLite3 is found, 0 otherwise" )
ENDIF()
