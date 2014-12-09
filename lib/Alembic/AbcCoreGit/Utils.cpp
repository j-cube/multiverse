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

#ifndef PATH_MAX
#define PATH_MAX 1023
#endif

char *xstrdup(const char *string)
{
  return strcpy((char *)malloc(strlen(string) + 1), string);
}

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
    struct stat info;

    if (stat(pathname.c_str(), &info) != 0)
    {
        if ((errno == ENOENT) || (errno == ENOTDIR))
            return false;       // does not exist

        // error
        return false;
    } else if (info.st_mode & S_IFDIR)  // S_ISDIR() doesn't exist on windows
    {
        // directory
        return true;
    } else
    {
        // file or something else
        return true;
    }

    return true;
}

bool isdir(const std::string& pathname)
{
    struct stat info;

    if (stat(pathname.c_str(), &info) != 0)
    {
        if ((errno == ENOENT) || (errno == ENOTDIR))
            return false;       // does not exist

        // error
        return false;
    } else if (info.st_mode & S_IFDIR)  // S_ISDIR() doesn't exist on windows
    {
        // directory
        return true;
    } else
    {
        // file or something else
        return false;
    }

    return false;
}

bool isfile(const std::string& pathname)
{
    struct stat info;

    if (stat(pathname.c_str(), &info) != 0)
    {
        if ((errno == ENOENT) || (errno == ENOTDIR))
            return false;       // does not exist

        // error
        return false;
    } else if (info.st_mode & S_IFDIR)  // S_ISDIR() doesn't exist on windows
    {
        // directory
        return false;
    } else
    {
        // file or something else
        return true;
    }

    return true;
}

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

std::string pathjoin(const std::string& p1, const std::string& p2, char pathsep)
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

bool is_dir_sep(char c, char pathsep)
{
    return (c == pathsep);
}

bool is_dir_sep(const std::string& s, char pathsep)
{
    if (s.length() <= 0)
        return false;
    return (s[0] == pathsep);
}

const char *relative_path(const char *abs, const char *base)
{
    static char buf[PATH_MAX + 1];
    int i = 0, j = 0;

    if (!base || !base[0])
        return abs;
    while (base[i]) {
        if (is_dir_sep(base[i])) {
            if (!is_dir_sep(abs[j]))
                return abs;
            while (is_dir_sep(base[i]))
                i++;
            while (is_dir_sep(abs[j]))
                j++;
            continue;
        } else if (abs[j] != base[i]) {
            return abs;
        }
        i++;
        j++;
    }
    if (
        /* "/foo" is a prefix of "/foo" */
        abs[j] &&
        /* "/foo" is not a prefix of "/foobar" */
        !is_dir_sep(base[i-1]) && !is_dir_sep(abs[j])
       )
        return abs;
    while (is_dir_sep(abs[j]))
        j++;
    if (!abs[j])
        strcpy(buf, ".");
    else
        strcpy(buf, abs + j);
    return buf;
}

std::string relative_path(const std::string& abs, const std::string& base)
{
    return relative_path(abs.c_str(), base.c_str());
}

/* We allow "recursive" symbolic links. Only within reason, though. */
#define MAXDEPTH 5

static inline char *find_last_dir_sep(const char *path)
{
    return strrchr((char *)path, '/');
}

/*
 * Use this to get the real path, i.e. resolve links. If you want an
 * absolute path but don't mind links, use absolute_path.
 *
 * If path is our buffer, then return path, as it's already what the
 * user wants.
 */
const char *real_path(const char *path)
{
    static char bufs[2][PATH_MAX + 1], *buf = bufs[0], *next_buf = bufs[1];
    char cwd[1024] = "";
    int buf_index = 1;

    int depth = MAXDEPTH;
    char *last_elem = NULL;
    struct stat st;

    /* We've already done it */
    if (path == buf || path == next_buf)
        return path;

    if (strlcpy(buf, path, PATH_MAX) >= PATH_MAX)
    {
        std::cerr << "pathname '" << path << "' too long" << std::endl;
        return NULL;
    }

    while (depth--) {
        if (!isdir(buf)) {
            char *last_slash = find_last_dir_sep(buf);
            if (last_slash) {
                *last_slash = '\0';
                last_elem = xstrdup(last_slash + 1);
            } else {
                last_elem = xstrdup(buf);
                *buf = '\0';
            }
        }

        if (*buf) {
            if (!*cwd && !getcwd(cwd, sizeof(cwd)))
            {
                std::cerr << "can't get current working directory" << std::endl;
                return NULL;
            }

            if (chdir(buf))
            {
                std::cerr << "can't set current working directory to '" << buf << "'" << std::endl;
                return NULL;
            }
        }
        if (!getcwd(buf, PATH_MAX))
        {
            std::cerr << "can't get current working directory" << std::endl;
            return NULL;
        }

        if (last_elem) {
            size_t len = strlen(buf);
            if (len + strlen(last_elem) + 2 > PATH_MAX)
            {
                std::cerr << "pathname '" << buf << "/" << last_elem << "' too long" << std::endl;
                return NULL;
            }
            if (len && !is_dir_sep(buf[len-1]))
                buf[len++] = '/';
            strcpy(buf + len, last_elem);
            free(last_elem);
            last_elem = NULL;
        }

        if (!lstat(buf, &st) && S_ISLNK(st.st_mode)) {
            ssize_t len = readlink(buf, next_buf, PATH_MAX);
            if (len < 0)
            {
                std::cerr << "invalid symlink '" << buf << "'" << std::endl;
                return NULL;
            }
            if (PATH_MAX <= len)
            {
                std::cerr << "symlink too long: '" << buf << "'" << std::endl;
                return NULL;
            }
            next_buf[len] = '\0';
            buf = next_buf;
            buf_index = 1 - buf_index;
            next_buf = bufs[buf_index];
        } else
            break;
    }

    if (*cwd && chdir(cwd))
    {
        std::cerr << "can't change current directory back to '" << cwd << "'" << std::endl;
        return NULL;
    }

    return buf;
}

std::string real_path(const std::string& path)
{
    std::string res = real_path(path.c_str());
    return res;
}

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreGit
} // End namespace Alembic
