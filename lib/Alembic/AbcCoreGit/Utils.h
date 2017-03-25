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

#ifndef GLOBAL_TRACE_ENABLE
#define GLOBAL_TRACE_ENABLE 0
#endif /* GLOBAL_TRACE_ENABLE */

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
#ifndef TRACE
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
#endif

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

#ifndef TRACE
#define TRACE( TEXT )          do { } while(0)
#endif

#define UNIMPLEMENTED( TEXT )  do { } while(0)
#define TODO( TEXT )           do { } while(0)

#endif /* end !GLOBAL_TRACE_ENABLE */

#else /* start of ! DEBUG */

#ifndef TRACE
#define TRACE( TEXT )                   do { } while( 0 )
#endif
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
std::string dirname(const std::string& pathname);
bool mkpath(const std::string& path, mode_t mode = 0777);

std::string pathjoin(const std::string& p1, const std::string& p2); // actual OS paths
std::string v_pathjoin(const std::string& p1, const std::string& p2, char pathsep = '/'); // "virtual" paths

std::string relative_path(const std::string& abs, const std::string& base);

std::string real_path(const std::string& path);

std::string make_uuid();
std::string unique_temp_path(const std::string& prefix = "multiverse-", const std::string& suffix = "");
bool write_file(const std::string& pathname, const std::string& contents);
bool rmrf(const std::string& pathname);

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

/* POD printing/debugging */

void printPodValue(const std::string& msg, Alembic::Util::PlainOldDataType pod, void* buffer);
void printPodArray(const std::string& msg, Alembic::Util::PlainOldDataType pod, size_t count, void* buffer);

} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif
