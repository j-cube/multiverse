//-*****************************************************************************
//
// Copyright (c) 2015,
//
// All rights reserved.
//
//-*****************************************************************************

#ifndef _ALEMBIC_GIT_MEMCACHED_H_
#define _ALEMBIC_GIT_MEMCACHED_H_

#include <git2.h>

extern "C" {

void memcached_backend__free(git_odb_backend *_backend);
int git_odb_backend_memcached(git_odb_backend **backend_out, const char *host, int port);

}

#endif /* _ALEMBIC_GIT_MEMCACHED_H_ */
