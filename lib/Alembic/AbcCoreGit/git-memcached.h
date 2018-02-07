/*****************************************************************************/
/*  multiverse - a next generation storage back-end for Alembic              */
/*                                                                           */
/*  Copyright 2015 J CUBE Inc. Tokyo, Japan.                                 */
/*                                                                           */
/*  Licensed under the Apache License, Version 2.0 (the "License");          */
/*  you may not use this file except in compliance with the License.         */
/*  You may obtain a copy of the License at                                  */
/*                                                                           */
/*      http://www.apache.org/licenses/LICENSE-2.0                           */
/*                                                                           */
/*  Unless required by applicable law or agreed to in writing, software      */
/*  distributed under the License is distributed on an "AS IS" BASIS,        */
/*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. */
/*  See the License for the specific language governing permissions and      */
/*  limitations under the License.                                           */
/*****************************************************************************/

#ifndef _ALEMBIC_GIT_MEMCACHED_H_
#define _ALEMBIC_GIT_MEMCACHED_H_

#include <git2.h>

extern "C" {

void memcached_backend__free(git_odb_backend *_backend);
int git_odb_backend_memcached(git_odb_backend **backend_out, const char *host, int port);

}

#endif /* _ALEMBIC_GIT_MEMCACHED_H_ */
