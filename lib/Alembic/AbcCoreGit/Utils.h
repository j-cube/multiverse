//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

#ifndef _Alembic_AbcCoreGit_Utils_h_
#define _Alembic_AbcCoreGit_Utils_h_

#include <Alembic/AbcCoreGit/Foundation.h>
#include <git2.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include <boost/current_function.hpp>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

#if defined(DEBUG) && (! defined(NDEBUG))

#define PRINT_SHORT_FUNCNAME

#define DBG_FUNCNAME BOOST_CURRENT_FUNCTION

// http://ascii-table.com/ansi-escape-sequences.php

#define ANSI_RESET         "\x1b[0m"
#define ANSI_BOLD          "\x1b[1m"
#define ANSI_NEGATIVE      "\x1b[7m"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_BOLD_RED      "\x1b[1;31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_BOLD_GREEN    "\x1b[1;32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_BOLD_YELLOW   "\x1b[1;33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_BOLD_BLUE     "\x1b[1;34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_BOLD_MAGENTA  "\x1b[1;35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_BOLD_CYAN     "\x1b[1;36m"

#define _T_FUNC             ANSI_COLOR_GREEN << DBG_FUNCNAME << ANSI_RESET
#define _T_TRACE            "[" << ANSI_BOLD_YELLOW << "TRACE" << ANSI_RESET << "] "
#define _T_UNIMPLEMENTED    "[" << ANSI_COLOR_RED << "UNIMPLEMENTED" << ANSI_RESET << "] "
#define _T_TODO             "[" << ANSI_COLOR_RED << "TODO" << ANSI_RESET << "] "

#define TRACE( TEXT )                             \
do                                                               \
{                                                                \
    std::cerr << _T_TRACE <<                                     \
        ANSI_BOLD << TEXT << ANSI_RESET <<                       \
        std::endl << "\tin " << _T_FUNC << std::endl;            \
    std::cerr.flush();                                           \
}                                                                \
while( 0 )

#define UNIMPLEMENTED( TEXT )                     \
do                                                               \
{                                                                \
    std::cerr << _T_UNIMPLEMENTED <<                             \
        ANSI_BOLD_YELLOW << TEXT << ANSI_RESET <<                \
        std::endl << "\tin " << DBG_FUNCNAME << std::endl;       \
    std::cerr.flush();                                           \
}                                                                \
while( 0 )

#define TODO( TEXT )                              \
do                                                               \
{                                                                \
    std::cerr << _T_TODO <<                                      \
        ANSI_COLOR_YELLOW << TEXT << ANSI_RESET <<               \
        std::endl << "\tin " << DBG_FUNCNAME << std::endl;       \
    std::cerr.flush();                                           \
}                                                                \
while( 0 )

#else /* start of ! DEBUG */

#define TRACE( TEXT )                   do { } while( 0 )
#define UNIMPLEMENTED( TEXT )           do { } while( 0 )
#define TODO( TEXT )                    do { } while( 0 )

#endif /* end of ! DEBUG */

int mkpath(const std::string& path, mode_t mode = 0777);

} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif
