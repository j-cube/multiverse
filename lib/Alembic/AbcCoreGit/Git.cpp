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

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

#ifndef GIT_SUCCESS
#define GIT_SUCCESS 0
#endif /* GIT_SUCCESS */

static std::string pathjoin(std::string p1, std::string p2, char pathsep = '/')
{
    if (p1.length() == 0)
        return p2;
    if (p2.length() == 0)
        return p1;

    ABCA_ASSERT(p1.length() > 0, "invalid string length");
    ABCA_ASSERT(p2.length() > 0, "invalid string length");

    char p_last_ch = *p1.rbegin();
    char first_ch = p2[0];
    if ((p_last_ch == pathsep) || (first_ch == pathsep))
        return p1 + p2;
    return p1 + pathsep + p2;

}

void git_check_error(int error_code, const std::string& action)
{
    if ( !error_code )
       return;

    const git_error *error = giterr_last();

    ABCA_THROW( "libgit2 error " << error_code << " " << action <<
        " - " << ((error && error->message) ? error->message : "???") );
}

//-*****************************************************************************
GitRepo::GitRepo( const std::string& pathname, git_repository *repo, git_config *cfg ) :
    m_pathname(pathname), m_repo(repo), m_repo_cfg(cfg), m_odb(NULL)
{
    if (m_repo)
    {
        int error = git_repository_odb(&m_odb, m_repo);
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

// create a group and add it as a child to this group
GitGroupPtr GitRepo::addGroup( const std::string& name )
{
    GitGroupPtr child(new GitGroup( shared_from_this(), name ));
    return child;
}

std::string GitRepo::repr(bool extended) const
{
    std::ostringstream ss;
    if (extended)
        ss << "<GitRepo path:'" << m_pathname << "'>";
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
    m_repo_ptr(repo), m_name(name), m_written(false)
{
    // top-level group
    TRACE("GitGroup::GitGroup(repo) created:" << repr());
}

GitGroup::GitGroup( GitGroupPtr parent, const std::string& name ) :
    m_repo_ptr(parent->repo()), m_parent_ptr(parent), m_name(name)
{
    // child group
    TRACE("GitGroup::GitGroup(parent) created:" << repr());
}

//-*****************************************************************************
GitGroup::~GitGroup()
{
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
    UNIMPLEMENTED("GitGroup::addData(GitGroupPtr);");
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
    if (! m_written)
    {
        TRACE("GitGroup::writeToDisk() path:'" << absPathname() << "' (WRITING)");
        int rc = mkpath( absPathname(), 0777 );
        ABCA_ASSERT((rc == 0) || (errno == EEXIST), "can't create directory '" << absPathname() << "'");
    } else
    {
        TRACE("GitGroup::writeToDisk() path:'" << absPathname() << "' (skipping, already written)");
    }
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
