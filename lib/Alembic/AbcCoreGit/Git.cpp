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

#include <Alembic/AbcCoreGit/Git.h>
#include <Alembic/AbcCoreGit/Utils.h>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <cassert>

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include <time.h>

#include <git2.h>
#include <git2/errors.h>
#include <git2/sys/repository.h>
#include <git2/sys/odb_backend.h>
#include <git2/sys/refdb_backend.h>
#include <git2/odb_backend.h>

#include <Alembic/AbcCoreGit/git-memcached.h>
#include <Alembic/AbcCoreGit/git-sqlite.h>
#include <Alembic/AbcCoreGit/git-milliways.h>

#include <Alembic/AbcCoreGit/JSON.h>
#include "Utils.h"

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {


/* --------------------------------------------------------------------
 *   MACROS
 * -------------------------------------------------------------------- */

// uncomment the following if libgit2 is < 0.22.x (i.e.: doesn't have git_libgit2_init(), ...)
// #define LIBGIT2_OLD

#ifndef GIT_SUCCESS
#define GIT_SUCCESS 0
#endif /* GIT_SUCCESS */

// define MILLIWAYS_ENABLED to enable milliways at run-time
#define MILLIWAYS_ENABLED

#define MILLIWAYS_SINGLE_FILE

#if !defined(MILLIWAYS_ENABLED)
    #undef MILLIWAYS_SINGLE_FILE
#endif

// define only one of these
// #define USE_SQLITE_BACKEND
// #define USE_MEMCACHED_BACKEND
//#define USE_MILLIWAYS_BACKEND

#define GIT_MEMCACHED_BACKEND_HOST "127.0.0.1"
#define GIT_MEMCACHED_BACKEND_PORT 11211

#if defined(MILLIWAYS_ENABLED)
    #undef USE_CUSTOM_BACKEND
    #undef USE_SQLITE_BACKEND
    #undef USE_MEMCACHED_BACKEND
    #undef USE_MILLIWAYS_BACKEND
#else
    #if defined(USE_SQLITE_BACKEND) && defined(USE_MEMCACHED_BACKEND)
    #error "define only one of USE_SQLITE_BACKEND or USE_MEMCACHED_BACKEND or USE_MILLIWAYS_BACKEND"
    #endif /* defined(USE_SQLITE_BACKEND) && defined(USE_MEMCACHED_BACKEND) */


    #if defined(USE_SQLITE_BACKEND) || defined(USE_MEMCACHED_BACKEND) || defined(USE_MILLIWAYS_BACKEND)
    #define USE_CUSTOM_BACKEND
    #endif /* defined(USE_SQLITE_BACKEND) || defined(USE_MEMCACHED_BACKEND) || defined(USE_MILLIWAYS_BACKEND) */
#endif

/* --------------------------------------------------------------------
 *   PROTOTYPES
 * -------------------------------------------------------------------- */

static bool git_check_error(int error_code, const std::string& action);
static bool git_check_ok(int error_code, const std::string& action);
static bool git_check_rc(int rc, const std::string& action, std::string& errorMessage);

#if 0
static std::string slurp(const std::string& pathname);
static std::string slurp(std::ifstream& in);
#endif /* 0 */

/* --------------------------------------------------------------------
 *   UTILITY FUNCTIONS
 * -------------------------------------------------------------------- */

#if 0
static std::string slurp(const std::string& pathname)
{
    std::ifstream ifs(pathname.c_str(), std::ios::in | std::ios::binary);
    return slurp(ifs);
}

static std::string slurp(std::ifstream& in)
{
    std::stringstream sstr;
    sstr << in.rdbuf();
    return sstr.str();
}
#endif /* 0 */

static void get_system_user_info(std::string & username,
                                 std::string & mail)
{
#ifdef _MSC_VER
    const char * username_env = getenv("USERNAME");
    const char * hostname_env = getenv("COMPUTERNAME");
#else
    const char * username_env = getenv("USER");
    const char * hostname_env = getenv("HOSTNAME");
#endif
    if (username_env)
        username = username_env;
    else
        username = "multiverse";

    if (hostname_env)
    {
        mail = username + "@";
        mail += hostname_env;
        mail += ".com";
    }
    else
        mail = username + "@localhost";
}

static bool git_check_error(int error_code, const std::string& action)
{
    if (error_code == GIT_SUCCESS)
       return false;

    const git_error *error = giterr_last();

    std::cerr << "ERROR: libgit2 error " << error_code << " - "
        << action << " - "
        << ((error && error->message) ? error->message : "???")
        << std::endl;
    ABCA_THROW( "libgit2 error " << error_code << " " << action <<
        " - " << ((error && error->message) ? error->message : "???") );
    // std::cerr << "ERROR: libgit2 error " << error_code << " - "
    //     << action << " - "
    //     << ((error && error->message) ? error->message : "???")
    //     << std::endl;
    return true;
}

static bool git_check_ok(int error_code, const std::string& action)
{
    if (error_code == GIT_SUCCESS)
       return true;

    const git_error *error = giterr_last();

    TRACE( "libgit2 error " << error_code << " " << action <<
        " - " << ((error && error->message) ? error->message : "???") );
    ABCA_THROW( "libgit2 error " << error_code << " " << action <<
        " - " << ((error && error->message) ? error->message : "???") );
    // std::cerr << "ERROR: libgit2 error " << error_code << " - "
    //     << action << " - "
    //     << ((error && error->message) ? error->message : "???")
    //     << std::endl;
    return false;
}

static bool git_check_rc(int rc, const std::string& action, std::string& errorMessage)
{
    if (rc == GIT_SUCCESS)
       return true;

    const git_error *error = giterr_last();

    std::ostringstream ss;
    ss << "ERROR: libgit2 error code:" << rc << " " <<
        action << " - " <<
        ((error && error->message) ? error->message : "(UNKNOWN)");
    errorMessage = ss.str();

    std::cerr << errorMessage << std::endl;

    return false;
}

std::string git_time_str(const git_time& intime)
{
    char sign, out[32];
    memset(out, '\0', 32);

    struct tm *intm;
    int offset, hours, minutes;
    time_t t;

    offset = intime.offset;
    if (offset < 0) {
        sign = '-';
        offset = -offset;
    } else {
        sign = '+';
    }

    hours   = offset / 60;
    minutes = offset % 60;

    t = (time_t)intime.time + (intime.offset * 60);

    intm = gmtime(&t);
#ifdef _MSC_VER
    const char *formatstr = "%a %b %d %H:%M:%S %Y";
#else
    const char *formatstr = "%a %b %e %T %Y";
#endif
    strftime(out, sizeof(out) - 1, formatstr, intm);

    std::ostringstream ss;
    ss << out << " " << sign <<
        std::setfill('0') << std::setw(2) << hours <<
                             std::setw(2) << minutes;
    return ss.str();
}


/* --------------------------------------------------------------------
 *   CLASSES
 * -------------------------------------------------------------------- */

/* --- LibGit --------------------------------------------------------- */

// based on Monoid pattern of Alexandrescu's "Modern C++ Design"
// see:
//   http://stackoverflow.com/questions/2496918/singleton-pattern-in-c
//   http://stackoverflow.com/a/2498423/1363486

Alembic::Util::shared_ptr<LibGit> LibGit::m_instance = Alembic::Util::shared_ptr<LibGit>(static_cast<LibGit*>(NULL));

LibGit::LibGit()
{
#ifdef LIBGIT2_OLD
    git_threads_init();
#else
    git_libgit2_init();
#endif
}

LibGit::~LibGit()
{
#ifdef LIBGIT2_OLD
    git_threads_shutdown();
#else
    git_libgit2_shutdown();
#endif
}

Alembic::Util::shared_ptr<LibGit> LibGit::Instance()
{
    if (m_instance && m_instance.get())
        return m_instance;

    m_instance.reset(new LibGit());
    atexit(&CleanUp);
    return m_instance;
}

bool LibGit::IsInitialized()
{
    return (m_instance && m_instance.get());
}

bool LibGit::Initialize()
{
    if (! IsInitialized())
    {
        Alembic::Util::shared_ptr<LibGit> ptr = Instance();
    }
    return IsInitialized();
}

void LibGit::CleanUp()
{
    m_instance.reset();
}


/* --- GitMode -------------------------------------------------------- */

std::ostream& operator<< (std::ostream& out, const GitMode& value)
{
    switch (GitMode::Type(value))
    {
    case GitMode::Read:
        out << "R";
        break;
    case GitMode::Write:
        out << "W";
        break;
    case GitMode::ReadWrite:
        out << "R/W";
        break;
    default:
        out << "UNKNOWN";
        break;
    }
    return out;
}


/* --- GitCommitInfo -------------------------------------------------- */

GitCommitInfo::GitCommitInfo(const git_oid& oid, const git_commit* commit)
{
    assert(commit);

    char oidstr[GIT_OID_HEXSZ + 1];

    // git_oid_tostr(oidstr, sizeof(oidstr), git_commit_id(commit));
    git_oid_tostr(oidstr, sizeof(oidstr), &oid);

    const git_signature *sig = git_commit_author(commit);
    assert(sig);

    id           = oidstr;
    author       = sig->name;
    author_email = sig->email;
    when_time    = sig->when;
    when         = git_time_str(sig->when);
    message      = git_commit_message(commit);
}

std::ostream& operator<< (std::ostream& out, const GitCommitInfo& cinfo)
{
    out << "<Commit " << cinfo.id << " author:'" << cinfo.author << "' date:" << cinfo.when << " message:'" << cinfo.message << "'>";
    return out;
}


/* --- GitRepo -------------------------------------------------------- */

#if 0
GitRepo::GitRepo(git_repository* repo, git_config* config, git_signature* signature) :
    m_repo(repo), m_mode(GitMode::ReadWrite), m_cfg(config), m_sig(signature),
    m_odb(NULL), m_refdb(NULL), m_index(NULL), m_error(false)
{
    LibGit::Initialize();
}
#endif

GitRepo::GitRepo(const std::string& pathname_, const Alembic::AbcCoreFactory::IOptions& options, GitMode mode_, bool milliwaysEnable) :
    m_pathname(pathname_), m_mode(mode_), m_repo(NULL), m_cfg(NULL), m_sig(NULL),
    m_odb(NULL), m_refdb(NULL), m_index(NULL), m_git_backend(NULL), m_error(false),
    m_index_dirty(false),
    m_options(options), m_ignore_wrong_rev(DEFAULT_IGNORE_WRONG_REV),
    m_milliways_enabled(milliwaysEnable), m_support_repo(),
    m_cleaned_up(false)
{
    TRACE("GitRepo::GitRepo(pathname:'" << pathname_ << "' mode:" << mode_ << ")");

    LibGit::Initialize();

    bool ok = true;
    int rc;

    if (m_options.has("revision"))
    {
        m_revision = boost::any_cast<std::string>(m_options["revision"]);
        TRACE("GitRepo has revision '" << m_revision << "' specified.");
    }

    if (m_options.has("ignoreNonexistentRevision"))
    {
        m_ignore_wrong_rev = boost::any_cast<bool>(m_options["ignoreNonexistentRevision"]);
        TRACE("ignoreNonexistentRevision specified: " << m_ignore_wrong_rev);
    }

    if (m_options.has("milliways"))
    {
        m_milliways_enabled = boost::any_cast<bool>(m_options["milliways"]);
        TRACE("milliways enable/disable flag specified: " << (m_milliways_enabled ? "enabled" : "disabled"));
    }

#ifdef MILLIWAYS_SINGLE_FILE
    bool milliwaysSingleFile = m_milliways_enabled;

    if (isfile(m_pathname)) {
        m_milliways_enabled = true;
        milliwaysSingleFile = true;
    }
#else
    bool milliwaysSingleFile = false;
#endif

    TRACE("milliways            : " << (m_milliways_enabled ? "enabled" : "disabled"));
    TRACE("milliways single file: " << (milliwaysSingleFile ? "enabled" : "disabled"));

    // if passed the path to the "archive.json.abc" file, use the containing directory instead
    if (! milliwaysSingleFile) {
        if (isfile(m_pathname))
        {
            TRACE("file specified ('" << m_pathname << "'), switching to parent directory: '" << dirname(m_pathname) << "'");
            m_pathname = dirname(m_pathname);
        }
    }

    std::string repoPathname;
    std::string dotGitPathname;
    std::string sqlitePathname;
    std::string milliwaysPathname;

    if (milliwaysSingleFile) {
        m_support_repo = unique_temp_path("multiverse-", ".mw.git");
        repoPathname = m_support_repo;
        dotGitPathname = pathjoin(repoPathname, ".git");
        sqlitePathname = pathjoin(repoPathname, "store.db");
        milliwaysPathname = m_pathname;
    } else {
        repoPathname = pathname();
        dotGitPathname = pathjoin(repoPathname, ".git");
        sqlitePathname = pathjoin(repoPathname, "store.db");
        milliwaysPathname = pathjoin(repoPathname, "store.mwdb");
    }

    if (milliwaysSingleFile) {
        bool createDummyRepo = false;

        if (file_exists(milliwaysPathname))
            createDummyRepo = true;

        if (createDummyRepo)
        {
            TRACE("creating support repo structure '" << dotGitPathname << "'");
            ok = mkpath(dotGitPathname, 0777) &&
                mkpath(pathjoin(dotGitPathname, "refs"), 0777) &&
                mkpath(pathjoin(pathjoin(dotGitPathname, "refs"), "tags"), 0777) &&
                mkpath(pathjoin(pathjoin(dotGitPathname, "refs"), "heads"), 0777) &&
                mkpath(pathjoin(dotGitPathname, "info"), 0777) &&
                mkpath(pathjoin(dotGitPathname, "hooks"), 0777) &&
                mkpath(pathjoin(dotGitPathname, "objects"), 0777) &&
                mkpath(pathjoin(pathjoin(dotGitPathname, "objects"), "info"), 0777) &&
                mkpath(pathjoin(pathjoin(dotGitPathname, "objects"), "pack"), 0777);
            if (! ok) {
                std::cerr << "ERROR: can't recreate support repo at path " << dotGitPathname << std::endl;
                ABCA_THROW( "ERROR: can't recreate support repo at path " << dotGitPathname );
                goto ret;
            }
            write_file(pathjoin(dotGitPathname, "config").c_str(),
                        "[core]\n"
                        "\tbare = false\n"
                        "\tfilemode = true\n"
                        "\tlogallrefupdates = true\n"
                        "\tignorecase = true\n"
                        "[core]\n"
                        "\trepositoryformatversion = 0\n"
                        "[alembic]\n"
                        "\tformatversion = 1\n"
                        "[alembic]\n"
                        "\tlibversion = 10700\n");
            write_file(pathjoin(dotGitPathname, "description").c_str(),
                        "Unnamed repository; edit this file 'description' to name the repository.\n");
            write_file(pathjoin(pathjoin(dotGitPathname, "info"), "exclude").c_str(),
                        "# File patterns to ignore; see `git help ignore` for more information.\n"
                        "# Lines that start with '#' are comments.\n");
            write_file(pathjoin(dotGitPathname, "HEAD").c_str(),
                        "ref: refs/heads/master\n");
            TRACE("DONE");
        }
    }

    if ((m_mode == GitMode::Write) || (m_mode == GitMode::ReadWrite))
    {
        if (! isdir(repoPathname))
        {
            TRACE("initializing Git repository '" << repoPathname << "'");
            rc = git_repository_init(&m_repo, repoPathname.c_str(), /* is_bare */ 0);
            ok = ok && git_check_ok(rc, "initializing git repository");
            if (! ok)
            {
                // ABCA_THROW( "Could not open (initialize) file: " << repoPathname << " (git repo)");
                goto ret;
            }

            assert(m_repo);

            git_repository_config(&m_cfg, m_repo /* , NULL, NULL */);

            git_config_set_int32(m_cfg, "core.repositoryformatversion", 0);
            git_config_set_bool(m_cfg, "core.filemode", 1);
            git_config_set_bool(m_cfg, "core.bare", 0);
            git_config_set_bool(m_cfg, "core.logallrefupdates", 1);
            git_config_set_bool(m_cfg, "core.ignorecase", 1);

            // set the version of the Alembic git backend
            // This expresses the AbcCoreGit version - how properties, are stored within Git, etc.
            git_config_set_int32(m_cfg, "alembic.formatversion", ALEMBIC_GIT_FILE_VERSION);
            // This is the Alembic library version XXYYZZ
            // Where XX is the major version, YY is the minor version
            // and ZZ is the patch version
            git_config_set_int32(m_cfg, "alembic.libversion", ALEMBIC_LIBRARY_VERSION);

            //git_repository_set_workdir(repo, m_fileName);

            git_repository_free(m_repo);
            m_repo = NULL;
        } else
        {
            TRACE("Git repository '" << repoPathname << "' exists, opening (for writing)");
            if (file_exists(milliwaysPathname))
            {
                TRACE("milliways repository detected, force-enabling milliways");
                m_milliways_enabled = true;
            }
        }
    } else
    {
        if (isdir(repoPathname))
        {
            TRACE("Git repository '" << repoPathname << "' exists, opening (for reading)");
            if (file_exists(milliwaysPathname))
            {
                TRACE("milliways repository detected, force-enabling milliways");
                m_milliways_enabled = true;
            }
        }
    }

    assert(! m_repo);

    rc = git_repository_open(&m_repo, dotGitPathname.c_str());
    ok = ok && git_check_ok(rc, "opening git repository");
    if (! ok)
    {
        TRACE("error opening repository from '" << dotGitPathname << "'");
        //ABCA_THROW( "Could not open file: " << repoPathname << " (git repo)");
        goto ret;
    }

    assert(m_repo);

    rc = git_repository_config(&m_cfg, m_repo /* , NULL, NULL */);
    ok = ok && git_check_ok(rc, "opening git config");
    if (!ok)
    {
        TRACE("error fetching config for repository '" << dotGitPathname << "'");
        //ABCA_THROW( "Could not get Git configuration for repository: " << repoPathname);
        goto ret;
    }
    assert(m_cfg);

    rc = git_signature_default(&m_sig, m_repo);
    if (rc != GIT_SUCCESS)
    {
        std::string username, mail;
        get_system_user_info(username, mail);

        rc = git_signature_now(&m_sig, username.c_str(), mail.c_str());
    }
    ok = ok && git_check_ok(rc, "getting default signature");
    if (! ok) goto ret;

    assert(m_sig);

#ifdef MILLIWAYS_ENABLED
    if (milliwaysEnabled())
    {
        TRACE("milliways enabled, setting custom git odb backend");

        assert(! m_odb);

        rc = git_odb_new(&m_odb);
        ok = ok && git_check_ok(rc, "creating new odb without backends");
        if (!ok) goto ret;

        assert(m_odb);

        TRACE("call git_odb_backend_milliways()");
        rc = git_odb_backend_milliways(&m_git_backend, milliwaysPathname.c_str());
        ok = ok && git_check_ok(rc, "connecting to milliways ODB backend");
        if (!ok) goto ret;

        TRACE("call git_odb_add_backend()");
        rc = git_odb_add_backend(m_odb, m_git_backend, /* priority */ 1);
        ok = ok && git_check_ok(rc, "add custom backend to object database");
        if (!ok) goto ret;

        TRACE("call git_repository_set_odb()");
        git_repository_set_odb(m_repo, m_odb);

        /* refdb */
        assert(! m_refdb);
        rc = git_refdb_new(&m_refdb, m_repo);
        ok = ok && git_check_ok(rc, "creating new refdb");
        if (!ok) goto ret;

        assert(m_refdb);

        git_refdb_backend *milliways_refdb_backend = NULL;

        TRACE("call git_refdb_backend_milliways()");
        rc = git_refdb_backend_milliways(&milliways_refdb_backend, milliwaysPathname.c_str());
        ok = ok && git_check_ok(rc, "connecting to milliways refdb backend");
        if (!ok) goto ret;

        assert(milliways_refdb_backend);

        TRACE("call git_refdb_set_backend()");
        rc = git_refdb_set_backend(m_refdb, milliways_refdb_backend);
        ok = ok && git_check_ok(rc, "add custom milliways backed to refdb");
        if (!ok) goto ret;

        TRACE("call git_repository_set_refdb()");
        git_repository_set_refdb(m_repo, m_refdb);
    }
#endif

#ifdef USE_CUSTOM_BACKEND

    rc = git_odb_new(&m_odb);
    ok = ok && git_check_ok(rc, "creating new odb without backends");
    if (!ok) goto ret;

    assert(m_odb);

#ifdef USE_SQLITE_BACKEND
    rc = git_odb_backend_sqlite(&m_git_backend, sqlitePathname.c_str());
    ok = ok && git_check_ok(rc, "connecting to sqlite backend");
#endif

#ifdef USE_MEMCACHED_BACKEND
    rc = git_odb_backend_memcached(&m_git_backend, GIT_MEMCACHED_BACKEND_HOST, GIT_MEMCACHED_BACKEND_PORT);
    ok = ok && git_check_ok(rc, "connecting to memcached backend");
#endif

#ifdef USE_MILLIWAYS_BACKEND
    rc = git_odb_backend_milliways(&m_git_backend, milliwaysPathname.c_str());
    ok = ok && git_check_ok(rc, "connecting to milliways backend");
#endif

    if (!ok) goto ret;

    rc = git_odb_add_backend(m_odb, m_git_backend, /* priority */ 1);
    ok = ok && git_check_ok(rc, "add custom backend to object database");
    if (!ok) goto ret;

    git_repository_set_odb(m_repo, m_odb);

#else /* start !USE_CUSTOM_BACKEND */

    rc = git_repository_odb(&m_odb, m_repo);
    ok = ok && git_check_ok(rc, "accessing git repository database");
    if (!ok) goto ret;

#endif /* end !USE_CUSTOM_BACKEND */

    rc = git_repository_index(&m_index, m_repo);
    ok = ok && git_check_ok(rc, "opening repository index");
    if (!ok)
    {
        goto ret;
    }

ret:
    m_error = m_error || (!ok);
    if (m_error)
    {
        if (m_sig)
            git_signature_free(m_sig);
        m_sig = NULL;
        if (m_index)
            git_index_free(m_index);
        m_index = NULL;
        if (m_refdb)
            git_refdb_free(m_refdb);
        m_refdb = NULL;
        if (m_odb)
            git_odb_free(m_odb);
        m_odb = NULL;

#ifdef MILLIWAYS_ENABLED
    if (milliwaysEnabled())
    {
        TRACE("milliways enabled, cleaning up custom git odb backend");

        // if (m_git_backend)
        //     milliways_backend__free(m_git_backend);

        // m_git_backend = NULL;
    }
#endif

#ifdef USE_CUSTOM_BACKEND

#ifdef USE_SQLITE_BACKEND
        if (m_git_backend)
            sqlite_backend__free(m_git_backend);
#endif
#ifdef USE_MEMCACHED_BACKEND
        if (m_git_backend)
            memcached_backend__free(m_git_backend);
#endif
#ifdef USE_MILLIWAYS_BACKEND
        // if (m_git_backend)
        //     milliways_backend__free(m_git_backend);
#endif
        // m_git_backend = NULL;

#endif /* end USE_CUSTOM_BACKEND */

        if (m_cfg)
            git_config_free(m_cfg);
        m_cfg = NULL;
        if (m_repo)
            git_repository_free(m_repo);
        m_repo = NULL;
        m_git_backend = NULL;
    }
}

GitRepo::GitRepo(const std::string& pathname_, GitMode mode_, bool milliwaysEnable) :
    m_pathname(pathname_), m_mode(mode_), m_repo(NULL), m_cfg(NULL), m_sig(NULL),
    m_odb(NULL), m_refdb(NULL), m_index(NULL), m_error(false),
    m_index_dirty(false), m_ignore_wrong_rev(DEFAULT_IGNORE_WRONG_REV),
    m_milliways_enabled(milliwaysEnable), m_support_repo(),
    m_cleaned_up(false)
{
    TRACE("GitRepo::GitRepo(pathname:'" << pathname_ << "' mode:" << mode_ << ")");

    LibGit::Initialize();

    bool ok = true;
    int rc;

#if defined(MILLIWAYS_SINGLE_FILE)
    bool milliwaysSingleFile = m_milliways_enabled;
#else
    bool milliwaysSingleFile = false;
#endif

    std::string repoPathname;
    std::string dotGitPathname;
    std::string sqlitePathname;
    std::string milliwaysPathname;

    if (milliwaysSingleFile) {
        m_support_repo = unique_temp_path("multiverse-", ".mw.git");
        repoPathname = m_support_repo;
        dotGitPathname = pathjoin(repoPathname, ".git");
        sqlitePathname = pathjoin(repoPathname, "store.db");
        milliwaysPathname = m_pathname;
    } else {
        repoPathname = pathname();
        dotGitPathname = pathjoin(repoPathname, ".git");
        sqlitePathname = pathjoin(repoPathname, "store.db");
        milliwaysPathname = pathjoin(repoPathname, "store.mwdb");
    }

    if (milliwaysSingleFile) {
        bool createDummyRepo = false;

        if (file_exists(milliwaysPathname))
            createDummyRepo = true;

        if (createDummyRepo)
        {
            TRACE("creating support repo structure '" << dotGitPathname << "'");
            ok = mkpath(dotGitPathname, 0777) &&
                mkpath(pathjoin(dotGitPathname, "refs"), 0777) &&
                mkpath(pathjoin(pathjoin(dotGitPathname, "refs"), "tags"), 0777) &&
                mkpath(pathjoin(pathjoin(dotGitPathname, "refs"), "heads"), 0777) &&
                mkpath(pathjoin(dotGitPathname, "info"), 0777) &&
                mkpath(pathjoin(dotGitPathname, "hooks"), 0777) &&
                mkpath(pathjoin(dotGitPathname, "objects"), 0777) &&
                mkpath(pathjoin(pathjoin(dotGitPathname, "objects"), "info"), 0777) &&
                mkpath(pathjoin(pathjoin(dotGitPathname, "objects"), "pack"), 0777);
            if (! ok) {
                std::cerr << "ERROR: can't recreate support repo at path " << dotGitPathname << std::endl;
                ABCA_THROW( "ERROR: can't recreate support repo at path " << dotGitPathname );
                goto ret;
            }
            write_file(pathjoin(dotGitPathname, "config").c_str(),
                        "[core]\n"
                        "\tbare = false\n"
                        "\tfilemode = true\n"
                        "\tlogallrefupdates = true\n"
                        "\tignorecase = true\n"
                        "[core]\n"
                        "\trepositoryformatversion = 0\n"
                        "[alembic]\n"
                        "\tformatversion = 1\n"
                        "[alembic]\n"
                        "\tlibversion = 10700\n");
            write_file(pathjoin(dotGitPathname, "description").c_str(),
                        "Unnamed repository; edit this file 'description' to name the repository.\n");
            write_file(pathjoin(pathjoin(dotGitPathname, "info"), "exclude").c_str(),
                        "# File patterns to ignore; see `git help ignore` for more information.\n"
                        "# Lines that start with '#' are comments.\n");
            write_file(pathjoin(dotGitPathname, "HEAD").c_str(),
                        "ref: refs/heads/master\n");
            TRACE("DONE");
        }
    }

    if ((m_mode == GitMode::Write) || (m_mode == GitMode::ReadWrite))
    {
        if (! isdir(repoPathname))
        {
            TRACE("initializing Git repository '" << repoPathname << "'");
            rc = git_repository_init(&m_repo, repoPathname.c_str(), /* is_bare */ 0);
            ok = ok && git_check_ok(rc, "initializing git repository");
            if (! ok)
            {
                // ABCA_THROW( "Could not open (initialize) file: " << repoPathname << " (git repo)");
                goto ret;
            }

            assert(m_repo);

            git_repository_config(&m_cfg, m_repo /* , NULL, NULL */);

            git_config_set_int32(m_cfg, "core.repositoryformatversion", 0);
            git_config_set_bool(m_cfg, "core.filemode", 1);
            git_config_set_bool(m_cfg, "core.bare", 0);
            git_config_set_bool(m_cfg, "core.logallrefupdates", 1);
            git_config_set_bool(m_cfg, "core.ignorecase", 1);

            // set the version of the Alembic git backend
            // This expresses the AbcCoreGit version - how properties, are stored within Git, etc.
            git_config_set_int32(m_cfg, "alembic.formatversion", ALEMBIC_GIT_FILE_VERSION);
            // This is the Alembic library version XXYYZZ
            // Where XX is the major version, YY is the minor version
            // and ZZ is the patch version
            git_config_set_int32(m_cfg, "alembic.libversion", ALEMBIC_LIBRARY_VERSION);

            //git_repository_set_workdir(repo, m_fileName);

            git_repository_free(m_repo);
            m_repo = NULL;
        } else
        {
            TRACE("Git repository '" << repoPathname << "' exists, opening (for writing)");
        }
    }

    assert(! m_repo);

    rc = git_repository_open(&m_repo, dotGitPathname.c_str());
    ok = ok && git_check_ok(rc, "opening git repository");
    if (!ok)
    {
        TRACE("error opening repository from '" << dotGitPathname << "'");
        //ABCA_THROW( "Could not open file: " << repoPathname << " (git repo)");
        goto ret;
    }

    assert(m_repo);

    rc = git_repository_config(&m_cfg, m_repo /* , NULL, NULL */);
    ok = ok && git_check_ok(rc, "opening git config");
    if (!ok)
    {
        TRACE("error fetching config for repository '" << dotGitPathname << "'");
        //ABCA_THROW( "Could not get Git configuration for repository: " << repoPathname);
        goto ret;
    }
    assert(m_cfg);

    rc = git_signature_default(&m_sig, m_repo);
    if (rc != GIT_SUCCESS)
    {
        std::string username, mail;
        get_system_user_info(username, mail);

        rc = git_signature_now(&m_sig, username.c_str(), mail.c_str());
    }
    ok = ok && git_check_ok(rc, "getting default signature");
    if (! ok) goto ret;

    assert(m_sig);

#ifdef MILLIWAYS_ENABLED
    if (milliwaysEnabled())
    {
        TRACE("milliways enabled, setting custom git odb backend");

        rc = git_odb_new(&m_odb);
        ok = ok && git_check_ok(rc, "creating new odb without backends");
        if (!ok) goto ret;

        assert(m_odb);

        TRACE("call git_odb_backend_milliways()");
        rc = git_odb_backend_milliways(&m_git_backend, milliwaysPathname.c_str());
        ok = ok && git_check_ok(rc, "connecting to milliways backend");
        if (!ok) goto ret;

        TRACE("call git_odb_add_backend()");
        rc = git_odb_add_backend(m_odb, m_git_backend, /* priority */ 1);
        ok = ok && git_check_ok(rc, "add custom backend to object database");
        if (!ok) goto ret;

        TRACE("call git_repository_set_odb()");
        git_repository_set_odb(m_repo, m_odb);

        /* refdb */
        assert(! m_refdb);
        rc = git_refdb_new(&m_refdb, m_repo);
        ok = ok && git_check_ok(rc, "creating new refdb");
        if (!ok) goto ret;

        assert(m_refdb);

        git_refdb_backend *milliways_refdb_backend = NULL;

        TRACE("call git_refdb_backend_milliways()");
        rc = git_refdb_backend_milliways(&milliways_refdb_backend, milliwaysPathname.c_str());
        ok = ok && git_check_ok(rc, "connecting to milliways refdb backend");
        if (!ok) goto ret;

        assert(milliways_refdb_backend);

        TRACE("call git_refdb_set_backend()");
        rc = git_refdb_set_backend(m_refdb, milliways_refdb_backend);
        ok = ok && git_check_ok(rc, "add custom milliways backed to refdb");
        if (!ok) goto ret;

        TRACE("call git_repository_set_refdb()");
        git_repository_set_refdb(m_repo, m_refdb);
    }
#endif

#ifdef USE_CUSTOM_BACKEND

    rc = git_odb_new(&m_odb);
    ok = ok && git_check_ok(rc, "creating new odb without backends");
    if (!ok) goto ret;

    assert(m_odb);

#ifdef USE_SQLITE_BACKEND
    rc = git_odb_backend_sqlite(&m_git_backend, sqlitePathname.c_str());
    ok = ok && git_check_ok(rc, "connecting to sqlite backend");
#endif

#ifdef USE_MEMCACHED_BACKEND
    rc = git_odb_backend_memcached(&m_git_backend, GIT_MEMCACHED_BACKEND_HOST, GIT_MEMCACHED_BACKEND_PORT);
    ok = ok && git_check_ok(rc, "connecting to memcached backend");
#endif

#ifdef USE_MILLIWAYS_BACKEND
    rc = git_odb_backend_milliways(&m_git_backend, milliwaysPathname.c_str());
    ok = ok && git_check_ok(rc, "connecting to milliways backend");
#endif

    if (!ok) goto ret;

    rc = git_odb_add_backend(m_odb, m_git_backend, /* priority */ 1);
    ok = ok && git_check_ok(rc, "add custom backend to object database");
    if (!ok) goto ret;

    git_repository_set_odb(m_repo, m_odb);

#else /* start !USE_CUSTOM_BACKEND */

    rc = git_repository_odb(&m_odb, m_repo);
    ok = ok && git_check_ok(rc, "accessing git repository database");
    if (!ok) goto ret;

#endif /* end !USE_CUSTOM_BACKEND */

    rc = git_repository_index(&m_index, m_repo);
    ok = ok && git_check_ok(rc, "opening repository index");
    if (!ok)
    {
        goto ret;
    }

ret:
    m_error = m_error || (!ok);
    if (m_error)
    {
        if (m_sig)
            git_signature_free(m_sig);
        m_sig = NULL;
        if (m_index)
            git_index_free(m_index);
        m_index = NULL;
        if (m_refdb)
            git_refdb_free(m_refdb);
        m_refdb = NULL;
        if (m_odb)
            git_odb_free(m_odb);
        m_odb = NULL;

#ifdef MILLIWAYS_ENABLED
    if (milliwaysEnabled())
    {
        TRACE("milliways enabled, cleaning up custom git odb backend");

        // if (m_git_backend)
        //     milliways_backend__free(m_git_backend);

        // m_git_backend = NULL;
    }
#endif

#ifdef USE_CUSTOM_BACKEND

#ifdef USE_SQLITE_BACKEND
        if (m_git_backend)
            sqlite_backend__free(m_git_backend);
#endif
#ifdef USE_MEMCACHED_BACKEND
        if (m_git_backend)
            memcached_backend__free(m_git_backend);
#endif
#ifdef USE_MILLIWAYS_BACKEND
        // if (m_git_backend)
        //     milliways_backend__free(m_git_backend);
#endif
        // m_git_backend = NULL;

#endif /* end USE_CUSTOM_BACKEND */

        if (m_cfg)
            git_config_free(m_cfg);
        m_cfg = NULL;
        if (m_repo)
            git_repository_free(m_repo);
        m_repo = NULL;
        m_git_backend = NULL;
    }
}

GitRepo::~GitRepo()
{
    std::cerr << "GitRepo::DESTRUCT\n";
    if (! m_cleaned_up)
        cleanup();
}

void GitRepo::cleanup()
{
    if (m_cleaned_up)
        return;

    m_cleaned_up = true;

    TRACE("GitRepo::cleanup()");

    if (m_index)
        git_index_free(m_index);
    m_index = NULL;
    if (m_refdb)
    {
        TRACE("calling git_refdb_free()");
        git_refdb_free(m_refdb);
    }
    m_odb = NULL;
    if (m_odb)
    {
        TRACE("calling git_odb_free()");
        git_odb_free(m_odb);
    }
    m_odb = NULL;

#ifdef MILLIWAYS_ENABLED
    if (milliwaysEnabled())
    {
        TRACE("milliways enabled, cleaning up custom git odb backend");

        if (m_git_backend && (! milliways_backend__cleanedup(m_git_backend)))
            milliways_backend__free(m_git_backend);

        m_git_backend = NULL;
    }
#endif

#ifdef USE_CUSTOM_BACKEND

#ifdef USE_SQLITE_BACKEND
    if (m_git_backend)
        sqlite_backend__free(m_git_backend);
#endif
#ifdef USE_MEMCACHED_BACKEND
    if (m_git_backend)
        memcached_backend__free(m_git_backend);
#endif
#ifdef USE_MILLIWAYS_BACKEND
    // if (m_git_backend)
    //     milliways_backend__free(m_git_backend);
#endif
    // m_git_backend = NULL;

#endif /* end USE_CUSTOM_BACKEND */

    if (m_sig)
        git_signature_free(m_sig);
    m_sig = NULL;
    if (m_cfg)
        git_config_free(m_cfg);
    m_cfg = NULL;
    if (m_repo)
        git_repository_free(m_repo);
    m_repo = NULL;

    m_git_backend = NULL;

#if defined(MILLIWAYS_SINGLE_FILE)
    bool milliwaysSingleFile = m_milliways_enabled;
#else
    bool milliwaysSingleFile = false;
#endif

    if (milliwaysSingleFile && (! m_support_repo.empty()) && file_exists(m_support_repo)) {
        rmrf(m_support_repo);
        m_support_repo = "";
    }
}

// WARNING: be sure to have an existing shared_ptr to the repo
// before calling the following method!
// (Because it uses shared_from_this()...)
GitTreebuilderPtr GitRepo::root()
{
    if (m_root && m_root.get())
        return m_root;

    // GitRepoPtr thisPtr(this);

    m_root = GitTreebuilder::Create(ptr());
    m_root->_set_filename("/");
    return m_root;
}

// WARNING: be sure to have an existing shared_ptr to the repo
// before calling the following method!
// (Because it uses shared_from_this()...)
GitTreePtr GitRepo::rootTree()
{
    if (m_root_tree && m_root_tree.get())
        return m_root_tree;

    m_root_tree = GitTree::Create(ptr());
    return m_root_tree;
}

int32_t GitRepo::formatVersion() const
{
    int rc;

    int32_t formatversion = -1;

    rc = git_config_get_int32(&formatversion, g_config(), "alembic.formatversion");
    if (rc != GIT_SUCCESS)
    {
        formatversion = ALEMBIC_GIT_FILE_VERSION;
        std::cerr << "WARNING: can't get alembic.formatversion config value, still try to open it" << std::endl;
    }

    return formatversion;
}

int32_t GitRepo::libVersion() const
{
    int rc;

    int32_t libversion = -1;

    rc = git_config_get_int32(&libversion, g_config(), "alembic.libversion");
    if (rc != GIT_SUCCESS)
    {
        libversion = ALEMBIC_LIBRARY_VERSION;
        std::cerr << "WARNING: can't get alembic.libversion config value, still try to open it" << std::endl;
    }

    return libversion;
}

/* index based API */

bool GitRepo::add(const std::string& path)
{
    ABCA_ASSERT( m_repo, "libgit2 repository must be open" );
    ABCA_ASSERT( m_index, "libgit2 index must be open" );

    TRACE("adding '" << path << "' to git index");
    int rc = git_index_add_bypath(m_index, path.c_str());
    if (! git_check_ok(rc, "adding file to git index"))
        return false;
    m_index_dirty = true;

    return true;
}

bool GitRepo::write_index()
{
    ABCA_ASSERT( m_repo, "libgit2 repository must be open" );
    ABCA_ASSERT( m_index, "libgit2 index must be open" );

    if (! m_index_dirty)
    {
        TRACE("git index not changed, skipping write...");
        return true;
    }

    TRACE("writing git index");
    int rc = git_index_write(m_index);
    if (! git_check_ok(rc, "writing git index"))
        return false;
    m_index_dirty = false;

    return true;
}

bool GitRepo::commit_index(const std::string& message) const
{
    git_signature *sig;
    git_oid tree_id, commit_id;
    git_tree *tree;

    git_oid head_oid;
    git_commit *head_commit = NULL;

    int rc;

    ABCA_ASSERT( m_repo, "libgit2 repository must be open" );
    ABCA_ASSERT( m_index, "libgit2 index must be open" );

    /** First use the config to initialize a commit signature for the user. */
    rc = git_signature_default(&sig, m_repo);
    if (rc != GIT_SUCCESS)
    {
        std::string username, mail;
        get_system_user_info(username, mail);

        rc = git_signature_now(&sig, username.c_str(), mail.c_str());
    }
    if (! git_check_ok(rc, "obtaining commit signature. Perhaps 'user.name' and 'user.email' are not set."))
        return false;

    /* Now let's create an empty tree for this commit */

    /**
     * Outside of this example, you could call git_index_add_bypath()
     * here to put actual files into the index.  For our purposes, we'll
     * leave it empty for now.
     */

    rc = git_index_write_tree(&tree_id, m_index);
    if (! git_check_ok(rc, "writing initial tree from index"))
        return false;

    rc = git_tree_lookup(&tree, m_repo, &tree_id);
    if (! git_check_ok(rc, "looking up initial tree"))
        return false;

    /**
     * Ready to create the initial commit.
     *
     * Normally creating a commit would involve looking up the current
     * HEAD commit and making that be the parent of the initial commit,
     * but here this is the first commit so there will be no parent.
     */

    if (git_reference_name_to_id(&head_oid, m_repo, "HEAD") < 0)
    {
        TRACE("HEAD not found (no previous commits)");
        head_commit = NULL;
    } else
    {
        rc = git_commit_lookup(&head_commit, m_repo, &head_oid);
        if (! git_check_ok(rc, "looking up HEAD commit"))
            return false;
    }

    if (! head_commit)
    {
        TRACE("Performing first commit.");
        rc = git_commit_create_v(
                    &commit_id, m_repo, "HEAD", sig, sig,
                    NULL, message.c_str(), tree, 0);
        if (! git_check_ok(rc, "creating the initial commit"))
            return false;
    } else
    {
        rc = git_commit_create_v(
                    &commit_id, m_repo, "HEAD", sig, sig,
                    NULL, message.c_str(), tree, 1, head_commit);
        if (! git_check_ok(rc, "creating the commit"))
            return false;
    }

    /** Clean up so we don't leak memory. */

    git_tree_free(tree);
    git_signature_free(sig);

    return true;
}

bool GitRepo::commit_treebuilder(const std::string& message)
{
    git_oid head_oid;
    git_commit *head_commit = NULL;

    git_oid oid_commit;

    int rc;

    ABCA_ASSERT( m_repo, "libgit2 repository must be open" );

    /**
     * Ready to create the initial commit.
     *
     * Normally creating a commit would involve looking up the current
     * HEAD commit and making that be the parent of the initial commit,
     * but here this is the first commit so there will be no parent.
     */

    if (git_reference_name_to_id(&head_oid, m_repo, "HEAD") < 0)
    {
        TRACE("HEAD not found (no previous commits)");
        head_commit = NULL;
    } else
    {
        rc = git_commit_lookup(&head_commit, m_repo, &head_oid);
        if (! git_check_ok(rc, "looking up HEAD commit"))
            return false;
    }

    if (! head_commit)
    {
        TRACE("Performing first commit.");
        rc = git_commit_create_v(
                    &oid_commit, g_ptr(), "HEAD", g_signature(), g_signature(),
                    NULL, message.c_str(), root()->tree(), 0);
        if (! git_check_ok(rc, "creating the initial commit"))
            return false;
    } else
    {
        rc = git_commit_create_v(
                    &oid_commit, g_ptr(), "HEAD", g_signature(), g_signature(),
                    NULL, message.c_str(), root()->tree(), 1, head_commit);
        if (! git_check_ok(rc, "creating the commit"))
            return false;
    }

    return true;
}

/* misc */

std::string GitRepo::relpath(const std::string& pathname_) const
{
    return relative_path(real_path(pathname_), real_path(pathname()));
}

/* groups */

// create a group and add it as a child to this group
GitGroupPtr GitRepo::addGroup( const std::string& name )
{
    ABCA_ASSERT( (name != "/") || (! m_root_group_ptr.get()), "Root group '/' already exists" );

    GitGroupPtr child(new GitGroup( shared_from_this(), name ));
    if (name == "/")
        child->_set_treebld(root());

    return child;
}

GitGroupPtr GitRepo::rootGroup()
{
    if (! m_root_group_ptr.get())
        m_root_group_ptr = addGroup("/");

    return m_root_group_ptr;
}

/* trash history */

#define LOGMESSAGE( TEXT )                                       \
do {                                                             \
    std::ostringstream ss;                                       \
    ss << TEXT;                                                  \
    log_message = ss.str();                                      \
} while( 0 )

bool GitRepo::trashHistory(std::string& errorMessage, const std::string& branchName)
{
    git_repository* repo = g_ptr();
    git_reference*  head = NULL;
    git_tree* tree = NULL;
    git_commit* commit = NULL;
    int rc;
    bool ok = true;

    errorMessage = "";

    /* get head commit and its tree */

    git_repository_head(&head, repo);
    assert(head);
    git_commit_lookup(&commit, repo, git_reference_target(head));
    assert(commit);
    git_reference_free(head);
    head = NULL;

    const git_oid* oid = git_commit_id(commit);
    char oid_str[40];
    git_oid_tostr(oid_str, 40, oid);
    TRACE("[TRASH HISTORY] commit " << oid_str);

    rc = git_commit_tree(&tree, commit);
    ok = ok && git_check_rc(rc, "creating new root commit", errorMessage);
    if (! ok)
    {
        if (tree) git_tree_free(tree);
        git_commit_free(commit);
        return false;
    }
    assert(tree);

    /* create a new commit re-using the tree from the current head commit */
    const git_signature *sig_author = git_commit_author(commit);
    const git_signature *sig_committer = git_commit_committer(commit);
    assert(sig_author);
    assert(sig_committer);

    git_oid oid_fresh_commit;
    rc = git_commit_create(
        &oid_fresh_commit, repo, /* update_ref */ NULL,
        /* author */ sig_author, /* committer */ sig_committer,
        /* encoding */ NULL, git_commit_message(commit),
        tree,
        /* parent_count */ 0, /* parents */ NULL);
    ok = ok && git_check_rc(rc, "creating new root commit", errorMessage);
    if (! ok)
    {
        git_tree_free(tree);
        git_commit_free(commit);
        return false;
    }

    git_commit* fresh_commit = NULL;
    git_commit_lookup(&fresh_commit, repo, &oid_fresh_commit);
    assert(fresh_commit);

    git_oid_tostr(oid_str, 40, &oid_fresh_commit);
    TRACE("[TRASH HISTORY] fresh commit " << oid_str);

    std::string oldBranchName = "old_" + branchName;
    std::string freshBranchName = "fresh";
    std::string log_message;

    /* create a new branch, using the newly created commit */
    git_reference *fresh_branch = NULL;
    LOGMESSAGE("creating new trashed history branch '" << freshBranchName << "'");
    rc = git_branch_create(&fresh_branch, repo,
        /* branch name */ freshBranchName.c_str() /* "fresh" */, /* target */ fresh_commit, /* force */ 0);
    ok = ok && git_check_rc(rc, log_message, errorMessage);
    if (! ok)
    {
        if (fresh_branch) git_reference_free(fresh_branch);
        git_tree_free(tree);
        git_commit_free(commit);
        return false;
    }
    assert(fresh_branch);

    /* Make HEAD point to this branch */
    assert(! head);
    rc = git_reference_symbolic_create(&head, repo,
        /* name */ "HEAD", /* target */ git_reference_name(fresh_branch),
        /* force */ 1,
        /* log_message */ "change HEAD when trashing history");
    ok = ok && git_check_rc(rc, "setting HEAD", errorMessage);
    if (! ok)
    {
        if (head) git_reference_free(head);
        git_reference_free(fresh_branch);
        git_tree_free(tree);
        git_commit_free(commit);
        return false;
    }
    assert(head);
    git_reference_free(head);
    head = NULL;

    /* rename master -> old_master */
    git_reference *master_branch = NULL;
    git_branch_lookup(&master_branch, repo, branchName.c_str() /* "master" */, GIT_BRANCH_LOCAL);
    assert(master_branch);
    git_reference *old_master_branch = NULL;
    LOGMESSAGE("renaming original '" << branchName << "' to '" << oldBranchName << "'");
    rc = git_branch_move(&old_master_branch,
        /* branch */ master_branch, /* new name */ oldBranchName.c_str() /* "old_master" */, /* force */ 0);
    ok = ok && git_check_rc(rc, log_message, errorMessage);
    if (! ok)
    {
        if (old_master_branch) git_reference_free(old_master_branch);
        git_reference_free(master_branch);
        git_reference_free(fresh_branch);
        git_tree_free(tree);
        git_commit_free(commit);
        return false;
    }
    assert(old_master_branch);
    git_reference_free(master_branch); master_branch = NULL;

    /* rename fresh -> master */
    git_reference *new_master_branch = NULL;
    assert(fresh_branch);
    LOGMESSAGE("renaming new '" << freshBranchName << "' to '" << branchName << "'");
    rc = git_branch_move(&new_master_branch,
        /* branch */ fresh_branch, /* new name */ branchName.c_str() /* "master" */, /* force */ 0);
    ok = ok && git_check_rc(rc, log_message, errorMessage);
    if (! ok)
    {
        if (new_master_branch) git_reference_free(new_master_branch);
        git_reference_free(old_master_branch);
        git_reference_free(fresh_branch);
        git_tree_free(tree);
        git_commit_free(commit);
        return false;
    }
    assert(new_master_branch);
    git_reference_free(new_master_branch); new_master_branch = NULL;
    git_reference_free(fresh_branch); fresh_branch = NULL;

    /* delete old_master */
    assert(old_master_branch);
    LOGMESSAGE("deleting original branch (now '" << oldBranchName << "')");
    rc = git_branch_delete(old_master_branch);
    ok = ok && git_check_rc(rc, log_message, errorMessage);
    if (! ok)
    {
        git_reference_free(old_master_branch);
        git_tree_free(tree);
        git_commit_free(commit);
        return false;
    }
    git_reference_free(old_master_branch); old_master_branch = NULL;

    git_tree_free(tree);
    git_commit_free(commit);
    return true;
}

/* fetch history */

std::vector<GitCommitInfo> GitRepo::getHistory(bool& error)
{
    std::vector<GitCommitInfo> results;
    int g_error = 0;

    git_revwalk *walker = NULL;

    error = false;

    if (git_revwalk_new(&walker, g_ptr()) < 0)
        goto fail;
    assert(walker);

    // set the sort order to topological + time: parents after children, sorted by commit time
    git_revwalk_sorting(walker,
        GIT_SORT_TOPOLOGICAL |
        GIT_SORT_TIME);

    /* Pushing marks starting points */
    g_error = git_revwalk_push_head(walker);
    if (g_error) goto fail;
    g_error = git_revwalk_push_ref(walker, "HEAD");
    if (g_error) goto fail;
    g_error = git_revwalk_push_glob(walker, "tags/*");
    if (g_error) goto fail;

    /* Hiding marks stopping points not set/needed */

    /* Walk the walk */
    git_oid oid;
    while (! git_revwalk_next(&oid, walker))
    {
        git_commit *commit = NULL;

        g_error = git_commit_lookup(&commit, g_ptr(), &oid);
        if (g_error)
        {
            if (commit)
                git_commit_free(commit);
            goto fail;
        }
        assert(commit);

        GitCommitInfo cinfo(oid, commit);
        results.push_back(cinfo);

        git_commit_free(commit);
    }

    git_revwalk_free(walker);
    walker = NULL;

    error = false;
    return results;

fail:
    if (walker)
        git_revwalk_free(walker);
    walker = NULL;

    error = true;
    return results;
}

std::string GitRepo::getHistoryJSON(bool& error)
{
    error = false;

    rapidjson::Document document;
    document.SetArray();            // set the document as an array
    // must pass an allocator when the object may need to allocate memory
    rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

    std::vector<GitCommitInfo> commits = getHistory(error);
    if (error)
        return "";
    std::vector<GitCommitInfo>::const_iterator it;
    for (it = commits.begin(); it != commits.end(); ++it)
    {
        const GitCommitInfo& cinfo = *it;

        rapidjson::Value commitInfoJson( rapidjson::kObjectType );
        JsonSet(document, commitInfoJson, "id", cinfo.id);
        JsonSet(document, commitInfoJson, "author", cinfo.author);
        JsonSet(document, commitInfoJson, "email", cinfo.author_email);
        JsonSet(document, commitInfoJson, "when", cinfo.when);
        JsonSet(document, commitInfoJson, "message", cinfo.message);

        document.PushBack(commitInfoJson, allocator);
    }

    return JsonWrite(document);
}

std::ostream& operator<< (std::ostream& out, const GitRepo& repo)
{
    out << "<GitRepo path:'" << repo.pathname() << "' mode:" << repo.mode() << ">";
    return out;
}


/* --- GitTree -------------------------------------------------------- */

GitTree::GitTree(GitRepoPtr repo) :
    m_repo(repo), m_filename("/"), m_tree(NULL), m_ignore_wrong_rev(false), m_error(false)
{
    git_reference* head = NULL;
    git_commit* commit = NULL;

    memset(m_tree_oid.id, 0, GIT_OID_RAWSZ);

    int rc = git_repository_head(&head, m_repo->g_ptr());
    m_error = m_error || git_check_error(rc, "determining HEAD");

    rc = git_commit_lookup(&commit, repo->g_ptr(), git_reference_target(head));
    m_error = m_error || git_check_error(rc, "getting HEAD commit");
    git_reference_free(head);
    head = NULL;

    if (commit)
    {
        rc = git_commit_tree(&m_tree, commit);
        m_error = m_error || git_check_error(rc, "getting commit tree");

        if (m_tree)
            git_oid_cpy(&m_tree_oid, git_tree_id(m_tree));
    }

    git_commit_free(commit);
    commit = NULL;
}

GitTree::GitTree(GitRepoPtr repo, const std::string& rev_str, bool ignoreWrongRevision) :
    m_repo(repo), m_filename("/"), m_tree(NULL), m_revision(rev_str), m_ignore_wrong_rev(ignoreWrongRevision), m_error(false)
{
    git_object* obj = NULL;
    git_commit* commit = NULL;

    memset(m_tree_oid.id, 0, GIT_OID_RAWSZ);

    int rc = git_revparse_single(&obj, repo->g_ptr(), m_revision.c_str());
    if ((rc != GIT_SUCCESS) && m_ignore_wrong_rev)
    {
        goto go_ahead;
    }
    m_error = m_error || git_check_error(rc, "parsing revision string");

    rc = git_commit_lookup(&commit, repo->g_ptr(), git_object_id(obj));
    m_error = m_error || git_check_error(rc, "getting commit for revision string");

    // rc = git_tree_lookup(&m_tree, repo->g_ptr(), git_object_id(obj));
    // m_error = m_error || git_check_error(rc, "looking up tree");

go_ahead:
    if ((! commit) && m_ignore_wrong_rev)
    {
        /* flag to ignore wrong/non-existent revision specified: fetch HEAD */

        std::cerr << "WARNING: specified revision not found, but ignore-wrong-rev flag active: try to fetch/return HEAD" << std::endl;

        git_reference* head = NULL;

        rc = git_repository_head(&head, m_repo->g_ptr());
        m_error = m_error || git_check_error(rc, "determining HEAD");

        rc = git_commit_lookup(&commit, repo->g_ptr(), git_reference_target(head));
        m_error = m_error || git_check_error(rc, "getting HEAD commit");
        git_reference_free(head);
        head = NULL;
    }

    if (commit)
    {
        rc = git_commit_tree(&m_tree, commit);
        m_error = m_error || git_check_error(rc, "getting commit tree");

        if (m_tree)
            git_oid_cpy(&m_tree_oid, git_tree_id(m_tree));

        git_commit_free(commit);
        commit = NULL;
    }

    git_object_free(obj);
    obj = NULL;
}

GitTree::GitTree(GitRepoPtr repo, GitTreePtr parent_ptr, git_tree* lg_ptr) :
    m_repo(repo), m_parent(parent_ptr), m_tree(NULL), m_ignore_wrong_rev(false), m_error(false)
{
    memset(m_tree_oid.id, 0, GIT_OID_RAWSZ);

    m_tree = lg_ptr;

    if (m_tree)
        git_oid_cpy(&m_tree_oid, git_tree_id(m_tree));
}

GitTree::~GitTree()
{
    if (m_tree)
        git_tree_free(m_tree);
    m_tree = NULL;
}

boost::optional<GitTreePtr> GitTree::Create(GitTreePtr parent_ptr, const std::string& filename)
{
    assert(parent_ptr.get());

    return parent_ptr->getChildTree(filename);
}

std::string GitTree::pathname() const
{
    if (isRoot())
        return filename();

    assert(m_parent && m_parent.get());

    return v_pathjoin(m_parent->pathname(), filename(), '/');
}

std::string GitTree::relPathname() const
{
    if (isRoot())
        return filename();

    assert(m_parent && m_parent.get());

    return pathjoin(m_parent->relPathname(), filename());
}

std::string GitTree::absPathname() const
{
    return pathjoin(m_repo->pathname(), pathname());
}

// WARNING: be sure to have an existing shared_ptr to this treebuilder
// before calling the following method!
// (Because it uses shared_from_this()...)
boost::optional<GitTreePtr> GitTree::getChildTree(const std::string& filename)
{
    if (! m_tree)
        return boost::none;

    assert(m_tree);

    const git_tree_entry *entry = git_tree_entry_byname(m_tree, filename.c_str());
    if (! entry)
        return boost::none;

    assert(entry);
    if (git_tree_entry_type(entry) == GIT_OBJ_TREE)
    {
        git_tree *subtree = NULL;
        int rc = git_tree_lookup(&subtree, repo()->g_ptr(), git_tree_entry_id(entry));
        m_error = m_error || git_check_error(rc, "getting child tree");
        return GitTreePtr(new GitTree(repo(), shared_from_this(), subtree));
    }

    return boost::none;
}

boost::optional<std::string> GitTree::getChildFile(const std::string& filename)
{
    if (! m_tree)
        return boost::none;

    assert(m_tree);

    const git_tree_entry *entry = git_tree_entry_byname(m_tree, filename.c_str());
    if (! entry)
        return boost::none;

    assert(entry);
    if (git_tree_entry_type(entry) == GIT_OBJ_BLOB)
    {
        git_blob *blob = NULL;
        int rc = git_blob_lookup(&blob, repo()->g_ptr(), git_tree_entry_id(entry));
        m_error = m_error || git_check_error(rc, "getting child blob");

        std::string blob_str(static_cast<const char *>(git_blob_rawcontent(blob)), git_blob_rawsize(blob));

        git_blob_free(blob);
        blob = NULL;

        return blob_str;
    }

    return boost::none;
}

bool GitTree::hasChild(const std::string& filename)
{
    if (! m_tree)
        return false;

    const git_tree_entry *entry = git_tree_entry_byname(m_tree, filename.c_str());
    if (! entry)
        return false;

    assert(entry);
    return true;
}

bool GitTree::hasFileChild(const std::string& filename)
{
    if (! m_tree)
        return false;

    const git_tree_entry *entry = git_tree_entry_byname(m_tree, filename.c_str());
    if (! entry)
        return false;

    assert(entry);
    return (git_tree_entry_type(entry) == GIT_OBJ_BLOB);
}

bool GitTree::hasTreeChild(const std::string& filename)
{
    if (! m_tree)
        return false;

    const git_tree_entry *entry = git_tree_entry_byname(m_tree, filename.c_str());
    if (! entry)
        return false;

    assert(entry);
    return (git_tree_entry_type(entry) == GIT_OBJ_TREE);
}

std::ostream& operator<< (std::ostream& out, const GitTree& tree)
{
    out << "<GitTree path:'" << tree.pathname() << "'>";
    return out;
}

/* --- GitTreebuilder ------------------------------------------------- */

GitTreebuilder::GitTreebuilder(GitRepoPtr repo) :
    m_repo(repo), m_tree_bld(NULL), m_tree(NULL), m_written(false), m_dirty(false), m_error(false)
{
    int rc = git_treebuilder_new(&m_tree_bld, m_repo->g_ptr(), /* source */ NULL);
    m_error = m_error || git_check_error(rc, "creating treebuilder");
    memset(m_tree_oid.id, 0, GIT_OID_RAWSZ);
}

GitTreebuilderPtr GitTreebuilder::Create(GitTreebuilderPtr parent, const std::string& filename)
{
    assert(parent.get());

    GitTreebuilderPtr ptr = GitTreebuilder::Create(parent->repo());
    ptr->_set_parent(parent);
    ptr->_set_filename(filename);
    return ptr;
}

void GitTreebuilder::_set_parent(GitTreebuilderPtr parent)
{
    m_parent = parent;
}

void GitTreebuilder::_set_filename(const std::string& filename)
{
    m_filename = filename;
}

GitTreebuilder::~GitTreebuilder()
{
    if (m_tree)
        git_tree_free(m_tree);
    m_tree = NULL;
    if (m_tree_bld)
        git_treebuilder_free(m_tree_bld);
    m_tree_bld = NULL;
}

std::string GitTreebuilder::pathname() const
{
    if (isRoot())
        return filename();

    assert(m_parent && m_parent.get());

    return v_pathjoin(m_parent->pathname(), filename(), '/');
}

std::string GitTreebuilder::relPathname() const
{
    if (isRoot())
        return filename();

    assert(m_parent && m_parent.get());

    return pathjoin(m_parent->relPathname(), filename());
}

std::string GitTreebuilder::absPathname() const
{
    return pathjoin(m_repo->pathname(), pathname());
}

GitTreebuilderPtr GitTreebuilder::getChild(const std::string& filename)
{
    std::vector<GitTreebuilderPtr>::const_iterator it;
    for (it = m_children.begin(); it != m_children.end(); ++it)
    {
        const GitTreebuilderPtr& child = *it;

        if (child->filename() == filename)
            return child;
    }

    return GitTreebuilderPtr();
}

bool GitTreebuilder::dirty() const
{
    if (m_dirty)
        return true;

    std::vector<GitTreebuilderPtr>::const_iterator it;
    for (it = m_children.begin(); it != m_children.end(); ++it)
    {
        const GitTreebuilderPtr& child = *it;

        if (child->dirty())
            return true;
    }

    return false;
}

bool GitTreebuilder::write()
{
    bool op_ok, ok = true;
    int rc;

    if (m_written && !dirty())
        return true;

    std::vector<GitTreebuilderPtr>::iterator it;
    for (it = m_children.begin(); it != m_children.end(); ++it)
    {
        GitTreebuilderPtr& child = *it;

        op_ok = _write_subtree_and_insert(child);
        ok = ok && op_ok;
    }
    m_error = m_error || (!ok);
    if (! ok)
        return false;

    rc = git_treebuilder_write(&m_tree_oid, m_tree_bld);
    ok = ok && git_check_ok(rc, "writing treebuilder to the db");
    if (! ok) goto ret;

    rc = git_tree_lookup(&m_tree, m_repo->g_ptr(), &m_tree_oid);
    ok = ok && git_check_ok(rc, "looking up tree from tree oid");
    if (! ok) goto ret;

ret:
    m_error = m_error || (!ok);
    m_written = ok;
    m_dirty = m_dirty && !ok;
    return m_written;
}

bool GitTreebuilder::add_file_from_memory(const std::string& filename, const std::string& contents)
{
    git_oid oid_blob;                       /* the SHA1 for our blob in the tree */
    git_blob *blob = NULL;                  /* our blob in the tree */
    int rc;

    rc = git_blob_create_frombuffer(&oid_blob, m_repo->g_ptr(),
        contents.c_str(), contents.length());
    m_error = m_error || git_check_error(rc, "creating blob");
    if (m_error) goto ret;

    rc = git_blob_lookup(&blob, m_repo->g_ptr(), &oid_blob);
    m_error = m_error || git_check_error(rc, "looking up blob oid");
    if (m_error) goto ret;

    rc = git_treebuilder_insert(NULL, m_tree_bld,
        filename.c_str(), &oid_blob, GIT_FILEMODE_BLOB);
    m_error = m_error || git_check_error(rc, "adding blob to treebuilder");
    if (m_error) goto ret;

    m_dirty = m_dirty || !m_error;

ret:
    if (blob)
        git_blob_free(blob);
    return !m_error;
}

GitTreebuilderPtr GitTreebuilder::add_tree(const std::string& filename)
{
    GitTreebuilderPtr childPtr = GitTreebuilder::Create(ptr(), filename);

    bool ok = _add_subtree(childPtr);
    if (ok)
        return childPtr;
    return GitTreebuilderPtr();
}

bool GitTreebuilder::_add_subtree(GitTreebuilderPtr subtreePtr)
{
    if (m_children_set.count(subtreePtr) != 0)
        return true;

    m_children_set.insert(subtreePtr);
    m_children.push_back(subtreePtr);

    return true;
}

bool GitTreebuilder::_write_subtree_and_insert(GitTreebuilderPtr child)
{
    assert(child && child.get());

    if (! child->write())
        return false;

    git_oid subtree_oid = child->oid();

    int rc = git_treebuilder_insert(NULL, m_tree_bld,
        child->filename().c_str(), &subtree_oid, GIT_FILEMODE_TREE);
    m_error = m_error || git_check_error(rc, "adding subdirectory tree to treebuilder");

    m_dirty = m_dirty || !m_error;

    return !m_error;
}

std::ostream& operator<< (std::ostream& out, const GitTreebuilder& treeBld)
{
    out << "<GitTreebuilder path:'" << treeBld.pathname() << "'>";
    return out;
}


/* --- GitObject ------------------------------------------------------ */

GitObject::GitObject( GitRepoPtr repo, git_oid oid ) :
    m_repo_ptr(repo), m_oid(oid), m_obj(NULL)
{
    int error = git_odb_read(&m_obj, g_odb(), &m_oid);
    git_check_error(error, "reading git object from odb");
}

GitObject::GitObject( GitRepoPtr repo, const std::string& oid_hex ) :
    m_repo_ptr(repo), m_obj(NULL)
{
    int error = git_oid_fromstr(&m_oid, oid_hex.c_str());
    git_check_error(error, "getting git oid from hex string");

    git_odb_read(&m_obj, g_odb(), &m_oid);
    git_check_error(error, "reading git object from odb");
}

std::string GitObject::hex() const
{
    char out[41];
    out[40] = '\0';

    // If you have a oid, you can easily get the hex value of the SHA as well.
    git_oid_fmt(out, &m_oid);
    out[40] = '\0';
    return out;
}

GitObject::~GitObject()
{
    if (m_obj)
    {
        git_odb_object_free(m_obj);
        m_obj = NULL;
    }
}


/* --- GitGroup ------------------------------------------------------- */

GitGroup::GitGroup( GitRepoPtr repo, const std::string& name ) :
    m_repo_ptr(repo), m_name(name), m_written(false), m_read(false)
{
    // top-level group
    TRACE("GitGroup::GitGroup(repo) created:" << repr());

    const GitMode& mode_ = mode();
    if ((mode_ == GitMode::Read) || (mode_ == GitMode::ReadWrite))
        readFromDisk();
}

GitGroup::GitGroup( GitGroupPtr parent, const std::string& name ) :
    m_repo_ptr(parent->repo()), m_parent_ptr(parent), m_name(name),
    m_written(false), m_read(false)
{
    // child group
    TRACE("GitGroup::GitGroup(parent) created:" << repr());

    const GitMode& mode_ = mode();
    if ((mode_ == GitMode::Read) || (mode_ == GitMode::ReadWrite))
        readFromDisk();
}

//-*****************************************************************************
GitGroup::~GitGroup()
{
    const GitMode& mode_ = mode();
    if ((mode_ == GitMode::Write) || (mode_ == GitMode::ReadWrite))
        writeToDisk();

    // not necessary, but better clean up memory in case of nasty bugs...
    m_parent_ptr.reset();
    m_repo_ptr.reset();

    m_written = false;
    m_read = false;
}

// create a group and add it as a child to this group
GitGroupPtr GitGroup::addGroup( const std::string& name )
{
    GitGroupPtr child(new GitGroup( shared_from_this(), name ));

    assert(! (child->m_treebld_ptr || child->m_treebld_ptr.get()));

    GitTreebuilderPtr treebld_ptr = treebuilder();
    if (treebld_ptr)
    {
        child->_set_treebld( treebld_ptr->add_tree(name) );
    }

    return child;
}

/* tree API (R) */

GitTreePtr GitGroup::tree()
{
    if (! m_tree_ptr)
    {
        if (isTopLevel())
        {
            if (hasRevisionSpec())
            {
                assert(isTopLevel());
                assert(! m_tree_ptr);

                ABCA_ASSERT( isTopLevel(), "Specifying a revision is only possible for toplevel groups" );
                ABCA_ASSERT( (!m_tree_ptr), "Tree already determined" );

                TRACE("fetching top-level tree for revision '" << revisionString() << "' (ignore wrong revision: " << ignoreWrongRevision() << ")");
                m_tree_ptr = GitTree::Create(repo(), revisionString(), ignoreWrongRevision());
            } else
                m_tree_ptr = GitTree::Create(repo());
        }
        else
        {
            boost::optional<GitTreePtr> o_tree = GitTree::Create(parent()->tree(), filename());

            if (o_tree)
                m_tree_ptr = *o_tree;
            else
            {
                ABCA_THROW( "can't obtain git tree for '" << pathname() << "'" );
            }
        }
    }

    return m_tree_ptr;
}

bool GitGroup::hasGitTree()
{
    if (m_tree_ptr)
        return true;

    if (isTopLevel())
        return true;

    if (! parent()->tree())
        return false;

    return parent()->tree()->hasTreeChild(filename());
}

GitTreebuilderPtr GitGroup::treebuilder()
{
    if (! m_treebld_ptr)
    {
        GitGroupPtr parent_ = parent();
        if (parent_)
        {
            _set_treebld( parent_->treebuilder()->add_tree(name()) );
        }
    }

    return m_treebld_ptr;
};

bool GitGroup::add_file_from_memory(const std::string& filename, const std::string& contents)
{
    return treebuilder()->add_file_from_memory(filename, contents);
}

GitDataPtr GitGroup::addData(Alembic::Util::uint64_t iSize, const void * iData)
{
    UNIMPLEMENTED("GitGroup::addData(size, data);");
    return GitDataPtr( new GitData() );
}

// write data streams from multiple sources as one continuous data stream
// and add it as a child to this group
GitDataPtr GitGroup::addData(Alembic::Util::uint64_t iNumData,
                 const Alembic::Util::uint64_t * iSizes,
                 const void ** iDatas)
{
    UNIMPLEMENTED("GitGroup::addData(num, sizes, datas);");
    return GitDataPtr( new GitData() );
}

// reference existing data
void GitGroup::addData(GitDataPtr iData)
{
    UNIMPLEMENTED("GitGroup::addData(GitDataPtr);");
}

// reference an existing group
void GitGroup::addGroup(GitGroupPtr iGroup)
{
    UNIMPLEMENTED("GitGroup::addGroup(GitGroupPtr);");
}

// convenience function for adding empty data
void GitGroup::addEmptyData()
{
    UNIMPLEMENTED("GitGroup::addEmptyData();");
}

std::string GitGroup::fullname() const
{
    if (isTopLevel())
        return name();

    ABCA_ASSERT( m_parent_ptr, "Invalid parent group" );

    return v_pathjoin(m_parent_ptr->fullname(), name(), '/');
}

std::string GitGroup::relPathname() const
{
    if (isTopLevel())
        return name();

    ABCA_ASSERT( m_parent_ptr, "Invalid parent group" );

    return pathjoin(m_parent_ptr->relPathname(), name());
}

std::string GitGroup::absPathname() const
{
    return pathjoin(m_repo_ptr->pathname(), pathname());
}

void GitGroup::writeToDisk()
{
#if JSON_TO_DISK
    const GitMode& mode_ = mode();
    if ((mode_ != GitMode::Write) && (mode_ != GitMode::ReadWrite))
        return;

    if (! m_written)
    {
        TRACE("GitGroup::writeToDisk() path:'" << absPathname() << "' (WRITING)");
        bool r = mkpath( absPathname(), 0777 );
        ABCA_ASSERT(r || (errno == EEXIST), "can't create directory '" << absPathname() << "'");

        m_written = true;
    } else
    {
        TRACE("GitGroup::writeToDisk() path:'" << absPathname() << "' (skipping, already written)");
    }
#else

    m_written = true;

#endif

    ABCA_ASSERT( m_written, "data not written" );
}

bool GitGroup::readFromDisk()
{
    const GitMode& mode_ = mode();
    if ((mode_ != GitMode::Read) && (mode_ != GitMode::ReadWrite))
        return false;

    if (! m_read)
    {
#if JSON_TO_DISK
        TRACE("GitGroup::readFromDisk() path:'" << absPathname() << "' (READING)");
        ABCA_ASSERT(isdir(absPathname()), "directory '" << absPathname() << "' doesn't exist");

        m_read = true;
#else /* ! JSON_TO_DISK */
        TRACE("GitGroup::readFromDisk() path:'" << absPathname() << "' (READING)");
        ABCA_ASSERT(hasGitTree(), "git tree for '" << absPathname() << "' doesn't exist");

        m_read = true;
#endif /* ! JSON_TO_DISK */
    } else
    {
        TRACE("GitGroup::readFromDisk() path:'" << absPathname() << "' (skipping, already read)");
    }

    ABCA_ASSERT( m_read, "data not read" );
    return m_read;
}

std::string GitGroup::repr(bool extended) const
{
    std::ostringstream ss;
    std::string parentRepr;
    if (extended)
    {
        if (isTopLevel())
        {
            ABCA_ASSERT( !m_parent_ptr, "Invalid parent group (should be null)" );
            ss << "<GitGroup TOP repo:'" << m_repo_ptr->pathname() <<
                "' name:'" << name() <<
                "' fullname:'" << fullname() <<
                "'" << ">";
        } else
        {
            ABCA_ASSERT( m_parent_ptr, "Invalid parent group" );
            parentRepr = m_parent_ptr->repr();
            ss << "<GitGroup repo:'" << m_repo_ptr->pathname() <<
                "' parent:" << parentRepr <<
                "' name:'" << name() <<
                "' fullname:'" << fullname() <<
                "'" << ">";
        }
    } else
    {
        ss << "'" << pathname() << "'";
    }
    return ss.str();
}

bool GitGroup::finalize()
{
#if JSON_TO_DISK
    return mkpath( absPathname(), 0777 );
#else
    return true;
#endif
}

void GitGroup::_set_treebld(GitTreebuilderPtr treebld_ptr)
{
    assert((! m_treebld_ptr) || (! m_treebld_ptr.get()));

    m_treebld_ptr = treebld_ptr;
}

std::ostream& operator<< (std::ostream& out, const GitGroup& group)
{
    if (group.isTopLevel())
    {
        ABCA_ASSERT( !group.parent(), "Invalid parent group (should be null)" );
        out << "<GitGroup TOP repo:'" << group.repo()->pathname() <<
            "' pathname:'" << group.pathname() <<
            "'" << ">";
    } else
    {
        ABCA_ASSERT( group.parent(), "Invalid parent group" );
        out << "<GitGroup repo:'" << group.repo()->pathname() <<
            "' pathname:'" << group.pathname() <<
            "'" << ">";
    }
    return out;
}


/* --- GitData -------------------------------------------------------- */

GitData::GitData(GitGroupPtr iGroup, Alembic::Util::uint64_t iPos,
          Alembic::Util::uint64_t iSize) :
    m_group(iGroup),
    m_pos(iPos),
    m_size(iSize)
{
    ABCA_ASSERT( m_group, "invalid group" );
}

GitData::GitData()
{
}

GitData::~GitData()
{
}

void GitData::write(Alembic::Util::uint64_t iSize, const void * iData)
{
    UNIMPLEMENTED("GitData::write(size, data)");
}

void GitData::write(Alembic::Util::uint64_t iNumData,
            const Alembic::Util::uint64_t * iSizes,
            const void ** iDatas)
{
    UNIMPLEMENTED("GitData::write(num, sizes, datas)");
}

// rewrites over part of the already written data, does not change
// the size of the already written data.  If what is attempting
// to be rewritten exceeds the boundaries of what is already written,
// the existing data will be unchanged
void GitData::rewrite(Alembic::Util::uint64_t iSize, void * iData,
             Alembic::Util::uint64_t iOffset)
{
    UNIMPLEMENTED("GitData::rewrite(size, data, offset)");
}

Alembic::Util::uint64_t GitData::getSize() const
{
    return m_size;
}

std::string GitData::repr(bool extended) const
{
    std::ostringstream ss;

    if (extended)
    {
        if (m_group)
            ss << "<GitData group:" << m_group->repr() << ">";
        else
            ss << "<GitData>";
    } else
    {
        ss << "<GitData>";
    }
    return ss.str();
}

Alembic::Util::uint64_t GitData::getPos() const
{
    return m_pos;
}

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreGit
} // End namespace Alembic
