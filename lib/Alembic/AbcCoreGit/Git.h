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

#include <iostream>
#include <sstream>

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

// enum-like functionality in a struct (http://stackoverflow.com/a/2506286)
struct GitMode
{
    enum Type
    {
        Read, Write, ReadWrite
    };

    GitMode(Type t) : m_t(t) {}
    GitMode(const GitMode& other) : m_t(other.m_t) {}
    GitMode& operator= (const GitMode &rhs) { m_t = rhs.m_t; return *this; }
    bool operator== (const GitMode &rhs) { return (m_t == rhs.m_t); }
    bool operator== (const Type &rhs) { return (m_t == rhs); }
    operator Type () const { return m_t; }

    std::string repr(bool extended = false) const { std::ostringstream ss; ss << *this; return ss.str(); }

    //friend inline bool operator== (const GitMode &lhs, const GitMode &rhs) { return (lhs.m_t == rhs.m_t); }
    friend std::ostream& operator<< ( std::ostream& out, const GitMode& value );

private:
    Type m_t;
    //prevent automatic conversion for any other built-in types such as bool, int, etc
    template<typename T> operator T () const;

    //prevent automatic operator==(int)
    bool operator== (int);
};

//-*****************************************************************************
class GitRepo : public Alembic::Util::enable_shared_from_this<GitRepo>
{
public:
    GitRepo( const std::string& pathname, GitMode mode );
    GitRepo( const std::string& pathname, git_repository *repo, git_config *cfg );
    virtual ~GitRepo();

    const GitMode& mode() const { return m_mode; }

    int32_t formatVersion() const;
    int32_t libVersion() const;

    bool isValid() const;

    bool isClean() const;

    bool isFrozen() const;      // frozen means not correctly and completely written

    bool add(const std::string& path);
    bool write_index();
    bool commit(const std::string& message) const;

    std::string relpath(const std::string& pathname) const;

    // create a top-level group from this repo
    GitGroupPtr addGroup( const std::string& name );

    GitGroupPtr rootGroup();

    const std::string& pathname() const           { return m_pathname; }
    git_repository *g_repository() const          { return m_repo; }
    git_index *g_index() const                    { return m_index; }

    git_config *g_config() const                  { return m_repo_cfg; }

    git_odb *g_odb() const                        { return m_odb; }

    std::string repr(bool extended=false) const;

    friend std::ostream& operator<< ( std::ostream& out, const GitRepo& value );

private:
    bool lg2_check_error(int error_code, const std::string& action) const;

    GitMode         m_mode;
    std::string     m_pathname;
    git_repository  *m_repo;
    git_config      *m_repo_cfg;
    git_odb         *m_odb;
    git_index       *m_index;
    bool            m_index_dirty;

    GitGroupPtr     m_root_group_ptr;
};

typedef Alembic::Util::shared_ptr<GitRepo> GitRepoPtr;
typedef Alembic::Util::shared_ptr<const GitRepo> GitRepoConstPtr;


inline std::ostream& operator<< ( std::ostream& out, const GitRepo& value )
{
    out << value.repr();
    return out;
}

//-*****************************************************************************
class GitObject : public Alembic::Util::enable_shared_from_this<GitObject>
{
public:
    GitObject( GitRepoPtr repo, git_oid oid );
    GitObject( GitRepoPtr repo, const std::string& oid_hex );
    virtual ~GitObject();

    GitRepoPtr repo()                             { return m_repo_ptr; }
    GitRepoConstPtr repo() const                  { return m_repo_ptr; }
    const GitMode& mode() const                   { return repo()->mode(); }

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
    GitRepoConstPtr repo() const                  { return m_repo_ptr; }
    const GitMode& mode() const                   { return repo()->mode(); }
    GitGroupPtr parent()                          { return m_parent_ptr; }
    const std::string& name() const               { return m_name; }
    std::string fullname() const;
    std::string relPathname() const;
    std::string absPathname() const;
    std::string pathname() const                  { return relPathname(); }

    void writeToDisk();
    bool readFromDisk();

    std::string repr(bool extended=false) const;

    bool finalize();

private:
    GitRepoPtr      m_repo_ptr;
    GitGroupPtr     m_parent_ptr;
    std::string     m_name;

    bool            m_written;
    bool            m_read;
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
