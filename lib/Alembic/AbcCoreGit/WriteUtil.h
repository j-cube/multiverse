//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

#ifndef _Alembic_AbcCoreGit_WriteUtil_h_
#define _Alembic_AbcCoreGit_WriteUtil_h_

#include <Alembic/AbcCoreGit/Foundation.h>
#include <Alembic/AbcCoreGit/Git.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
void HashPropertyHeader( const AbcA::PropertyHeader & iHeader,
                         Util::SpookyHash & ioHash );

} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif /* _Alembic_AbcCoreGit_WriteUtil_h_ */
