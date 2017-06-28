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

#include <Alembic/AbcCoreGit/Utils.h>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <stdexcept>

#include <errno.h>

#include <boost/system/system_error.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/locale.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

bool trace_enable = true;

/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t strlcpy(char *dst, const char *src, size_t siz)
{
    register char *d = dst;
    register const char *s = src;
    register size_t n = siz;

    /* Copy as many bytes as will fit */
    if (n != 0 && --n != 0) {
        do {
            if ((*d++ = *s++) == 0)
                break;
        } while (--n != 0);
    }

    /* Not enough room in dst, add NUL and traverse rest of src */
    if (n == 0) {
        if (siz != 0)
            *d = '\0';      /* NUL-terminate dst */
        while (*s++)
            ;
    }

    return(s - src - 1);    /* count does not include NUL */
}

bool file_exists(const std::string& pathname)
{
    boost::filesystem::path p(pathname);
    boost::system::error_code ec;

    return boost::filesystem::exists(p, ec);
}

bool isdir(const std::string& pathname)
{
    boost::filesystem::path p(pathname);
    boost::system::error_code ec;

    return boost::filesystem::is_directory(p, ec);
}

bool isfile(const std::string& pathname)
{
    boost::filesystem::path p(pathname);
    boost::system::error_code ec;

    return (boost::filesystem::is_regular_file(p, ec) || boost::filesystem::is_symlink(p, ec));
}

std::string dirname(const std::string& pathname)
{
    boost::filesystem::path p(pathname);
    boost::filesystem::path dir = p.parent_path();

#ifdef _MSC_VER
    return boost::locale::conv::utf_to_utf<char>(dir.native());
#else
    return dir.native();
#endif
}

/* Function with behaviour like `mkdir -p'  */
bool mkpath(const std::string& pathname, mode_t mode)
{
    boost::filesystem::path p(pathname);
    boost::system::error_code ec;

    if (boost::filesystem::exists(p, ec))
    {
        return boost::filesystem::is_directory(p, ec);
    }

    boost::filesystem::create_directories(p, ec);

    return boost::filesystem::is_directory(p, ec);
}

// actual Operating System pathnames
std::string pathjoin(const std::string& pathname1, const std::string& pathname2)
{
    if (pathname1.length() == 0)
        return pathname2;
    if (pathname2.length() == 0)
        return pathname1;

    ABCA_ASSERT(pathname1.length() > 0, "invalid string length");
    ABCA_ASSERT(pathname2.length() > 0, "invalid string length");

    boost::filesystem::path p1(pathname1);
    boost::filesystem::path p2(pathname2);

#ifdef _MSC_VER
    std::string joined_path = boost::locale::conv::utf_to_utf<char>((p1 / p2).native());
    std::replace(joined_path.begin(), joined_path.end(), '\\' , '/');
    return joined_path;
#else
    return (p1 / p2).native();
#endif
}

// "virtual" pathnames
std::string v_pathjoin(const std::string& p1, const std::string& p2, char pathsep)
{
    if (p1.length() == 0)
        return p2;
    if (p2.length() == 0)
        return p1;

    ABCA_ASSERT(p1.length() > 0, "invalid string length");
    ABCA_ASSERT(p2.length() > 0, "invalid string length");

    char p_last_ch = *p1.rbegin();
    char first_ch = p2[0];

    if ((p_last_ch == pathsep) && (first_ch == pathsep))
    {
        if (p2.length() == 1)
            return p1;
        if (p1.length() == 1)
            return p2;
        return p1 + p2.substr(1);
    }

    if ((p_last_ch == pathsep) || (first_ch == pathsep))
        return p1 + p2;
    return p1 + pathsep + p2;

}

std::string path_separator()
{
    return "/";
}

bool ends_with_separator(const std::string& pathname)
{
    std::string sep = path_separator();

    return ((pathname.size() >= sep.size()) &&
            (pathname.compare(pathname.size() - sep.size(), sep.size(), sep) == 0));
}
//#define HAS_DOT_SLASH

/**
 * https://svn.boost.org/trac/boost/ticket/1976#comment:2
 * 
 * "The idea: uncomplete(/foo/new, /foo/bar) => ../new
 *  The use case for this is any time you get a full path (from an open dialog, perhaps)
 *  and want to store a relative path so that the group of files can be moved to a different
 *  directory without breaking the paths. An IDE would be a simple example, so that the
 *  project file could be safely checked out of subversion."
 * 
 * ALGORITHM:
 *  iterate path and base
 * compare all elements so far of path and base
 * whilst they are the same, no write to output
 * when they change, or one runs out:
 *   write to output, ../ times the number of remaining elements in base
 *   write to output, the remaining elements in path
 *
 * From:
 *   http://stackoverflow.com/questions/5772992/get-relative-path-from-two-absolute-paths
 *   http://stackoverflow.com/a/5773008
 */
static boost::filesystem::path
naive_uncomplete(boost::filesystem::path p, boost::filesystem::path base, bool canonicalize = true)
{
    using boost::filesystem::path;
#if HAS_DOT_SLASH
    using boost::filesystem::dot;
    using boost::filesystem::slash;
#endif /* HAS_DOT_SLASH */

    // when we canonicalize, the path must exist
    if (canonicalize)
    {
        p = boost::filesystem::canonical(p);
        base = boost::filesystem::canonical(base);
    }

    p.make_preferred();
    base.make_preferred();

    if (p == base)
        return "./";
        /*!! this breaks stuff if path is a filename rather than a directory,
             which it most likely is... but then base shouldn't be a filename so... */

    boost::filesystem::path from_path, from_base, output;

    boost::filesystem::path::iterator path_it = p.begin(),    path_end = p.end();
    boost::filesystem::path::iterator base_it = base.begin(), base_end = base.end();

    // check for emptiness
    if ((path_it == path_end) || (base_it == base_end))
        throw std::runtime_error("path or base was empty; couldn't generate relative path");

#ifdef WIN32
    // drive letters are different; don't generate a relative path
    if (*path_it != *base_it)
        return p;

    // now advance past drive letters; relative paths should only go up
    // to the root of the drive and not past it
    ++path_it, ++base_it;
#endif

    // Cache system-dependent dot, double-dot and slash strings
#if HAS_DOT_SLASH
    const std::string _dot  = std::string(1, dot<path>::value);
    const std::string _dots = std::string(2, dot<path>::value);
    const std::string _sep = std::string(1, slash<path>::value);
#else
    const std::string _dot  = ".";
    const std::string _dots = "..";
    boost::filesystem::path p_slash("/");
    boost::filesystem::path::string_type preferredSlash = p_slash.make_preferred().native();
#ifdef _MSC_VER
    const std::string _sep = boost::locale::conv::utf_to_utf<char>(preferredSlash);
#else
    const std::string _sep = preferredSlash;
#endif
#endif

    // iterate over path and base
    while (true) {

        // compare all elements so far of path and base to find greatest common root;
        // when elements of path and base differ, or run out:
        if ((path_it == path_end) || (base_it == base_end) || (*path_it != *base_it)) {

            // write to output, ../ times the number of remaining elements in base;
            // this is how far we've had to come down the tree from base to get to the common root
            for (; base_it != base_end; ++base_it) {
                if (*base_it == _dot)
                    continue;
                else if (*base_it == _sep)
                    continue;

                output /= "../";
            }

            // write to output, the remaining elements in path;
            // this is the path relative from the common root
            boost::filesystem::path::iterator path_it_start = path_it;
            for (; path_it != path_end; ++path_it) {

                if (path_it != path_it_start)
                    output /= "/";

                if (*path_it == _dot)
                    continue;
                if (*path_it == _sep)
                    continue;

                output /= *path_it;
            }

            break;
        }

        // add directory level to both paths and continue iteration
        from_path /= path(*path_it);
        from_base /= path(*base_it);

        ++path_it, ++base_it;
    }

    return output;
}

std::string relative_path(const std::string& abs_path, const std::string& base)
{
    boost::filesystem::path rel_p = naive_uncomplete(abs_path, base);
#ifdef _MSC_VER
    return boost::locale::conv::utf_to_utf<char>(rel_p.native());
#else
    return rel_p.native();
#endif
}

std::string real_path(const std::string& pathname)
{
    boost::filesystem::path p(pathname);
    boost::system::error_code ec;

    boost::filesystem::path c_p = boost::filesystem::canonical(p);
#ifdef _MSC_VER
    return boost::locale::conv::utf_to_utf<char>(c_p.native());
#else
    return c_p.native();
#endif
}

std::string make_uuid()
{
    static boost::uuids::random_generator generator;
    return boost::lexical_cast<std::string>(generator());
}

std::string unique_temp_path(const std::string& prefix, const std::string& suffix)
{
    std::string tmpdir = boost::filesystem::temp_directory_path().generic_string();
    return pathjoin(tmpdir, prefix + make_uuid() + suffix);
}

bool write_file(const std::string& pathname, const std::string& contents)
{
    std::ofstream out_file;
    try {
        out_file.exceptions( std::ofstream::failbit | std::ofstream::badbit );
        out_file.open(pathname.c_str());
        out_file << contents;
        out_file.close();
    } catch (const std::ofstream::failure& exc) {
        return false;
    }
    return true;
}

bool rmrf(const std::string& pathname)
{
    try {
        boost::filesystem::remove_all(boost::filesystem::path(pathname));
    } catch (boost::filesystem::filesystem_error& exc) {
        return false;
    }
    return true;
}

double time_us()
{
    static boost::posix_time::ptime ms_t_start = boost::posix_time::microsec_clock::local_time();
    boost::posix_time::ptime ms_t_now = boost::posix_time::microsec_clock::local_time();
    boost::posix_time::time_duration ms_t_diff = ms_t_now - ms_t_start;

    return static_cast<double>(ms_t_diff.total_microseconds());
}

double time_ms()
{
    static boost::posix_time::ptime ms_t_start = boost::posix_time::microsec_clock::local_time();
    boost::posix_time::ptime ms_t_now = boost::posix_time::microsec_clock::local_time();
    boost::posix_time::time_duration ms_t_diff = ms_t_now - ms_t_start;

    return static_cast<double>(ms_t_diff.total_milliseconds());
}

double Profile::s_json_creation = 0.0;
double Profile::s_json_output = 0.0;
double Profile::s_disk_write = 0.0;
double Profile::s_git = 0.0;

std::ostream& operator<< ( std::ostream& out, const Profile& obj )
{
    out << "ELAPSED - json prep:" << obj.json_creation() << "us  " <<
        "json output:" << obj.json_output() << "us  " <<
        "disk write:" << obj.disk_write() << "us  " <<
        "git:" << obj.git() << "us";
    return out;
}

template <Alembic::Util::PlainOldDataType POD>
struct PodPrinter;

template <>
struct PodPrinter<Util::kBooleanPOD>
{
    static void Print(const std::string& msg, void *buffer) {
#if GLOBAL_TRACE_ENABLE
        Util::bool_t *ptr = (Util::bool_t*)buffer;
        TRACE(msg << " kBooleanPOD: " << *ptr);
#endif
    }
};

template <>
struct PodPrinter<Util::kUint8POD>
{
    static void Print(const std::string& msg, void *buffer) {
#if GLOBAL_TRACE_ENABLE
        Util::uint8_t *ptr = (Util::uint8_t*)buffer;
        TRACE(msg << " kUint8POD: " << ((int)(*ptr)));
#endif
    }
};

template <>
struct PodPrinter<Util::kInt8POD>
{
    static void Print(const std::string& msg, void *buffer) {
#if GLOBAL_TRACE_ENABLE
        Util::int8_t *ptr = (Util::int8_t*)buffer;
        TRACE(msg << " kInt8POD: " << ((int)(*ptr)));
#endif
    }
};

template <>
struct PodPrinter<Util::kUint16POD>
{
    static void Print(const std::string& msg, void *buffer) {
#if GLOBAL_TRACE_ENABLE
        Util::uint16_t *ptr = (Util::uint16_t*)buffer;
        TRACE(msg << " kUint16POD: " << *ptr);
#endif
    }
};

template <>
struct PodPrinter<Util::kInt16POD>
{
    static void Print(const std::string& msg, void *buffer) {
#if GLOBAL_TRACE_ENABLE
        Util::int16_t *ptr = (Util::int16_t*)buffer;
        TRACE(msg << " kInt16POD: " << *ptr);
#endif
    }
};

template <>
struct PodPrinter<Util::kUint32POD>
{
    static void Print(const std::string& msg, void *buffer) {
#if GLOBAL_TRACE_ENABLE
        Util::uint32_t *ptr = (Util::uint32_t*)buffer;
        TRACE(msg << " kUint32POD: " << *ptr);
#endif
    }
};

template <>
struct PodPrinter<Util::kInt32POD>
{
    static void Print(const std::string& msg, void *buffer) {
#if GLOBAL_TRACE_ENABLE
        Util::int32_t *ptr = (Util::int32_t*)buffer;
        TRACE(msg << " kInt32POD: " << *ptr);
#endif
    }
};

template <>
struct PodPrinter<Util::kUint64POD>
{
    static void Print(const std::string& msg, void *buffer) {
#if GLOBAL_TRACE_ENABLE
        Util::uint64_t *ptr = (Util::uint64_t*)buffer;
        TRACE(msg << " kUint64POD: " << *ptr);
#endif
    }
};

template <>
struct PodPrinter<Util::kInt64POD>
{
    static void Print(const std::string& msg, void *buffer) {
#if GLOBAL_TRACE_ENABLE
        Util::int64_t *ptr = (Util::int64_t*)buffer;
        TRACE(msg << " kInt64POD: " << *ptr);
#endif
    }
};

template <>
struct PodPrinter<Util::kFloat16POD>
{
    static void Print(const std::string& msg, void *buffer) {
#if GLOBAL_TRACE_ENABLE
        Util::float16_t *ptr = (Util::float16_t*)buffer;
        TRACE(msg << " kFloat16POD: " << *ptr);
#endif
    }
};

template <>
struct PodPrinter<Util::kFloat32POD>
{
    static void Print(const std::string& msg, void *buffer) {
#if GLOBAL_TRACE_ENABLE
        Util::float32_t *ptr = (Util::float32_t*)buffer;
        TRACE(msg << " kFloat32POD: " << *ptr);
#endif
    }
};

template <>
struct PodPrinter<Util::kFloat64POD>
{
    static void Print(const std::string& msg, void *buffer) {
#if GLOBAL_TRACE_ENABLE
        Util::float64_t *ptr = (Util::float64_t*)buffer;
        TRACE(msg << " kFloat64POD: " << *ptr);
#endif
    }
};

template <>
struct PodPrinter<Util::kStringPOD>
{
    static void Print(const std::string& msg, void *buffer) {
#if GLOBAL_TRACE_ENABLE
        Util::string *ptr = (Util::string*)buffer;
        Util::string& s = *ptr;
        TRACE(msg << " kStringPOD: '" << s << "' (size:" << s.size() << ")");
#endif
    }
};

template <>
struct PodPrinter<Util::kWstringPOD>
{
    static void Print(const std::string& msg, void *buffer) {
#if GLOBAL_TRACE_ENABLE
        Util::wstring *ptr = (Util::wstring*)buffer;
        Util::wstring& ws = *ptr;
        Util::string ws_s(ws.begin(), ws.end());
        TRACE(msg << " kWstringPOD: '" << ws_s << "' (wsize:" << ws.size() << "  size:" << ws_s.size() << ")");
#endif
    }
};


void printPodValue(const std::string& msg, Alembic::Util::PlainOldDataType pod, void* buffer)
{
    switch (pod)
    {
    case Util::kBooleanPOD:
        PodPrinter<Util::kBooleanPOD>::Print(msg, buffer);
        break;
    case Util::kUint8POD:
        PodPrinter<Util::kUint8POD>::Print(msg, buffer);
        break;
    case Util::kInt8POD:
        PodPrinter<Util::kInt8POD>::Print(msg, buffer);
        break;
    case Util::kUint16POD:
        PodPrinter<Util::kUint16POD>::Print(msg, buffer);
        break;
    case Util::kInt16POD:
        PodPrinter<Util::kInt16POD>::Print(msg, buffer);
        break;
    case Util::kUint32POD:
        PodPrinter<Util::kUint32POD>::Print(msg, buffer);
        break;
    case Util::kInt32POD:
        PodPrinter<Util::kInt32POD>::Print(msg, buffer);
        break;
    case Util::kUint64POD:
        PodPrinter<Util::kUint64POD>::Print(msg, buffer);
        break;
    case Util::kInt64POD:
        PodPrinter<Util::kInt64POD>::Print(msg, buffer);
        break;
    case Util::kFloat16POD:
        PodPrinter<Util::kFloat16POD>::Print(msg, buffer);
        break;
    case Util::kFloat32POD:
        PodPrinter<Util::kFloat32POD>::Print(msg, buffer);
        break;
    case Util::kFloat64POD:
        PodPrinter<Util::kFloat64POD>::Print(msg, buffer);
        break;
    case Util::kStringPOD:
        PodPrinter<Util::kStringPOD>::Print(msg, buffer);
        break;
    case Util::kWstringPOD:
        PodPrinter<Util::kWstringPOD>::Print(msg, buffer);
        break;
    default:
        TRACE("UNSUPPORTED POD " << PODName(pod));
        break;
    }
}

void printPodArray(const std::string& msg, Alembic::Util::PlainOldDataType pod, size_t count, void* buffer)
{
#if GLOBAL_TRACE_ENABLE
    for (size_t i = 0; i < count; i++)
    {
        std::ostringstream ss;
        ss << " [" << i << "]";
        char *ptr = ((char *)buffer) + (i * PODNumBytes(pod));
        printPodValue(msg + ss.str(), pod, (void*)ptr);
    }
#endif
}

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreGit
} // End namespace Alembic
