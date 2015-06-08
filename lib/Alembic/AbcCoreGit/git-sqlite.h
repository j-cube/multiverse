//-*****************************************************************************
//
// Copyright (c) 2015,
//
// All rights reserved.
//
//-*****************************************************************************

#ifndef _ALEMBIC_GIT_SQLITE_H_
#define _ALEMBIC_GIT_SQLITE_H_

#include <git2.h>

extern "C" {

void sqlite_backend__free(git_odb_backend *_backend);
int git_odb_backend_sqlite(git_odb_backend **backend_out, const char *sqlite_db);

}

#endif /* _ALEMBIC_GIT_SQLITE_H_ */
