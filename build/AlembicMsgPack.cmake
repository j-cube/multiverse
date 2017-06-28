##-*****************************************************************************
##
## Copyright (C) 2014
##
## All rights reserved.
##
##-*****************************************************************************

# FIND_PACKAGE( MsgPack REQUIRED )
FIND_PACKAGE( MsgPack )

IF( MSGPACK_FOUND )
  SET( ALEMBIC_MSGPACK_FOUND 1 CACHE STRING "Set to 1 if MsgPack is found, 0 otherwise" )
ELSE()
  SET( ALEMBIC_MSGPACK_FOUND 0 CACHE STRING "Set to 1 if MsgPack is found, 0 otherwise" )
ENDIF()
