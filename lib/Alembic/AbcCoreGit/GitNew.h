#ifndef W_GIT_H
#define W_GIT_H

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <cstdlib>

#include <errno.h>

#include <stdio.h>
#include <stdlib.h>

#include <set>
#include <vector>

#include <boost/config.hpp>
//#include <boost/tr1/memory.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/operators.hpp>

#include <git2.h>

namespace Git {


/* --------------------------------------------------------------------
 *   MACROS
 * -------------------------------------------------------------------- */

#ifndef GIT_SUCCESS
#define GIT_SUCCESS 0
#endif /* GIT_SUCCESS */

/* --------------------------------------------------------------------
 *   UTILITY FUNCTIONS
 * -------------------------------------------------------------------- */

/* --------------------------------------------------------------------
 *   CLASSES
 * -------------------------------------------------------------------- */

/* --- forward declarations ------------------------------------------- */

struct GitMode;
class GitRepo;
class GitTreebuilder;

typedef boost::shared_ptr<GitRepo> GitRepoPtr;
typedef boost::shared_ptr<GitTreebuilder> GitTreebuilderPtr;

/* --- LibGit --------------------------------------------------------- */

// based on Monoid pattern of Alexandrescu's "Modern C++ Design"
// see:
//   http://stackoverflow.com/questions/2496918/singleton-pattern-in-c
//   http://stackoverflow.com/a/2498423/1363486

class LibGit
{
public:
	~LibGit();

	static boost::shared_ptr<LibGit> Instance();

	static bool IsInitialized();
	static bool Initialize();

private:
	static void CleanUp();

	LibGit();

	static boost::shared_ptr<LibGit> m_instance;
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


/* --- GitRepo -------------------------------------------------------- */

class GitRepo : public boost::enable_shared_from_this<GitRepo>
{
public:
    GitRepo(const std::string& pathname, GitMode mode = GitMode::ReadWrite);
	// GitRepo(const std::string& pathname);
	virtual ~GitRepo();

	static GitRepoPtr Create(const std::string& dotGitPathname) { return GitRepoPtr(new GitRepo(dotGitPathname)); }

	GitRepoPtr ptr() 				{ return shared_from_this(); }

	// WARNING: be sure to have an existing shared_ptr to the repo
	// before calling the following method!
	// (Because it uses shared_from_this()...)
	GitTreebuilderPtr root();

	std::string pathname() const	{ return m_pathname; }
    const GitMode& mode() const 	{ return m_mode; }

	bool error()					{ return m_error; }
	git_repository* g_ptr()			{ return m_repo; }

	git_config* g_config()			{ return m_cfg; }
	git_signature* g_signature()	{ return m_sig; }
    git_index *g_index() const      { return m_index; }
    git_odb *g_odb() const          { return m_odb; }

private:
	GitRepo();									// disable default constructor
	GitRepo(const GitRepo& other);				// disable copy constructor
	GitRepo(git_repository* repo, git_config* config, git_signature* signature);	// private constructor

	std::string m_pathname;
	GitMode m_mode;
	git_repository* m_repo;
    git_config *m_cfg;
    git_signature *m_sig;
    git_odb *m_odb;
    git_index *m_index;
	bool m_error;

    bool m_index_dirty;

	GitTreebuilderPtr m_root;
};

std::ostream& operator<< (std::ostream& out, const GitRepo& repo);
inline std::ostream& operator<< (std::ostream& out, const GitRepoPtr& repoPtr) { out << *repoPtr; return out; }

/* --- GitTreebuilder ------------------------------------------------- */
  
class GitTreebuilder : public boost::enable_shared_from_this<GitTreebuilder>, private boost::totally_ordered<GitTreebuilder>
{
public:
	virtual ~GitTreebuilder();

	GitTreebuilderPtr ptr()					{ return shared_from_this(); }

	bool error()							{ return m_error; }

	GitRepoPtr repo()						{ return m_repo; }

	bool isRoot() const                     { return !(m_parent && m_parent.get()); }
	GitTreebuilderPtr root() const			{ return m_repo->root(); }
	GitTreebuilderPtr parent() const		{ return m_parent; }
	std::string filename() const			{ return m_filename; }
	std::string pathname() const;
	std::string relPathname() const;
	std::string absPathname() const;

	GitTreebuilderPtr getChild(const std::string& filename);

	git_treebuilder* g_ptr()				{ return m_tree_bld; }
	git_treebuilder* g_ptr() const			{ return m_tree_bld; }
	git_repository* g_repo_ptr()			{ return m_repo->g_ptr(); }
	git_repository* g_repo_ptr() const		{ return m_repo->g_ptr(); }

	const git_oid& oid() const      		{ return m_tree_oid; }
	git_tree* tree()						{ return m_tree; }
	git_tree* tree() const					{ return m_tree; }

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
	GitTreebuilder();									// disable default constructor
	GitTreebuilder(const GitTreebuilder& other);		// disable copy constructor
	GitTreebuilder(GitRepoPtr repo_ptr);				// private constructor

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
	git_oid m_tree_oid;						/* the SHA1 for our corresponding tree */
	git_tree *m_tree;						/* our tree */
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



} /* end of namespace Git */

#endif /* W_GIT_H */
