##-*****************************************************************************
##
## Copyright (C) 2014
##
## All rights reserved.
##
##-*****************************************************************************

# FIND_PACKAGE( LibMemCached REQUIRED )
FIND_PACKAGE( LibMemCached )

IF( LIBMEMCACHED_FOUND )
  SET( ALEMBIC_LIBMEMCACHED_FOUND 1 CACHE STRING "Set to 1 if LibMemCached is found, 0 otherwise" )
ELSE()
  SET( ALEMBIC_LIBMEMCACHED_FOUND 0 CACHE STRING "Set to 1 if LibMemCached is found, 0 otherwise" )
ENDIF()
