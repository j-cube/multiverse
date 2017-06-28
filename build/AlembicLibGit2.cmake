##-*****************************************************************************
##
## Copyright (C) 2014
##
## All rights reserved.
##
##-*****************************************************************************

# FIND_PACKAGE( LibGit2 REQUIRED )
FIND_PACKAGE( LibGit2 )

IF( LIBGIT2_FOUND )
  SET( ALEMBIC_LIBGIT2_FOUND 1 CACHE STRING "Set to 1 if LibGit2 is found, 0 otherwise" )
ELSE()
  SET( ALEMBIC_LIBGIT2_FOUND 0 CACHE STRING "Set to 1 if LibGit2 is found, 0 otherwise" )
ENDIF()
