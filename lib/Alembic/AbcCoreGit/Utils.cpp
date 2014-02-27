//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

#include <Alembic/AbcCoreGit/Utils.h>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdlib>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdlib>

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <libgen.h>

#include <sys/stat.h>
#include <sys/types.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

/* Function with behaviour like `mkdir -p'  */
int mkpath(const std::string& path, mode_t mode)
{
    int rv = -1;

    //TRACE("mkpath('" << path << "')");

    if ((path == ".") || (path == "/"))
    	return 0;

    char *up_c = strdup(path.c_str());
    if (! up_c)
    	return -1;
    dirname(up_c);
    std::string up = up_c;
    free(up_c);

    rv = mkpath(up, mode);
    if ((rv < 0) && (errno != EEXIST))
		return rv;

    rv = mkdir(path.c_str(), mode);
    if ((rv < 0) && (errno == EEXIST))
    	rv = 0;
    return rv;
}

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreGit
} // End namespace Alembic
