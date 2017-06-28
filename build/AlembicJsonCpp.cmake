##-*****************************************************************************
##
## Copyright (C) 2014
##
## All rights reserved.
##
##-*****************************************************************************

# FIND_PACKAGE( JsonCpp REQUIRED )
FIND_PACKAGE( JsonCpp )

IF( JsonCpp_FOUND )
  SET( ALEMBIC_JSONCPP_FOUND 1 CACHE STRING "Set to 1 if JsonCpp is found, 0 otherwise" )
ELSE()
  SET( ALEMBIC_JSONCPP_FOUND 0 CACHE STRING "Set to 1 if JsonCpp is found, 0 otherwise" )
ENDIF()
