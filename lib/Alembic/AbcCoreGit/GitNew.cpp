/*****************************************************************************/
/*  Copyright (c) 2013-2015 J CUBE I. Tokyo, Japan. All Rights Reserved.     */
/*                                                                           */
/*  This program is free software; you can redistribute it and/or modify     */
/*  it under the terms of the GNU General Public License as published by     */
/*  the Free Software Foundation; either version 2 of the License, or        */
/*  (at your option) any later version.                                      */
/*                                                                           */
/*  This program is distributed in the hope that it will be useful,          */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of           */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            */
/*  GNU General Public License for more details.                             */
/*                                                                           */
/*  You should have received a copy of the GNU General Public License along  */
/*  with this program; if not, write to the Free Software Foundation, Inc.,  */
/*  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.              */
/*                                                                           */
/*  J CUBE Inc.                                                              */
/*  6F Azabu Green Terrace                                                   */
/*  3-20-1 Minami-Azabu, Minato-ku, Tokyo                                    */
/*  info@-jcube.jp                                                           */
/*****************************************************************************/

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <cassert>

#include <errno.h>

#include <stdio.h>
#include <stdlib.h>

#include <boost/system/system_error.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/current_function.hpp>

#include <git2.h>

#include "Git.h"

namespace Git {


/* --------------------------------------------------------------------
 *   MACROS
 * -------------------------------------------------------------------- */

/* --------------------------------------------------------------------
 *   PROTOTYPES
 * -------------------------------------------------------------------- */

static bool git_check_error(int error_code, const std::string& action);
static bool git_check_ok(int error_code, const std::string& action);

static std::string slurp(const std::string& pathname);
static std::string slurp(std::ifstream& in);

/* --------------------------------------------------------------------
 *   UTILITY FUNCTIONS
 * -------------------------------------------------------------------- */

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

    assert(pathname1.length() > 0);
    assert(pathname2.length() > 0);

    boost::filesystem::path p1(pathname1);
    boost::filesystem::path p2(pathname2);

    return (p1 / p2).native();
}

// "virtual" pathnames
std::string v_pathjoin(const std::string& p1, const std::string& p2, char pathsep)
{
    if (p1.length() == 0)
        return p2;
    if (p2.length() == 0)
        return p1;

    assert(p1.length() > 0);
    assert(p2.length() > 0);

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
    boost::filesystem::path p_slash("/");
    boost::filesystem::path::string_type preferredSlash = p_slash.make_preferred().native();
    return preferredSlash;
}

bool ends_with_separator(const std::string& pathname)
{
    std::string sep = path_separator();

    return ((pathname.size() >= sep.size()) &&
            (pathname.compare(pathname.size() - sep.size(), sep.size(), sep) == 0));
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
    return true;
}

static bool git_check_ok(int error_code, const std::string& action)
{
	if (error_code == GIT_SUCCESS)
	   return true;

	const git_error *error = giterr_last();

	std::cerr << "ERROR: libgit2 error " << error_code << " - "
		<< action << " - "
		<< ((error && error->message) ? error->message : "???")
		<< std::endl;
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

boost::shared_ptr<LibGit> LibGit::m_instance = boost::shared_ptr<LibGit>(static_cast<LibGit*>(NULL));

LibGit::LibGit()
{
	git_libgit2_init();
}

LibGit::~LibGit()
{
	git_libgit2_shutdown();
}

boost::shared_ptr<LibGit> LibGit::Instance()
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
		boost::shared_ptr<LibGit> ptr = Instance();
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

GitRepo::GitRepo(git_repository* repo, git_config* config, git_signature* signature) :
	m_repo(repo), m_mode(GitMode::ReadWrite), m_cfg(config), m_sig(signature),
	m_odb(NULL), m_index(NULL), m_error(false)
{
	LibGit::Initialize();
}

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

#if 0
GitRepo::GitRepo(const std::string& pathname_) :
	m_pathname(pathname_), m_mode(GitMode::ReadWrite), m_repo(NULL), m_cfg(NULL), m_sig(NULL),
	m_odb(NULL), m_index(NULL), m_error(false)
{
	bool ok = true;
	int rc;

	std::string dotGitPathname = pathjoin(pathname(), ".git");
    rc = git_repository_open(&m_repo, dotGitPathname.c_str());
	ok = ok && git_check_ok(rc, "opening git repository");
	if (! ok) goto ret;

	assert(m_repo);

    rc = git_repository_config(&m_cfg, m_repo /* , NULL, NULL */);
	ok = ok && git_check_ok(rc, "opening git config");
	if (! ok) goto ret;

	assert(m_cfg);

    rc = git_signature_default(&m_sig, m_repo);
   	ok = ok && git_check_ok(rc, "getting default signature");
	if (! ok) goto ret;

	assert(m_sig);

ret:
	m_error = m_error || (!ok);
	if (m_error)
	{
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
}
#endif

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

GitTreebuilderPtr GitRepo::root()
{
	if (m_root && m_root.get())
		return m_root;

	// GitRepoPtr thisPtr(this);

	m_root = GitTreebuilder::Create(ptr());
	m_root->_set_filename("/");
	return m_root;
}

std::ostream& operator<< (std::ostream& out, const GitRepo& repo)
{
    out << "<GitRepo path:'" << repo.pathname() << "'>";
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
	git_oid oid_blob;						/* the SHA1 for our blob in the tree */
	git_blob *blob = NULL;					/* our blob in the tree */
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

} /* end of namespace Git */
