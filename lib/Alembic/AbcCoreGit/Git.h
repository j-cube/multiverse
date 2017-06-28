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

#ifndef _Alembic_AbcCoreGit_Git_h_
#define _Alembic_AbcCoreGit_Git_h_

#include <Alembic/AbcCoreGit/Foundation.h>
#include <Alembic/AbcCoreGit/Utils.h>

#include <Alembic/AbcCoreFactory/IFactory.h>

#include <iostream>
#include <sstream>

#include <set>
#include <vector>

#include <boost/operators.hpp>
#include <boost/optional.hpp>

#include <git2.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

/* --------------------------------------------------------------------
 *   MACROS
 * -------------------------------------------------------------------- */

/* --------------------------------------------------------------------
 *   UTILITY FUNCTIONS
 * -------------------------------------------------------------------- */

/* --------------------------------------------------------------------
 *   CLASSES
 * -------------------------------------------------------------------- */

/* --- forward declarations ------------------------------------------- */

class LibGit;
struct GitMode;
class GitRepo;
class GitTreebuilder;
class GitTree;

class GitGroup;
class GitData;

typedef Alembic::Util::shared_ptr<GitRepo> GitRepoPtr;
typedef Alembic::Util::shared_ptr<const GitRepo> GitRepoConstPtr;
typedef Alembic::Util::shared_ptr<GitTreebuilder> GitTreebuilderPtr;
typedef Alembic::Util::shared_ptr<GitTree> GitTreePtr;

typedef Alembic::Util::shared_ptr<GitGroup> GitGroupPtr;
typedef Alembic::Util::shared_ptr<const GitGroup> GitGroupConstPtr;

typedef Alembic::Util::shared_ptr<GitData> GitDataPtr;
typedef Alembic::Util::shared_ptr<const GitData> GitDataConstPtr;


/* --- utils ---------------------------------------------------------- */

std::string git_time_str(const git_time& intime);

/* --- LibGit --------------------------------------------------------- */

// based on Monoid pattern of Alexandrescu's "Modern C++ Design"
// see:
//   http://stackoverflow.com/questions/2496918/singleton-pattern-in-c
//   http://stackoverflow.com/a/2498423/1363486

class LibGit
{
public:
    ~LibGit();

    static Alembic::Util::shared_ptr<LibGit> Instance();

    static bool IsInitialized();
    static bool Initialize();

private:
    static void CleanUp();

    LibGit();

    static Alembic::Util::shared_ptr<LibGit> m_instance;
};


/* --- GitMode -------------------------------------------------------- */

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

    //friend inline bool operator== (const GitMode &lhs, const GitMode &rhs) { return (lhs.m_t == rhs.m_t); }
    friend std::ostream& operator<< ( std::ostream& out, const GitMode& value );

private:
    Type m_t;
    //prevent automatic conversion for any other built-in types such as bool, int, etc
    template<typename T> operator T () const;

    //prevent automatic operator==(int)
    bool operator== (int);
};

std::ostream& operator<< (std::ostream& out, const GitMode& value);


/* --- GitCommitInfo -------------------------------------------------- */

struct GitCommitInfo
{
    GitCommitInfo(const git_oid& oid, const git_commit* commit);

    std::string id;
    std::string author;
    std::string author_email;
    git_time when_time;
    std::string when;
    std::string message;
};

std::ostream& operator<< (std::ostream& out, const GitCommitInfo& value);


/* --- GitRepo -------------------------------------------------------- */

class GitRepo : public Alembic::Util::enable_shared_from_this<GitRepo>
{
public:
    static const bool DEFAULT_IGNORE_WRONG_REV = false;
    static const bool DEFAULT_MILLIWAYS_ENABLED = false;

    GitRepo(const std::string& pathname, GitMode mode = GitMode::ReadWrite, bool milliwaysEnable = DEFAULT_MILLIWAYS_ENABLED);
    GitRepo(const std::string& pathname, const Alembic::AbcCoreFactory::IOptions& options, GitMode mode = GitMode::Read, bool milliwaysEnable = DEFAULT_MILLIWAYS_ENABLED);
    // GitRepo(const std::string& pathname);
    virtual ~GitRepo();

    virtual void cleanup();

    static GitRepoPtr Create(const std::string& dotGitPathname) { return GitRepoPtr(new GitRepo(dotGitPathname)); }

    GitRepoPtr ptr()                { return shared_from_this(); }

    // WARNING: be sure to have an existing shared_ptr to the repo
    // before calling the following method!
    // (Because it uses shared_from_this()...)
    GitTreebuilderPtr root();

    // WARNING: be sure to have an existing shared_ptr to the repo
    // before calling the following method!
    // (Because it uses shared_from_this()...)
    GitTreePtr rootTree();

    std::string pathname() const    { return m_pathname; }
    const GitMode& mode() const     { return m_mode; }

    int32_t formatVersion() const;
    int32_t libVersion() const;

    /* index based API */

    bool add(const std::string& path);
    bool write_index();
    bool commit_index(const std::string& message) const;

    /* treebuilder-based API */

    bool commit_treebuilder(const std::string& message);

    /* misc */
    bool isValid() const            { return (m_repo && !error()); }
    bool isClean() const            { return false; /* [TODO]: implement git clean check */ }
    bool isFrozen() const           { return true; /* [TODO]: implement frozen check */ }

    Alembic::AbcCoreFactory::IOptions options() const { return m_options; }
    bool hasRevisionSpec() const { return (! m_revision.empty()); }
    std::string revisionString() const { return m_revision; }
    bool ignoreWrongRevision() const { return m_ignore_wrong_rev; }

    bool milliwaysEnabled() const { return m_milliways_enabled; }

    std::string relpath(const std::string& pathname_) const;

    /* groups */

    // create a top-level group from this repo
    GitGroupPtr addGroup( const std::string& name );

    GitGroupPtr rootGroup();

    /* error handling */

    bool error() const              { return m_error; }

    /* trash history */

    bool trashHistory(std::string& errorMessage, const std::string& branchName = "master");

    /* fetch history */

    std::vector<GitCommitInfo> getHistory(bool& error);
    std::string getHistoryJSON(bool& error);

    /* libgit2-level stuff */

    git_repository* g_ptr()             { return m_repo; }
    git_repository* g_ptr() const       { return m_repo; }
    git_config* g_config()              { return m_cfg; }
    git_config* g_config() const        { return m_cfg; }
    git_signature* g_signature()        { return m_sig; }
    git_signature* g_signature() const  { return m_sig; }
    git_index *g_index() const          { return m_index; }
    git_odb *g_odb() const              { return m_odb; }
    git_refdb *g_refdb() const              { return m_refdb; }

private:
    GitRepo();                                  // disable default constructor
    GitRepo(const GitRepo& other);              // disable copy constructor
//    GitRepo(git_repository* repo, git_config* config, git_signature* signature);    // private constructor

    std::string m_pathname;
    GitMode m_mode;
    git_repository* m_repo;
    git_config *m_cfg;
    git_signature *m_sig;
    git_odb *m_odb;
    git_refdb *m_refdb;
    git_index *m_index;

    git_odb_backend *m_git_backend;
    bool m_error;

    bool m_index_dirty;

    GitTreebuilderPtr m_root;
    GitTreePtr m_root_tree;
    GitGroupPtr m_root_group_ptr;

    Alembic::AbcCoreFactory::IOptions m_options;
    std::string m_revision;
    bool m_ignore_wrong_rev;
    bool m_milliways_enabled;
    std::string m_support_repo;

    bool m_cleaned_up;
};

std::ostream& operator<< (std::ostream& out, const GitRepo& repo);
inline std::ostream& operator<< (std::ostream& out, const GitRepoPtr& repoPtr) { out << *repoPtr; return out; }


/* --- GitTree -------------------------------------------------------- */

class GitTree : public Alembic::Util::enable_shared_from_this<GitTree>, private boost::totally_ordered<GitTree>
{
public:
    virtual ~GitTree();

    GitTreePtr ptr()                        { return shared_from_this(); }

    bool error()                            { return m_error; }

    GitRepoPtr repo()                       { return m_repo; }

    bool isRoot() const                     { return !(m_parent && m_parent.get()); }
    GitTreePtr root() const                 { return m_repo->rootTree(); }
    GitTreePtr parent() const               { return m_parent; }

    std::string filename() const            { return m_filename; }
    std::string pathname() const;
    std::string relPathname() const;
    std::string absPathname() const;

    // WARNING: be sure to have an existing shared_ptr to this treebuilder
    // before calling the following method!
    // (Because it uses shared_from_this()...)
    boost::optional<GitTreePtr> getChildTree(const std::string& filename);

    boost::optional<std::string> getChildFile(const std::string& filename);

    bool hasChild(const std::string& filename);
    bool hasFileChild(const std::string& filename);
    bool hasTreeChild(const std::string& filename);

    git_tree* g_ptr()                       { return m_tree; }
    git_tree* g_ptr() const                 { return m_tree; }
    git_repository* g_repo_ptr()            { return m_repo->g_ptr(); }
    git_repository* g_repo_ptr() const      { return m_repo->g_ptr(); }

    const git_oid& oid() const              { return m_tree_oid; }

    static bool cmp(const GitTree& a, const GitTree& b)
    {
        if (a.g_ptr() == b.g_ptr())
            return 0;
        if ((a.pathname() == b.pathname()) || (a.g_ptr() == b.g_ptr()))
            return 0;
        return a.pathname() < b.pathname();
    }
    bool operator< (const GitTree& rhs) const
    {
        return pathname() < rhs.pathname();
    }
    bool operator== (const GitTree& rhs) const
    {
        return GitTree::cmp(*this, rhs);
    }

    bool hasRevisionSpec() const { return (! m_revision.empty()); }
    std::string revisionString() const { return m_revision; }
    bool ignoreWrongRevision() const { return m_ignore_wrong_rev; }

    static GitTreePtr Create(GitRepoPtr repo_ptr) { return GitTreePtr(new GitTree(repo_ptr)); }
    static GitTreePtr Create(GitRepoPtr repo_ptr, const std::string& revision, bool ignoreWrongRevision = false) { return GitTreePtr(new GitTree(repo_ptr, revision, ignoreWrongRevision)); }
    static boost::optional<GitTreePtr> Create(GitTreePtr parent_ptr, const std::string& filename);

private:
    GitTree();                                   // disable default constructor
    GitTree(const GitTree& other);               // disable copy constructor
    GitTree(GitRepoPtr repo_ptr);                // private constructor
    GitTree(GitRepoPtr repo_ptr, const std::string& rev_str, bool ignoreWrongRevision = false);  // private constructor
    GitTree(GitRepoPtr repo_ptr, GitTreePtr parent_ptr, git_tree* lg_ptr);     // private constructor

    GitRepoPtr m_repo;
    GitTreePtr m_parent;
    std::string m_filename;
    git_tree* m_tree;
    git_oid m_tree_oid;                     /* the SHA1 for our corresponding tree */
    std::string m_revision;
    bool m_ignore_wrong_rev;
    bool m_error;

    friend GitTreePtr GitRepo::rootTree();
};

inline bool cmp(const GitTree& a, const GitTree& b)
{
    return GitTree::cmp(a, b);
}

inline bool cmp(const GitTreePtr& a, const GitTreePtr& b)
{
    return GitTree::cmp(*a, *b);
}

std::ostream& operator<< (std::ostream& out, const GitTree& tree);
inline std::ostream& operator<< (std::ostream& out, const GitTreePtr& treePtr) { out << *treePtr; return out; }


/* --- GitTreebuilder ------------------------------------------------- */
  
class GitTreebuilder : public Alembic::Util::enable_shared_from_this<GitTreebuilder>, private boost::totally_ordered<GitTreebuilder>
{
public:
    virtual ~GitTreebuilder();

    GitTreebuilderPtr ptr()                 { return shared_from_this(); }

    bool error()                            { return m_error; }

    GitRepoPtr repo()                       { return m_repo; }

    bool isRoot() const                     { return !(m_parent && m_parent.get()); }
    GitTreebuilderPtr root() const          { return m_repo->root(); }
    GitTreebuilderPtr parent() const        { return m_parent; }
    std::string filename() const            { return m_filename; }
    std::string pathname() const;
    std::string relPathname() const;
    std::string absPathname() const;

    GitTreebuilderPtr getChild(const std::string& filename);

    git_treebuilder* g_ptr()                { return m_tree_bld; }
    git_treebuilder* g_ptr() const          { return m_tree_bld; }
    git_repository* g_repo_ptr()            { return m_repo->g_ptr(); }
    git_repository* g_repo_ptr() const      { return m_repo->g_ptr(); }

    const git_oid& oid() const              { return m_tree_oid; }
    git_tree* tree()                        { return m_tree; }
    git_tree* tree() const                  { return m_tree; }

    virtual bool write();
    virtual bool add_file_from_memory(const std::string& filename, const std::string& contents);

    // WARNING: be sure to have an existing shared_ptr to this treebuilder
    // before calling the following method!
    // (Because it uses shared_from_this()...)
    virtual GitTreebuilderPtr add_tree(const std::string& filename);

    virtual bool dirty() const;

    static bool cmp(const GitTreebuilder& a, const GitTreebuilder& b)
    {
        if ((a.pathname() == b.pathname()) && (a.g_ptr() == b.g_ptr()))
            return 0;
        return a.pathname() < b.pathname();
    }
    bool operator< (const GitTreebuilder& rhs) const
    {
        return pathname() < rhs.pathname();
    }
    bool operator== (const GitTreebuilder& rhs) const
    {
        return GitTreebuilder::cmp(*this, rhs);
    }

private:
    GitTreebuilder();                                   // disable default constructor
    GitTreebuilder(const GitTreebuilder& other);        // disable copy constructor
    GitTreebuilder(GitRepoPtr repo_ptr);                // private constructor

    static GitTreebuilderPtr Create(GitRepoPtr repo_ptr) { return GitTreebuilderPtr(new GitTreebuilder(repo_ptr)); }
    static GitTreebuilderPtr Create(GitTreebuilderPtr parent, const std::string& filename);

    bool _add_subtree(GitTreebuilderPtr subtreePtr);
    bool _write_subtree_and_insert(GitTreebuilderPtr child);

    void _set_parent(GitTreebuilderPtr parent);
    void _set_filename(const std::string& filename);

    GitRepoPtr m_repo;
    GitTreebuilderPtr m_parent;
    std::string m_filename;
    git_treebuilder* m_tree_bld;
    git_oid m_tree_oid;                     /* the SHA1 for our corresponding tree */
    git_tree *m_tree;                       /* our tree */
    bool m_written;
    bool m_dirty;
    bool m_error;

    std::set<GitTreebuilderPtr> m_children_set;
    std::vector<GitTreebuilderPtr> m_children;

    friend GitTreebuilderPtr GitRepo::root();
};

inline bool cmp(const GitTreebuilder& a, const GitTreebuilder& b)
{
    return GitTreebuilder::cmp(a, b);
}

inline bool cmp(const GitTreebuilderPtr& a, const GitTreebuilderPtr& b)
{
    return GitTreebuilder::cmp(*a, *b);
}

std::ostream& operator<< (std::ostream& out, const GitTreebuilder& treeBld);
inline std::ostream& operator<< (std::ostream& out, const GitTreebuilderPtr& treeBldPtr) { out << *treeBldPtr; return out; }


/* --- GitObject ------------------------------------------------------ */

class GitObject : public Alembic::Util::enable_shared_from_this<GitObject>
{
public:
    GitObject( GitRepoPtr repo, git_oid oid );
    GitObject( GitRepoPtr repo, const std::string& oid_hex );
    virtual ~GitObject();

    GitRepoPtr repo()                             { return m_repo_ptr; }
    GitRepoConstPtr repo() const                  { return m_repo_ptr; }
    const GitMode& mode() const                   { return repo()->mode(); }

    git_repository *g_repository() const          { return repo()->g_ptr(); }

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


/* --- GitGroup ------------------------------------------------------- */

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

    /* tree API (R) */

    GitTreePtr tree();
    bool hasGitTree();

    /* treebuilder API (W) */

    GitTreebuilderPtr treebuilder();

    bool add_file_from_memory(const std::string& filename, const std::string& contents);

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
    GitGroupPtr root()                            { return repo()->rootGroup(); }
    GitRepoConstPtr repo() const                  { return m_repo_ptr; }
    const GitMode& mode() const                   { return repo()->mode(); }
    GitGroupPtr parent() const                    { return m_parent_ptr; }
    const std::string& name() const               { return m_name; }
    std::string fullname() const;
    std::string filename() const                  { return name(); }
    std::string relPathname() const;
    std::string absPathname() const;
    std::string pathname() const                  { return relPathname(); }

    bool hasRevisionSpec() const                  { return m_repo_ptr->hasRevisionSpec(); }
    std::string revisionString() const            { return m_repo_ptr->revisionString(); }
    bool ignoreWrongRevision() const              { return m_repo_ptr->ignoreWrongRevision(); }

    void writeToDisk();
    bool readFromDisk();

    std::string repr(bool extended=false) const;

    bool finalize();

private:
    friend GitGroupPtr GitRepo::addGroup( const std::string& name );

    void _set_treebld(GitTreebuilderPtr treebld_ptr);

    GitRepoPtr      m_repo_ptr;
    GitGroupPtr     m_parent_ptr;
    std::string     m_name;

    GitTreebuilderPtr m_treebld_ptr;
    GitTreePtr        m_tree_ptr;

    bool            m_written;
    bool            m_read;
};

std::ostream& operator<< (std::ostream& out, const GitGroup& group);
inline std::ostream& operator<< (std::ostream& out, const GitGroupPtr& groupPtr) { out << *groupPtr; return out; }


/* --- GitData -------------------------------------------------------- */

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
