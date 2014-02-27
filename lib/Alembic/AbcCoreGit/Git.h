//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

#ifndef _Alembic_AbcCoreGit_Git_h_
#define _Alembic_AbcCoreGit_Git_h_

#include <Alembic/AbcCoreGit/Foundation.h>
#include <Alembic/AbcCoreGit/Utils.h>
#include <git2.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

void git_check_error(int error_code, const std::string& action);

class GitGroup;
typedef Alembic::Util::shared_ptr<GitGroup> GitGroupPtr;
typedef Alembic::Util::shared_ptr<const GitGroup> GitGroupConstPtr;

class GitData;
typedef Alembic::Util::shared_ptr<GitData> GitDataPtr;
typedef Alembic::Util::shared_ptr<const GitData> GitDataConstPtr;

//-*****************************************************************************
class GitRepo : public Alembic::Util::enable_shared_from_this<GitRepo>
{
public:
    GitRepo( const std::string& pathname, git_repository *repo, git_config *cfg );
    virtual ~GitRepo();

    // create a top-level group from this repo
    GitGroupPtr addGroup( const std::string& name );

    const std::string& pathname()                 { return m_pathname; }
    git_repository *g_repository() const          { return m_repo; }

    git_config *g_config() const                  { return m_repo_cfg; }

    git_odb *g_odb() const                        { return m_odb; }

    std::string repr(bool extended=false) const;

private:
    std::string     m_pathname;
    git_repository  *m_repo;
    git_config      *m_repo_cfg;
    git_odb         *m_odb;
};

typedef Alembic::Util::shared_ptr<GitRepo> GitRepoPtr;
typedef Alembic::Util::shared_ptr<const GitRepo> GitRepoConstPtr;

//-*****************************************************************************
class GitObject : public Alembic::Util::enable_shared_from_this<GitObject>
{
public:
    GitObject( GitRepoPtr repo, git_oid oid );
    GitObject( GitRepoPtr repo, const std::string& oid_hex );
    virtual ~GitObject();

    GitRepoPtr repo()                             { return m_repo_ptr; }
    GitRepoConstPtr repo() const                  { return m_repo_ptr; }

    git_repository *g_repository() const          { return repo()->g_repository(); }

    git_config *g_config() const                  { return repo()->g_config(); }

    git_odb *g_odb() const                        { return repo()->g_odb(); }

    git_oid g_oid() const                         { return m_oid; }

    git_odb_object *g_object() const              { return m_obj; }

    std::string hex() const;

private:
    GitRepoPtr      m_repo_ptr;

    git_oid         m_oid;
    git_odb_object *m_obj;
};

typedef Alembic::Util::shared_ptr<GitObject> GitObjectPtr;

//-*****************************************************************************
//! Represents a Group in the Git alembic backend and at the filesystem level
//! corresponds to a directory in the working tree
class GitGroup : public Alembic::Util::enable_shared_from_this<GitGroup>
{
public:
    GitGroup( GitRepoPtr repo, const std::string& name );        // top-level group
    GitGroup( GitGroupPtr parent, const std::string& name );     // child group
    virtual ~GitGroup();

    // create a group and add it as a child to this group
    GitGroupPtr addGroup( const std::string& name );

    // write the data stream and add it as a child to this group
    GitDataPtr addData(Alembic::Util::uint64_t iSize, const void * iData);

    // write data streams from multiple sources as one continuous data stream
    // and add it as a child to this group
    GitDataPtr addData(Alembic::Util::uint64_t iNumData,
                     const Alembic::Util::uint64_t * iSizes,
                     const void ** iDatas);

    // reference existing data
    void addData(GitDataPtr iData);

    // reference an existing group
    void addGroup(GitGroupPtr iGroup);

    // convenience function for adding empty data
    void addEmptyData();

    bool isTopLevel() const                       { return (!m_parent_ptr); }
    bool isChild() const                          { return (!isTopLevel()); }

    GitRepoPtr repo()                             { return m_repo_ptr; }
    GitGroupPtr parent()                          { return m_parent_ptr; }
    const std::string& name() const               { return m_name; }
    std::string fullname() const;
    std::string relPathname() const;
    std::string absPathname() const;
    std::string pathname() const                  { return relPathname(); }

    void writeToDisk();

    std::string repr(bool extended=false) const;

    bool finalize();

private:
    GitRepoPtr      m_repo_ptr;
    GitGroupPtr     m_parent_ptr;
    std::string     m_name;

    bool            m_written;
};

//-*****************************************************************************
//! Represents a Data chunk (child) in a GitGroup
class GitData : public Alembic::Util::enable_shared_from_this<GitData>
{
public:
    GitData( );                                                  // default empty data
    ~GitData();

    void write(Alembic::Util::uint64_t iSize, const void * iData);
    void write(Alembic::Util::uint64_t iNumData,
                const Alembic::Util::uint64_t * iSizes,
                const void ** iDatas);

    // rewrites over part of the already written data, does not change
    // the size of the already written data.  If what is attempting
    // to be rewritten exceeds the boundaries of what is already written,
    // the existing data will be unchanged
    void rewrite(Alembic::Util::uint64_t iSize, void * iData,
                 Alembic::Util::uint64_t iOffset=0);

    Alembic::Util::uint64_t getSize() const;

    std::string repr(bool extended=false) const;

private:
    friend class GitGroup; // friend so it can call our private constructor

    GitData(GitGroupPtr iGroup, Alembic::Util::uint64_t iPos,
          Alembic::Util::uint64_t iSize);

    Alembic::Util::uint64_t getPos() const;

    GitGroupPtr             m_group;
    Alembic::Util::uint64_t m_pos;
    Alembic::Util::uint64_t m_size;
};

} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif
