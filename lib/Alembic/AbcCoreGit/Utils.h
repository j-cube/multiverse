/*****************************************************************************/
/*  multiverse - a next generation storage back-end for Alembic              */
/*  Copyright (C) 2015 J CUBE Inc. Tokyo, Japan. All Rights Reserved.        */
/*                                                                           */
/*  This program is free software: you can redistribute it and/or modify     */
/*  it under the terms of the GNU General Public License as published by     */
/*  the Free Software Foundation, either version 3 of the License, or        */
/*  (at your option) any later version.                                      */
/*                                                                           */
/*  This program is distributed in the hope that it will be useful,          */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of           */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            */
/*  GNU General Public License for more details.                             */
/*                                                                           */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    */
/*                                                                           */
/*    J CUBE Inc.                                                            */
/*    6F Azabu Green Terrace                                                 */
/*    3-20-1 Minami-Azabu, Minato-ku, Tokyo                                  */
/*    info@-jcube.jp                                                         */
/*****************************************************************************/

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

#define GLOBAL_TRACE_ENABLE 1

extern bool trace_enable;

inline bool TRACE_ENABLE(bool enable = true)
{
    bool old = trace_enable;
    trace_enable = enable;
    return old;
}

inline bool TRACE_ENABLED() { return trace_enable; }

#if defined(DEBUG) && (! defined(NDEBUG))

#define PRINT_SHORT_FUNCNAME

#define DBG_FUNCNAME BOOST_CURRENT_FUNCTION

// http://ascii-table.com/ansi-escape-sequences.php

#ifdef _MSC_VER

#define ANSI_RESET         ""
#define ANSI_BOLD          ""
#define ANSI_NEGATIVE      ""
#define ANSI_COLOR_RED     ""
#define ANSI_BOLD_RED      ""
#define ANSI_COLOR_GREEN   ""
#define ANSI_BOLD_GREEN    ""
#define ANSI_COLOR_YELLOW  ""
#define ANSI_BOLD_YELLOW   ""
#define ANSI_COLOR_BLUE    ""
#define ANSI_BOLD_BLUE     ""
#define ANSI_COLOR_MAGENTA ""
#define ANSI_BOLD_MAGENTA  ""
#define ANSI_COLOR_CYAN    ""
#define ANSI_BOLD_CYAN     ""

#else /* start of !defined(_MSC_VER) */

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

#endif /* end of !defined(_MSC_VER) */

#define _T_FUNC             ANSI_COLOR_GREEN << DBG_FUNCNAME << ANSI_RESET
#define _T_TRACE            "[" << ANSI_BOLD_YELLOW << "TRACE" << ANSI_RESET << "] "
#define _T_UNIMPLEMENTED    "[" << ANSI_COLOR_RED << "UNIMPLEMENTED" << ANSI_RESET << "] "
#define _T_TODO             "[" << ANSI_COLOR_RED << "TODO" << ANSI_RESET << "] "

#if GLOBAL_TRACE_ENABLE
#define TRACE( TEXT )                             \
do                                                               \
{                                                                \
    if (TRACE_ENABLED()) {                                       \
        std::cerr << _T_TRACE <<                                 \
            ANSI_BOLD << TEXT << ANSI_RESET <<                   \
            std::endl << "\tin " << _T_FUNC << std::endl;        \
        std::cerr.flush();                                       \
    }                                                            \
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

#else /* start !GLOBAL_TRACE_ENABLE */

#define TRACE( TEXT )          do { } while(0)
#define UNIMPLEMENTED( TEXT )  do { } while(0)
#define TODO( TEXT )           do { } while(0)

#endif /* end !GLOBAL_TRACE_ENABLE */

#else /* start of ! DEBUG */

#define TRACE( TEXT )                   do { } while( 0 )
#define UNIMPLEMENTED( TEXT )           do { } while( 0 )
#define TODO( TEXT )                    do { } while( 0 )

#endif /* end of ! DEBUG */

#ifdef _MSC_VER
typedef int mode_t;
#endif /* _MSC_VER */

size_t strlcpy(char *dst, const char *src, size_t siz);

bool file_exists(const std::string& pathname);
bool isdir(const std::string& pathname);
bool isfile(const std::string& pathname);
bool mkpath(const std::string& path, mode_t mode = 0777);

std::string pathjoin(const std::string& p1, const std::string& p2); // actual OS paths
std::string v_pathjoin(const std::string& p1, const std::string& p2, char pathsep = '/'); // "virtual" paths

std::string relative_path(const std::string& abs, const std::string& base);

std::string real_path(const std::string& path);

double time_us();
double time_ms();

class Profile
{
public:
    Profile() {}

    static void add_json_creation(double amount)   { s_json_creation += amount; }
    static void add_json_output(double amount)     { s_json_output += amount; }
    static void add_disk_write(double amount)      { s_disk_write += amount; }
    static void add_git(double amount)             { s_git += amount; }

    static double json_creation()                  { return s_json_creation; }
    static double json_output()                    { return s_json_output; }
    static double disk_write()                     { return s_disk_write; }
    static double git()                            { return s_git; }

private:
    static double s_json_creation;
    static double s_json_output;
    static double s_disk_write;
    static double s_git;
};

std::ostream& operator<< ( std::ostream& out, const Profile& obj );

} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif
