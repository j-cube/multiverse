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
#include <fstream>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <cassert>

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

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

/* --------------------------------------------------------------------
 *   PROTOTYPES
 * -------------------------------------------------------------------- */

static bool git_check_error(int error_code, const std::string& action);
static bool git_check_ok(int error_code, const std::string& action);

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

static bool git_check_error(int error_code, const std::string& action)
{
    if (error_code == GIT_SUCCESS)
       return false;

    const git_error *error = giterr_last();

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

    ABCA_THROW( "libgit2 error " << error_code << " " << action <<
        " - " << ((error && error->message) ? error->message : "???") );
    // std::cerr << "ERROR: libgit2 error " << error_code << " - "
    //     << action << " - "
    //     << ((error && error->message) ? error->message : "???")
    //     << std::endl;
    return false;
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


/* --- GitRepo -------------------------------------------------------- */

#if 0
GitRepo::GitRepo(git_repository* repo, git_config* config, git_signature* signature) :
    m_repo(repo), m_mode(GitMode::ReadWrite), m_cfg(config), m_sig(signature),
    m_odb(NULL), m_index(NULL), m_error(false)
{
    LibGit::Initialize();
}
#endif

GitRepo::GitRepo(const std::string& pathname_, GitMode mode_) :
    m_pathname(pathname_), m_mode(mode_), m_repo(NULL), m_cfg(NULL), m_sig(NULL),
    m_odb(NULL), m_index(NULL), m_error(false)
{
    TRACE("GitRepo::GitRepo(pathname:'" << pathname_ << "' mode:" << mode_ << ")");

    LibGit::Initialize();

    bool ok = true;
    int rc;

    std::string dotGitPathname = pathjoin(pathname(), ".git");

    if ((m_mode == GitMode::Write) || (m_mode == GitMode::ReadWrite))
    {
        if (! isdir(m_pathname))
        {
            TRACE("initializing Git repository '" << m_pathname << "'");
            rc = git_repository_init(&m_repo, m_pathname.c_str(), /* is_bare */ 0);
            ok = ok && git_check_ok(rc, "initializing git repository");
            if (! ok)
            {
                // ABCA_THROW( "Could not open (initialize) file: " << m_pathname << " (git repo)");
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
            TRACE("Git repository '" << m_pathname << "' exists, opening (for writing)");
        }
    }

    assert(! m_repo);

    rc = git_repository_open(&m_repo, dotGitPathname.c_str());
    ok = ok && git_check_ok(rc, "opening git repository");
    if (!ok)
    {
        TRACE("error opening repository from '" << dotGitPathname << "'");
        //ABCA_THROW( "Could not open file: " << m_pathname << " (git repo)");
        goto ret;
    }

    assert(m_repo);

    rc = git_repository_config(&m_cfg, m_repo /* , NULL, NULL */);
    ok = ok && git_check_ok(rc, "opening git config");
    if (!ok)
    {
        TRACE("error fetching config for repository '" << dotGitPathname << "'");
        //ABCA_THROW( "Could not get Git configuration for repository: " << m_pathname);
        goto ret;
    }
    assert(m_cfg);

    rc = git_signature_default(&m_sig, m_repo);
    ok = ok && git_check_ok(rc, "getting default signature");
    if (! ok) goto ret;

    assert(m_sig);

    rc = git_repository_odb(&m_odb, m_repo);
    ok = ok && git_check_ok(rc, "accessing git repository database");
    if (!ok)
    {
        goto ret;
    }

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
        if (m_odb)
            git_odb_free(m_odb);
        m_odb = NULL;
        if (m_cfg)
            git_config_free(m_cfg);
        m_cfg = NULL;
        if (m_repo)
            git_repository_free(m_repo);
        m_repo = NULL;
    }
}

GitRepo::~GitRepo()
{
    if (m_index)
        git_index_free(m_index);
    m_index = NULL;
    if (m_odb)
        git_odb_free(m_odb);
    m_odb = NULL;
    if (m_sig)
        git_signature_free(m_sig);
    m_sig = NULL;
    if (m_cfg)
        git_config_free(m_cfg);
    m_cfg = NULL;
    if (m_repo)
        git_repository_free(m_repo);
    m_repo = NULL;
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
    git_check_error(rc, "getting alembic.formatversion config value");

    return formatversion;
}

int32_t GitRepo::libVersion() const
{
    int rc;

    int32_t libversion = -1;

    rc = git_config_get_int32(&libversion, g_config(), "alembic.libversion");
    git_check_error(rc, "getting alembic.libversion config value");

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

std::ostream& operator<< (std::ostream& out, const GitRepo& repo)
{
    out << "<GitRepo path:'" << repo.pathname() << "' mode:" << repo.mode() << ">";
    return out;
}


/* --- GitTree -------------------------------------------------------- */

GitTree::GitTree(GitRepoPtr repo) :
    m_repo(repo), m_filename("/"), m_tree(NULL), m_error(false)
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

    rc = git_commit_tree(&m_tree, commit);
    m_error = m_error || git_check_error(rc, "getting commit tree");

    if (m_tree)
        git_oid_cpy(&m_tree_oid, git_tree_id(m_tree));
}

GitTree::GitTree(GitRepoPtr repo, std::string rev_str) :
    m_repo(repo), m_filename("/"), m_tree(NULL), m_error(false)
{
    git_object* obj = NULL;

    memset(m_tree_oid.id, 0, GIT_OID_RAWSZ);

    int rc = git_revparse_single(&obj, repo->g_ptr(), rev_str.c_str());
    m_error = m_error || git_check_error(rc, "parsing revision string");

    rc = git_tree_lookup(&m_tree, repo->g_ptr(), git_object_id(obj));
    m_error = m_error || git_check_error(rc, "looking up tree");

    git_object_free(obj);
    obj = NULL;

    if (m_tree)
        git_oid_cpy(&m_tree_oid, git_tree_id(m_tree));
}

GitTree::GitTree(GitRepoPtr repo, GitTreePtr parent_ptr, git_tree* lg_ptr) :
    m_repo(repo), m_parent(parent_ptr), m_tree(NULL), m_error(false)
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
            m_tree_ptr = GitTree::Create(repo());
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
