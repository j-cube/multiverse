//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

#include <Alembic/AbcCoreGit/Git.h>
#include <Alembic/AbcCoreGit/Utils.h>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdlib>

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include "Utils.h"

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

#ifndef GIT_SUCCESS
#define GIT_SUCCESS 0
#endif /* GIT_SUCCESS */

static bool gitlib_initialized = false;

// TODO: find a better way to initialize and cleanup libgit2
static void gitlib_initialize()
{
    if (! gitlib_initialized)
    {
        git_threads_init();

        gitlib_initialized = true;
    }
}

// TODO: call gitlib_cleanup() somewhere
#if 0
static void gitlib_cleanup()
{
    if (gitlib_initialized)
    {
        git_threads_shutdown();

        gitlib_initialized = false;
    }
}
#endif /* 0 */

void git_check_error(int error_code, const std::string& action)
{
    if ( !error_code )
       return;

    const git_error *error = giterr_last();

    ABCA_THROW( "libgit2 error " << error_code << " " << action <<
        " - " << ((error && error->message) ? error->message : "???") );
}

std::ostream& operator<< ( std::ostream& out, const GitMode& value )
{
    switch (value)
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

//-*****************************************************************************
GitRepo::GitRepo( const std::string& pathname, git_repository *repo, git_config *cfg ) :
    m_mode(GitMode::Write), m_pathname(pathname), m_repo(repo), m_repo_cfg(cfg), m_odb(NULL)
{
    gitlib_initialize();

    if (m_repo)
    {
        int error = git_repository_odb(&m_odb, m_repo);
        git_check_error(error, "accessing git repository database");
    }
}

GitRepo::GitRepo( const std::string& pathname, GitMode mode ) :
    m_mode(mode), m_pathname(pathname), m_odb(NULL)
{
    TRACE( "GitRepo::GitRepo(pathname:'" << pathname << "' mode:" << mode );

    gitlib_initialize();

    int error;

    git_repository *repo = NULL;
    git_config *cfg = NULL;

    std::string git_dir = m_pathname + "/.git";

    if ((m_mode == GitMode::Write) || (m_mode == GitMode::ReadWrite))
    {
        if (! isdir(m_pathname))
        {
            TRACE( "initializing Git repository '" << m_pathname << "'" );
            error = git_repository_init (&repo, m_pathname.c_str(), /* is_bare */ 0);
            git_check_error(error, "initializing git repository");
            if (( error != GIT_SUCCESS ) || ( !repo ))
            {
                ABCA_THROW( "Could not open (initialize) file: " << m_pathname << " (git repo)");
            }
            git_repository_config(&cfg, repo /* , NULL, NULL */);

            git_config_set_int32 (cfg, "core.repositoryformatversion", 0);
            git_config_set_bool (cfg, "core.filemode", 1);
            git_config_set_bool (cfg, "core.bare", 0);
            git_config_set_bool (cfg, "core.logallrefupdates", 1);
            git_config_set_bool (cfg, "core.ignorecase", 1);

            // set the version of the Alembic git backend
            // This expresses the AbcCoreGit version - how properties, are stored within Git, etc.
            git_config_set_int32 (cfg, "alembic.formatversion", ALEMBIC_GIT_FILE_VERSION);
            // This is the Alembic library version XXYYZZ
            // Where XX is the major version, YY is the minor version
            // and ZZ is the patch version
            git_config_set_int32 (cfg, "alembic.libversion", ALEMBIC_LIBRARY_VERSION);

            //git_repository_set_workdir(repo, m_fileName);

            git_repository_free(repo);
            repo = NULL;
            //Sleep (1000);
        } else
        {
            TRACE( "Git repository '" << m_pathname << "' exists, opening (for writing)" );
        }
    }

    error = git_repository_open (&repo, git_dir.c_str());
    git_check_error(error, "opening git repository");
    if (( error != GIT_SUCCESS ) || ( !repo ))
    {
        TRACE("error opening repository from '" << git_dir << "'");
        ABCA_THROW( "Could not open file: " << m_pathname << " (git repo)");
    }
    if (repo)
    {
        error = git_repository_config(&cfg, repo /* , NULL, NULL */);
        if (( error != GIT_SUCCESS ) || ( !cfg ))
        {
            TRACE("error fetching config for repository '" << git_dir << "'");
            ABCA_THROW( "Could not get Git configuration for repository: " << m_pathname);
        }
    }

    m_repo = repo;
    m_repo_cfg = cfg;

    if (m_repo)
    {
        error = git_repository_odb(&m_odb, m_repo);
        git_check_error(error, "accessing git repository database");
    }
}

GitRepo::~GitRepo()
{
    if (m_repo)
    {
        git_repository_free(m_repo);
        m_repo = NULL;

        // TODO: should we free() m_repo_cfg (git_config *)?
        m_repo_cfg = NULL;

        // TODO: should we free() m_odb (git_odb *)?
        m_odb = NULL;
    }
}

int32_t GitRepo::formatVersion() const
{
    int error;

    int32_t formatversion = -1;

    error = git_config_get_int32(&formatversion, g_config(), "alembic.formatversion");
    git_check_error(error, "getting alembic.formatversion config value");

    return formatversion;
}

int32_t GitRepo::libVersion() const
{
    int error;

    int32_t libversion = -1;

    error = git_config_get_int32(&libversion, g_config(), "alembic.libversion");
    git_check_error(error, "getting alembic.libversion config value");

    return libversion;
}

bool GitRepo::isValid() const
{
    return m_repo ? true : false;
}

bool GitRepo::isClean() const
{
    // [TODO]: implement git clean check
    return false;
}

bool GitRepo::isFrozen() const
{
    // [TODO]: implement frozen check
    return true;
}

// create a group and add it as a child to this group
GitGroupPtr GitRepo::addGroup( const std::string& name )
{
    ABCA_ASSERT( (name != "/") || (! m_root_group_ptr.get()), "Root group '/' already exists" );
    GitGroupPtr child(new GitGroup( shared_from_this(), name ));
    return child;
}

GitGroupPtr GitRepo::rootGroup()
{
    if (! m_root_group_ptr.get())
        m_root_group_ptr = addGroup("/");

    return m_root_group_ptr;
}

std::string GitRepo::repr(bool extended) const
{
    std::ostringstream ss;

    if (extended)
        ss << "<GitRepo path:'" << m_pathname << "' mode:" << m_mode << ">";
    else
        ss << "'" << m_pathname << "'";
    return ss.str();
}

//-*****************************************************************************
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

//-*****************************************************************************
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
    m_repo_ptr(parent->repo()), m_parent_ptr(parent), m_name(name)
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
}

// create a group and add it as a child to this group
GitGroupPtr GitGroup::addGroup( const std::string& name )
{
    GitGroupPtr child(new GitGroup( shared_from_this(), name ));
    return child;
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

    return pathjoin(m_parent_ptr->fullname(), name(), '/');
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
    const GitMode& mode_ = mode();
    if ((mode_ != GitMode::Write) && (mode_ != GitMode::ReadWrite))
        return;

    if (! m_written)
    {
        TRACE("GitGroup::writeToDisk() path:'" << absPathname() << "' (WRITING)");
        int rc = mkpath( absPathname(), 0777 );
        ABCA_ASSERT((rc == 0) || (errno == EEXIST), "can't create directory '" << absPathname() << "'");

        m_written = true;
    } else
    {
        TRACE("GitGroup::writeToDisk() path:'" << absPathname() << "' (skipping, already written)");
    }

    ABCA_ASSERT( m_written, "data not written" );
}

bool GitGroup::readFromDisk()
{
    const GitMode& mode_ = mode();
    if ((mode_ != GitMode::Read) && (mode_ != GitMode::ReadWrite))
        return false;

    if (! m_read)
    {
        TRACE("GitGroup::readFromDisk() path:'" << absPathname() << "' (READING)");
        ABCA_ASSERT(isdir(absPathname()), "directory '" << absPathname() << "' doesn't exist");

        m_read = true;
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
    int rc = mkpath( absPathname(), 0777 );
    return (rc == 0);
}

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
