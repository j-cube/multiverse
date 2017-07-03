/*
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * In addition to the permissions in the GNU General Public License,
 * the authors give you unlimited permission to link the compiled
 * version of this file into combinations with other programs,
 * and to distribute those combinations without any restriction
 * coming from the use of this file.  (The General Public License
 * restrictions do apply in other respects; for example, they cover
 * modification of the file, and distribution when not linked into
 * a combined executable.)
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <git2.h>
#include <git2/errors.h>
#include <git2/sys/repository.h>
#include <git2/sys/odb_backend.h>
#include <git2/sys/refdb_backend.h>
#include <git2/sys/refs.h>
#include <git2/odb_backend.h>

#include "git-milliways.h"

#include "milliways/Seriously.h"
#include "milliways/KeyValueStore.h"

#include "milliways/Utils.h"
#include "Utils.h"
#include <algorithm>

#ifndef GIT_SUCCESS
#define GIT_SUCCESS 0
#endif /* GIT_SUCCESS */

#ifndef GIT_ENOMEM
#define GIT_ENOMEM GIT_ERROR
#endif

// #ifndef GIT_ENOTFOUND
// #define GIT_ENOTFOUND -1
// #endif

#define TRACE_MW 0

/* ----------------------------------------------------------------- */
/*   LOCAL PROTOTYPES                                                */
/* ----------------------------------------------------------------- */

extern "C" {

	/* odb backend */
static int milliways_backend__read_header(size_t *len_p, git_otype *type_p, git_odb_backend *backend_, const git_oid *oid);
static int milliways_backend__read(void **data_p, size_t *len_p, git_otype *type_p, git_odb_backend *backend_, const git_oid *oid);
static int milliways_backend__exists(git_odb_backend *backend_, const git_oid *oid);
static int milliways_backend__write(git_odb_backend *backend_, const git_oid *oid_, const void *data, size_t len, git_otype type);

	/* refdb backend */
static int milliways_refdb_backend__exists(int *exists, git_refdb_backend *backend_, const char *ref_name);
static int milliways_refdb_backend__lookup(git_reference **out, git_refdb_backend *backend_, const char *ref_name);
static int milliways_refdb_backend__iterator(git_reference_iterator **iter_, struct git_refdb_backend *backend_, const char *glob);
static int milliways_refdb_backend__iterator_next(git_reference **ref, git_reference_iterator *iter_);
static int milliways_refdb_backend__iterator_next_name(const char **ref_name, git_reference_iterator *iter_);
static void milliways_refdb_backend__iterator_free(git_reference_iterator *iter_);
static int milliways_refdb_backend__write(git_refdb_backend *backend_, const git_reference *ref, int force, const git_signature *who,
	const char *message, const git_oid *old, const char *old_target);
static int milliways_refdb_backend__rename(git_reference **out, git_refdb_backend *backend_, const char *old_name,
	const char *new_name, int force, const git_signature *who, const char *message);
static int milliways_refdb_backend__del(git_refdb_backend *backend_, const char *ref_name, const git_oid *old, const char *old_target);
static void milliways_refdb_backend__free(git_refdb_backend *backend_);

	/* refdb backend - reflog */
static int milliways_refdb_backend__has_log(git_refdb_backend *backend_, const char *refname);
static int milliways_refdb_backend__ensure_log(git_refdb_backend *backend_, const char *refname);
static int milliways_refdb_backend__reflog_read(git_reflog **out, git_refdb_backend *backend_, const char *name);
static int milliways_refdb_backend__reflog_write(git_refdb_backend *backend_, git_reflog *reflog);
static int milliways_refdb_backend__reflog_rename(git_refdb_backend *backend_, const char *old_name, const char *new_name);
static int milliways_refdb_backend__reflog_delete(git_refdb_backend *backend_, const char *name);

} /* extern "C" */

/* ----------------------------------------------------------------- */
/*   CODE                                                            */
/* ----------------------------------------------------------------- */

struct cache_el
{
	git_oid oid;
	git_otype type;
	size_t len;
	char *data;
	bool present;

	cache_el() :
		oid(), type(), len(0), data(NULL), present(false), mem(NULL), mem_size(0) {}
	~cache_el() {
		if (mem)
			delete[] mem;
		mem = NULL;
		mem_size = 0;
		data = NULL;
		len = 0;
		present = false;
	}

	bool resize(size_t needed) {
		if (needed == 0)
			return true;
		if (mem_size >= needed) {
			data = mem;
			return true;
		}
		assert(mem_size < needed);
		if (mem)
			delete[] mem;
		mem = NULL;
		mem_size = 0;
		data = NULL;
		mem = new char[needed + 1];
		if (! mem)
			return false;
		mem_size = needed;
		data = mem;
		return true;
	}

	bool matches(const git_oid *oid_) const {
		if (! oid_)
			return false;
		return (memcmp(oid.id, oid_->id, 20) == 0) ? true : false;
	}

	void invalidate() {
		memset(oid.id, 0, 20);
		len = 0;
	}

	bool set(const git_oid *oid_, git_otype type_, size_t len_, const void *data_) {
		if (oid_)
			oid = (const git_oid) *oid_;
		else {
			invalidate();
			return false;
		}
		present = true;
		type = type_;
		len = len_;
		if (! resize(len_)) {
			data = NULL;
			return false;
		}
		if (data && (len_ > 0)) {
			assert(data);
			memcpy(data, data_, len_);
		}
		return true;
	}

private:
	char *mem;
	size_t mem_size;
};

static int notified_first_write = 0;

typedef milliways::KeyValueStore kv_store_t;
typedef XTYPENAME kv_store_t::block_storage_type kv_blockstorage_t;
typedef XTYPENAME kv_store_t::Search kv_search_t;
typedef XTYPENAME kv_store_t::iterator kv_iterator_t;
typedef XTYPENAME kv_store_t::glob_iterator kv_glob_iterator_t;

static cache_el cached;

struct milliways_backend
{
public:
	git_odb_backend parent;
	git_refdb_backend parent_refdb;
	kv_blockstorage_t* bs;
	kv_store_t* kv;
	bool init;
	bool cleaned;
	bool is_open;
	std::string m_pathname;
	int m_refcnt;

	milliways_backend() :
		bs(NULL), kv(NULL), init(false), cleaned(false), is_open(false), m_pathname(), m_refcnt(0) { memset(&parent, 0, sizeof(git_odb_backend)); memset(&parent_refdb, 0, sizeof(git_refdb_backend)); }
	~milliways_backend();

	int refcnt() const { return m_refcnt; }
	int incref() { ++m_refcnt; return m_refcnt; }
	int decref() { if (m_refcnt > 0) --m_refcnt; return m_refcnt; }

	bool cleanedup() const { return cleaned; }

	const std::string& pathname() const { return m_pathname; }

	bool open(const std::string& pathname_);
	bool isOpen() const { return is_open; }
	void cleanup();

	git_odb_backend   *odb_backend() { return &parent; }
	git_refdb_backend *refdb_backend() { return &parent_refdb; }

	static struct milliways_backend* GetInstance(const std::string& pathname);
	static struct milliways_backend* FromOdb(git_odb_backend* ptr);
	static struct milliways_backend* FromRefdb(const git_refdb_backend* ptr);
	static std::map< std::string, struct milliways_backend* > s_instances;
	static std::map< const git_refdb_backend*, struct milliways_backend* > s_refdb_to_master;
};

struct milliways_refdb_iterator
{
	milliways_refdb_iterator(kv_glob_iterator_t& it, milliways_backend* backend_) :
		parent(), iterator(NULL), backend(backend_)
	{
		iterator = new kv_glob_iterator_t(it);
		parent.next = &milliways_refdb_backend__iterator_next;
		parent.next_name = &milliways_refdb_backend__iterator_next_name;
		parent.free = &milliways_refdb_backend__iterator_free;
	}
	~milliways_refdb_iterator() {
		if (iterator) delete iterator;
		iterator = NULL;
	}

	git_odb_backend   *odb_backend() { return backend->odb_backend(); }
	git_refdb_backend *refdb_backend() { return backend->refdb_backend(); }

	/* members */
	git_reference_iterator parent;

	kv_glob_iterator_t* iterator;

	milliways_backend* backend;
};


std::map< std::string, struct milliways_backend* > milliways_backend::s_instances;
std::map< const git_refdb_backend*, struct milliways_backend* > milliways_backend::s_refdb_to_master;

milliways_backend::~milliways_backend()
{
	if (init) cleanup();
	cleaned = true;
	is_open = false;

	typedef std::map< const git_refdb_backend*, struct milliways_backend* > r2m_map_t;
	typedef r2m_map_t::iterator r2m_it_t;

	std::vector<r2m_it_t> itrs;
	for (r2m_it_t it = s_refdb_to_master.begin(); it != s_refdb_to_master.end(); ++it) {
		milliways_backend* b_ptr = it->second;
		if (b_ptr == this)
			itrs.push_back(it);
	}

	for (std::vector<r2m_it_t>::size_type i = 0; i < itrs.size(); ++ i) {
		s_refdb_to_master.erase(itrs[i]);
	}

	// if (milliways_backend::s_refdb_to_master.count(this) != 0)
	// 	milliways_backend::s_refdb_to_master.erase(this);
	if (milliways_backend::s_instances.count(pathname()) != 0)
		milliways_backend::s_instances.erase(pathname());
}

struct milliways_backend* milliways_backend::GetInstance(const std::string& pathname)
{
	if (s_instances.count(pathname) != 0)
		return s_instances[pathname];

	milliways_backend* backend = new milliways_backend();
	if (! backend)
		return NULL;
	if (! backend->open(pathname))
		return NULL;
	assert(backend->isOpen());

	s_instances[pathname] = backend;
	s_refdb_to_master[backend->refdb_backend()] = backend;

	return backend;
}

struct milliways_backend* milliways_backend::FromOdb(git_odb_backend* ptr)
{
	return reinterpret_cast<milliways_backend*>(ptr);
}

struct milliways_backend* milliways_backend::FromRefdb(const git_refdb_backend* ptr)
{
	if (s_refdb_to_master.count(ptr) != 0)
		return s_refdb_to_master[ptr];
	return NULL;
}

bool milliways_backend::open(const std::string& pathname_)
{
	TRACE("milliways_backend::open()") ;

	assert(! is_open);
	assert(! bs);
	assert(! kv);
	assert(! init);
	init = true;

	bs = new kv_blockstorage_t(pathname_);
	if (! bs)
		goto do_cleanup;
	assert(bs);

	kv = new kv_store_t(bs);
	if (kv == NULL)
		goto do_cleanup;
	assert(kv);

	kv->open();

	parent.version = 1;
	parent.read = &milliways_backend__read;
	parent.read_header = &milliways_backend__read_header;
	parent.write = &milliways_backend__write;
	parent.exists = &milliways_backend__exists;
	parent.free = &milliways_backend__free;

	parent_refdb.exists = &milliways_refdb_backend__exists;
	parent_refdb.lookup = &milliways_refdb_backend__lookup;
	parent_refdb.iterator = &milliways_refdb_backend__iterator;
	parent_refdb.write = &milliways_refdb_backend__write;
	parent_refdb.del = &milliways_refdb_backend__del;
	parent_refdb.rename = &milliways_refdb_backend__rename;
	parent_refdb.compress = NULL;
	parent_refdb.free = &milliways_refdb_backend__free;

	parent_refdb.has_log = &milliways_refdb_backend__has_log;
	parent_refdb.ensure_log = &milliways_refdb_backend__ensure_log;
	parent_refdb.reflog_read = &milliways_refdb_backend__reflog_read;
	parent_refdb.reflog_write = &milliways_refdb_backend__reflog_write;
	parent_refdb.reflog_rename = &milliways_refdb_backend__reflog_rename;
	parent_refdb.reflog_delete = &milliways_refdb_backend__reflog_delete;

	m_pathname = pathname_;
	is_open = true;
	return true;

do_cleanup:
	cleanup();
	return false;
}

void milliways_backend::cleanup()
{
	if (! init)
		return;
	if (cleaned)
		return;
	TRACE("milliways_backend::cleanup()");
	if (kv)
	{
		if (kv->isOpen())
			kv->close();

		delete kv;
		kv = NULL;
	}
	if (bs)
	{
		delete bs;
		bs = NULL;
	}
	init = false;
	cleaned = true;
	is_open = false;
}

#define BUFFER_SIZE 512
#define LEN_BUFFER_SIZE 16

static int milliways_backend__read_header(size_t *len_p, git_otype *type_p, git_odb_backend *backend_, const git_oid *oid)
{
#if TRACE_MW
	double t_start = Alembic::AbcCoreGit::time_ms();
#endif /* TRACE_MW */
	assert(len_p && type_p && backend_ && oid);

	milliways_backend *backend = reinterpret_cast<milliways_backend*>(backend_);
	assert(backend);

	if (cached.matches(oid)) {
		*type_p = cached.type;
		*len_p = cached.len;
		return GIT_SUCCESS;
	}

	std::string s_oid(reinterpret_cast<const char*>(oid->id), 20);
	// std::cerr << "milliways_backend__read_header('" << milliways::hexify(s_oid) << "')" << std::endl;
	kv_search_t search;
    if (! backend->kv->find(s_oid, search)) {
#if TRACE_MW
		double t_elapsed = Alembic::AbcCoreGit::time_ms() - t_start;
		std::cerr << "MW::GETH " << milliways::hexify(s_oid) << " <- NO (" << t_elapsed << " ms)" << std::endl;
#endif /* TRACE_MW */
		return GIT_ENOTFOUND;
	}

	assert(search.found());

	std::string s_type, s_size;
	uint32_t v_type, v_size;

	seriously::Packer<LEN_BUFFER_SIZE> packer;

	/* extract type (int) */
	bool ok = backend->kv->get(search, s_type, sizeof(uint32_t));
	if (! ok) {
#if TRACE_MW
		double t_elapsed = Alembic::AbcCoreGit::time_ms() - t_start;
		std::cerr << "MW::GETH " << milliways::hexify(s_oid) << " <- NO (" << t_elapsed << " ms)" << std::endl;
#endif /* TRACE_MW */
		return GIT_ENOTFOUND;
	}
	packer.data(s_type.data(), s_type.size());
	packer >> v_type;

	*type_p = static_cast<git_otype>(v_type);

	/* extract value size (int) (but don't extract the value string itself) */
	*len_p = 0;
	ok = backend->kv->get(search, s_size, sizeof(uint32_t));
	if (! ok) {
#if TRACE_MW
		double t_elapsed = Alembic::AbcCoreGit::time_ms() - t_start;
		std::cerr << "MW::GETH " << milliways::hexify(s_oid) << " <- NO (" << t_elapsed << " ms)" << std::endl;
#endif /* TRACE_MW */
		return GIT_ENOTFOUND;
	}
	packer.data(s_size.data(), s_size.size());
	packer >> v_size;

	*len_p = static_cast<size_t>(v_size);

#if TRACE_MW
	double t_elapsed = Alembic::AbcCoreGit::time_ms() - t_start;
	std::cerr << "MW::GETH " << milliways::hexify(s_oid) << " <- OK (" << t_elapsed << " ms) " << v_type << " " << v_size << std::endl;
#endif /* TRACE_MW */

	return GIT_SUCCESS;
}

static int milliways_backend__read(void **data_p, size_t *len_p, git_otype *type_p, git_odb_backend *backend_, const git_oid *oid)
{
#if TRACE_MW
	double t_start = Alembic::AbcCoreGit::time_ms();
#endif /* TRACE_MW */
	assert(data_p && len_p && type_p && backend_ && oid);

	milliways_backend *backend = reinterpret_cast<milliways_backend*>(backend_);
	assert(backend);

	if (cached.matches(oid)) {
		*type_p = cached.type;
		*len_p = cached.len;
		if (data_p) {
			if (! cached.data)
				return GIT_ENOMEM;
			char* databuf = (char *)malloc(cached.len + 1);
			if (! databuf)
				return GIT_ENOMEM;
			assert(cached.data);
			memcpy(databuf, cached.data, cached.len);
			databuf[cached.len] = '\0';
			*data_p = databuf;
		}
		return GIT_SUCCESS;
	}

	std::string s_oid(reinterpret_cast<const char*>(oid->id), 20);
	// std::cerr << "milliways_backend__read('" << milliways::hexify(s_oid) << "')" << std::endl;
	kv_search_t search;
    if (! backend->kv->find(s_oid, search)) {
#if TRACE_MW
		double t_elapsed = Alembic::AbcCoreGit::time_ms() - t_start;
		std::cerr << "MW::GET " << milliways::hexify(s_oid) << " <- NO (" << t_elapsed << " ms)" << std::endl;
#endif /* TRACE_MW */
		// std::cerr << "  not found '" << milliways::hexify(s_oid) << "'" << std::endl;
		return GIT_ENOTFOUND;
	}

	assert(search.found());
	// std::cerr << "  found '" << milliways::hexify(s_oid) << "'" << std::endl;

	std::string blob;
	bool ok = backend->kv->get(search, blob);
	if (! ok) {
#if TRACE_MW
		double t_elapsed = Alembic::AbcCoreGit::time_ms() - t_start;
		std::cerr << "MW::GET " << milliways::hexify(s_oid) << " <- NO (" << t_elapsed << " ms)" << std::endl;
#endif /* TRACE_MW */
		// std::cerr << "  ERR: not found '" << milliways::hexify(s_oid) << "' at later stage!" << std::endl;
		return GIT_ENOTFOUND;
	}

	const char *blob_ptr   = blob.data();
	size_t      blob_avail = blob.size();

	uint32_t v_type = (uint32_t)-1, v_size = (uint32_t)-1;
	std::string s_data;

	seriously::Traits<uint32_t>::deserialize(blob_ptr, blob_avail, v_type);
	seriously::Traits<uint32_t>::deserialize(blob_ptr, blob_avail, v_size);
	assert(blob.size() == (sizeof(uint32_t) * 2 + v_size));
	assert((size_t)v_size == blob_avail);
	s_data.reserve(v_size);
	s_data.resize(v_size);
	s_data.assign(blob_ptr, std::min((size_t)v_size, blob_avail));
	// std::cerr << "  type:" << v_type << " size:" << v_size << " data-len:" << s_data.size() << " data:" << milliways::s_hexdump(s_data.data(), std::min(s_data.size(), (unsigned long)32)) << std::endl;
	*type_p = static_cast<git_otype>(v_type);
	*len_p = static_cast<size_t>(v_size);

#if 0
	seriously::Packer<LEN_BUFFER_SIZE> packer;
	std::string s_type, s_size;

	/* extract type (int) */
	bool ok = backend->kv->get(search, s_type, sizeof(uint32_t));
	if (! ok) {
		return GIT_ENOTFOUND;
	}
	packer.data(s_type.data(), s_type.size());
	packer >> v_type;

	*type_p = static_cast<git_otype>(v_type);

	/* extract value size (int) */
	*len_p = 0;
	ok = backend->kv->get(search, s_size, sizeof(uint32_t));
	if (! ok) {
		return GIT_ENOTFOUND;
	}
	packer.data(s_size.data(), s_size.size());
	packer >> v_size;
	std::cerr << "  type:" << v_type << " size:" << v_size << std::endl;

	*len_p = static_cast<size_t>(v_size);

	/* extract value (string) */
	std::string s_data;
	ok = backend->kv->get(search, s_data, static_cast<size_t>(v_size));
	if (! ok) {
		return GIT_ENOTFOUND;
	}
#endif
	char* databuf = (char *)malloc(static_cast<size_t>(v_size) + 1);
	if (! databuf)
		return GIT_ENOMEM;
	memcpy(databuf, s_data.data(), s_data.size());
	databuf[s_data.size()] = '\0';
	*data_p = databuf;

#if TRACE_MW
	double t_elapsed = Alembic::AbcCoreGit::time_ms() - t_start;
	std::cerr << "MW::GET " << milliways::hexify(s_oid) << " <- OK (" << t_elapsed << " ms) " << v_type << " " << v_size << " " << milliways::hexify(s_data) << std::endl;
#endif /* TRACE_MW */
	cached.set(oid, *type_p, *len_p, databuf);

	return GIT_SUCCESS;
}

static int milliways_backend__exists(git_odb_backend *backend_, const git_oid *oid)
{
#if TRACE_MW
	double t_start = Alembic::AbcCoreGit::time_ms();
#endif /* TRACE_MW */
	assert(backend_ && oid);

	if (cached.matches(oid)) {
		return cached.present ? 1 : 0;
	}

	milliways_backend *backend = reinterpret_cast<milliways_backend*>(backend_);
	assert(backend);

	std::string s_oid(reinterpret_cast<const char*>(oid->id), 20);
	// std::cerr << "milliways_backend__exists('" << milliways::hexify(s_oid) << "')" << std::endl;
	kv_search_t search;
    int r = backend->kv->has(s_oid) ? 1 : 0;
#if TRACE_MW
	double t_elapsed = Alembic::AbcCoreGit::time_ms() - t_start;
	std::cerr << "MW::HAS " << milliways::hexify(s_oid) << " <- (" << t_elapsed << " ms) " << (r ? "TRUE" : "FALSE") << std::endl;
#endif /* TRACE_MW */
	return r;
}

static int milliways_backend__write(git_odb_backend *backend_, const git_oid *oid_, const void *data, size_t len, git_otype type)
{
#if TRACE_MW
	double t_start = Alembic::AbcCoreGit::time_ms();
#endif /* TRACE_MW */
	git_oid oid_data, *oid = NULL;
	if (oid_)
	{
		oid = (git_oid *)oid_;
	} else
	{
		int status;
		oid = &oid_data;
		if ((status = git_odb_hash(oid, data, len, type)) < 0) {
#if TRACE_MW
			double t_elapsed = Alembic::AbcCoreGit::time_ms() - t_start;
			std::cerr << "MW::PUT ERROR (type:" << type << " len:" << len << ") (" << t_elapsed << " ms)" << std::endl;
#endif /* TRACE_MW */
			return status;
		}
	}

	assert(oid && backend_ && data);

	milliways_backend *backend = reinterpret_cast<milliways_backend*>(backend_);
	assert(backend);

	if (! notified_first_write) {
		std::cerr << "FIRST MILLIWAYS WRITE" << std::endl;
		notified_first_write = 1;
	}
	uint32_t v_type = static_cast<uint32_t>(type);
	uint32_t v_size = static_cast<uint32_t>(len);

	seriously::Packer<BUFFER_SIZE> packer;
	packer << v_type << v_size;

	std::string whole(packer.data(), packer.size());
	whole.append(reinterpret_cast<const char*>(data), len);

	std::string s_oid(reinterpret_cast<const char*>(oid->id), 20);
	// std::cerr << "milliways_backend__write('" << milliways::hexify(s_oid) << "', len:" << len << ", type:" << v_type << ")" << std::endl;
    if (! backend->kv->put(s_oid, whole)) {
#if TRACE_MW
		double t_elapsed = Alembic::AbcCoreGit::time_ms() - t_start;
		std::cerr << "MW::PUT " << milliways::hexify(s_oid) << " <- NO (" << t_elapsed << " ms) " << v_type << " " << v_size << " " << milliways::hexify(whole) << std::endl;
#endif /* TRACE_MW */
		return GIT_ERROR;
	}

#if TRACE_MW
	double t_elapsed = Alembic::AbcCoreGit::time_ms() - t_start;
	std::cerr << "MW::PUT " << milliways::hexify(s_oid) << " <- OK (" << t_elapsed << " ms) " << v_type << " " << v_size << " " << milliways::hexify(whole) << std::endl;
#endif /* TRACE_MW */
	cached.set(oid, type, len, data);

	return GIT_SUCCESS;
}

int milliways_backend__cleanedup(git_odb_backend *backend_)
{
	if (! backend_)
		return 0;
	milliways_backend* backend = reinterpret_cast<milliways_backend*>(backend_);

	if (! backend)
		return 0;
	assert(backend);

	return backend->cleanedup();
}

void milliways_backend__free(git_odb_backend *backend_)
{
	TRACE("CLEANUP MILLIWAYS BACKEND");
	assert(backend_);

	milliways_backend* backend = reinterpret_cast<milliways_backend*>(backend_);

	if (! backend)
		return;
	assert(backend);

	if (backend->decref() == 0) {
		std::cerr << "CLEANUP MILLIWAYS BACKEND\n";
		if (! backend->cleanedup())
			backend->cleanup();

		delete backend;
		backend = NULL;
	}
}

	/* refdb backend */
static int milliways_refdb_backend__exists(int *exists, git_refdb_backend *backend_, const char *ref_name)
{
#if TRACE_MW
	double t_start = Alembic::AbcCoreGit::time_ms();
#endif /* TRACE_MW */
	assert(backend_ && ref_name);

	milliways_backend *backend = milliways_backend::FromRefdb(backend_);
	assert(backend);

	std::string key("refdb:");
	key += ref_name;

	// std::cerr << "milliways_refdb_backend__exists('" << key << "')" << std::endl;
	kv_search_t search;
    int r = backend->kv->has(key) ? 1 : 0;
#if TRACE_MW
	double t_elapsed = Alembic::AbcCoreGit::time_ms() - t_start;
	std::cerr << "MW::HAS " << key << " <- (" << t_elapsed << " ms) " << (r ? "TRUE" : "FALSE") << std::endl;
#endif /* TRACE_MW */
	return r;
}

static int milliways_refdb_backend__lookup(git_reference **out, git_refdb_backend *backend_, const char *ref_name)
{
#if TRACE_MW
	double t_start = Alembic::AbcCoreGit::time_ms();
#endif /* TRACE_MW */
	assert(out && backend_ && ref_name);
	int error = GIT_OK;

	milliways_backend *backend = milliways_backend::FromRefdb(backend_);
	assert(backend);

	std::string key("refdb:");
	key += ref_name;

	// std::cerr << "milliways_refdb_backend__lookup('" << key << "')" << std::endl;
	kv_search_t search;
	bool found = backend->kv->find(key, search);
#if TRACE_MW
	if (! found) {
		double t_elapsed = Alembic::AbcCoreGit::time_ms() - t_start;
		std::cerr << "MW::GET " << key << " <- " << (ok ? "OK" : "NO") << " (" << t_elapsed << " ms)" << std::endl;
	}
#endif /* TRACE_MW */
    if (! found) {
		giterr_set_str(GITERR_REFERENCE, "milliways refdb couldn't find ref");
		error = GIT_ENOTFOUND;
		return error;
	}

	assert(search.found());
	// std::cerr << "  found '" << key << "'" << std::endl;

	std::string blob;
	bool ok = backend->kv->get(search, blob);
#if TRACE_MW
	if (! ok) {
		double t_elapsed = Alembic::AbcCoreGit::time_ms() - t_start;
		std::cerr << "MW::GET " << key << " <- " << (ok ? "OK" : "NO") << " (" << t_elapsed << " ms)" << std::endl;
	}
#endif /* TRACE_MW */
	if (! ok) {
		giterr_set_str(GITERR_REFERENCE, "milliways refdb fetch error");
		error = GIT_ERROR;
		return error;
	}

	const char *blob_ptr   = blob.data();
	size_t      blob_avail = blob.size();

	uint32_t v_ref_type = (uint32_t)-1;
	std::string v_ref;

	seriously::Traits<uint32_t>::deserialize(blob_ptr, blob_avail, v_ref_type);
	seriously::Traits<std::string>::deserialize(blob_ptr, blob_avail, v_ref);
	// std::cerr << "  ref_type:" << v_ref_type << " ref:" << v_ref << std::endl;

	git_ref_t ref_type = static_cast<git_ref_t>(v_ref_type);
	git_oid oid;

	switch (ref_type)
	{
	case GIT_REF_INVALID:
		giterr_set_str(GITERR_REFERENCE, "milliways refdb invalid reference type");
		error = GIT_ERROR;
		std::cerr << "ERROR[REFDB]: invalid reference type for reference '" << ref_name << "'" << std::endl;
		return error;
		break;
	case GIT_REF_OID:
		git_oid_fromstr(&oid, v_ref.c_str());
		if (out)
			*out = git_reference__alloc(ref_name, &oid, NULL);
		break;
	case GIT_REF_SYMBOLIC:
		if (out)
			*out = git_reference__alloc_symbolic(ref_name, v_ref.c_str());
		break;

	case GIT_REF_LISTALL:
	default:
		giterr_set_str(GITERR_REFERENCE, "milliways refdb unsupported reference type");
		error = GIT_ERROR;
		std::cerr << "ERROR[REFDB]: unsupported reference type " << static_cast<int>(ref_type) << " for reference '" << ref_name << "'" << std::endl;
		return error;
		break;
	}

#if TRACE_MW
	double t_elapsed = Alembic::AbcCoreGit::time_ms() - t_start;
	std::cerr << "MW::GET " << key << " <- OK (" << t_elapsed << " ms) " << v_ref_type << " " << v_ref << std::endl;
#endif /* TRACE_MW */

	return GIT_SUCCESS;
}

static int milliways_refdb_backend__iterator(git_reference_iterator **iter_, struct git_refdb_backend *backend_, const char *glob)
{
#if TRACE_MW
	double t_start = Alembic::AbcCoreGit::time_ms();
#endif /* TRACE_MW */
	assert(iter_ && backend_);

	milliways_backend *backend = milliways_backend::FromRefdb(backend_);
	assert(backend);

	std::string pattern("refdb:");
	pattern += ((glob != NULL) ? glob : "refs/*");

	kv_glob_iterator_t it = backend->kv->glob(pattern);

	// milliways_refdb_iterator *iterator = (milliways_refdb_iterator*) calloc(1, sizeof(milliways_refdb_iterator));
	milliways_refdb_iterator* iterator = new milliways_refdb_iterator(it, backend);

	// iterator->backend = backend;
	// iterator->iterator = new kv_glob_iterator_t(it);
	// iterator->current = 0;
	// iterator->parent.next = &milliways_refdb_backend__iterator_next;
	// iterator->parent.next_name = &milliways_refdb_backend__iterator_next_name;
	// iterator->parent.free = &milliways_refdb_backend__iterator_free;

	*iter_ = (git_reference_iterator *) iterator;

#if TRACE_MW
	double t_elapsed = Alembic::AbcCoreGit::time_ms() - t_start;
	std::cerr << "MW::GLOB start '" << pattern << "' <- OK (" << t_elapsed << " ms) " << std::endl;
#endif /* TRACE_MW */

	return GIT_SUCCESS;
}

static int milliways_refdb_backend__iterator_next(git_reference **ref, git_reference_iterator *iter_)
{
	assert(iter_);
	assert(ref);

	milliways_refdb_iterator* iterator = (milliways_refdb_iterator*) iter_;

	assert(iterator->iterator);

	if (iterator->iterator->end())
		return GIT_ITEROVER;

	std::string key = iterator->iterator->lookupKey();
	std::string ref_name = key.substr(6);
	int error = milliways_refdb_backend__lookup(ref, iterator->refdb_backend(), ref_name.c_str());

	++iterator->iterator;

	return error;
}

static int milliways_refdb_backend__iterator_next_name(const char **ref_name, git_reference_iterator *iter_)
{
	assert(iter_);
	assert(ref_name);

	milliways_refdb_iterator* iterator = (milliways_refdb_iterator*) iter_;

	assert(iterator->iterator);

	if (iterator->iterator->end())
		return GIT_ITEROVER;

	std::string key = iterator->iterator->lookupKey();
	std::string ref_name_ = key.substr(6);
	*ref_name = strdup(ref_name_.c_str());

	++iterator->iterator;

	return GIT_OK;
}

static void milliways_refdb_backend__iterator_free(git_reference_iterator *iter_)
{
	assert(iter_);

	milliways_refdb_iterator* iterator = (milliways_refdb_iterator*) iter_;

	if (iterator)
		delete iterator;
}

static int milliways_refdb_backend__write(git_refdb_backend *backend_, const git_reference *ref, int force, const git_signature *who,
	const char *message, const git_oid *old, const char *old_target)
{
#if TRACE_MW
	double t_start = Alembic::AbcCoreGit::time_ms();
#endif /* TRACE_MW */
	assert(ref && backend_);

	milliways_backend *backend = milliways_backend::FromRefdb(backend_);
	assert(backend);

	bool ok;
	int error = GIT_OK;

	const char *name = git_reference_name(ref);
	const git_oid *target;
	const char *symbolic_target;
	char oid_str[GIT_OID_HEXSZ + 1];


	target = git_reference_target(ref);
	symbolic_target = git_reference_symbolic_target(ref);

	std::string key("refdb:");
	key += name;

	/* FIXME handle force correctly */

	uint32_t v_type;
	std::string v_target;

	if (target) {
		git_oid_nfmt(oid_str, sizeof(oid_str), target);
		v_type = GIT_REF_OID;
		v_target = oid_str;
	} else {
		symbolic_target = git_reference_symbolic_target(ref);
		v_type = GIT_REF_SYMBOLIC;
		v_target = symbolic_target;
	}

	seriously::Packer<BUFFER_SIZE> packer;
	packer << v_type << v_target;
	std::string whole(packer.data(), packer.size());

	ok = backend->kv->put(key, whole);
#if TRACE_MW
	double t_elapsed = Alembic::AbcCoreGit::time_ms() - t_start;
	std::cerr << "MW::PUT " << key << " <- " << (ok ? "OK" : "NO") << " (" << t_elapsed << " ms) " << v_type << " " << v_target << std::endl;
#endif /* TRACE_MW */
	if (! ok) {
		giterr_set_str(GITERR_REFERENCE, "milliways refdb storage error");
		error = GIT_ERROR;
	} else
		error = GIT_OK;

	return error;
}

static int milliways_refdb_backend__rename(git_reference **out, git_refdb_backend *backend_, const char *old_name,
	const char *new_name, int force, const git_signature *who, const char *message)
{
	assert(backend_);
	assert(old_name && new_name);

	milliways_backend *backend = milliways_backend::FromRefdb(backend_);
	assert(backend);

	int error = GIT_ERROR;
	giterr_set_str(GITERR_REFERENCE, "milliways refdb storage error - rename UNIMPLEMENTED (yet)");

	return error;
}

static int milliways_refdb_backend__del(git_refdb_backend *backend_, const char *ref_name, const git_oid *old, const char *old_target)
{
	assert(backend_);
	assert(ref_name);

	milliways_backend *backend = milliways_backend::FromRefdb(backend_);
	assert(backend);

	int error = GIT_ERROR;
	giterr_set_str(GITERR_REFERENCE, "milliways refdb storage error - delete UNIMPLEMENTED (yet)");

	return error;
}

static void milliways_refdb_backend__free(git_refdb_backend *backend_)
{
	std::cerr << "milliways_refdb_backend__free()\n";
	assert(backend_);

	milliways_backend *backend = milliways_backend::FromRefdb(backend_);

	if (! backend)
		return;
	assert(backend);

	if (backend->decref() == 0) {
		std::cerr << "CLEANUP MILLIWAYS BACKEND\n";
		if (! backend->cleanedup())
			backend->cleanup();

		delete backend;
		backend = NULL;
	}
}

	/* refdb backend - reflog */
static int milliways_refdb_backend__has_log(git_refdb_backend *backend_, const char *refname)
{
	return 0;
}

static int milliways_refdb_backend__ensure_log(git_refdb_backend *backend_, const char *refname)
{
	std::cerr << "ERROR[REFDB]: reflog UNIMPLEMENTED" << std::endl;
	giterr_set_str(GITERR_REFERENCE, "reflog UNIMPLEMENTED");
	return GIT_ERROR;
}

static int milliways_refdb_backend__reflog_read(git_reflog **out, git_refdb_backend *backend_, const char *name)
{
	std::cerr << "ERROR[REFDB]: reflog UNIMPLEMENTED" << std::endl;
	giterr_set_str(GITERR_REFERENCE, "reflog UNIMPLEMENTED");
	return GIT_ERROR;
}

static int milliways_refdb_backend__reflog_write(git_refdb_backend *backend_, git_reflog *reflog)
{
	std::cerr << "ERROR[REFDB]: reflog UNIMPLEMENTED" << std::endl;
	giterr_set_str(GITERR_REFERENCE, "reflog UNIMPLEMENTED");
	return GIT_ERROR;
}

static int milliways_refdb_backend__reflog_rename(git_refdb_backend *backend_, const char *old_name, const char *new_name)
{
	std::cerr << "ERROR[REFDB]: reflog UNIMPLEMENTED" << std::endl;
	giterr_set_str(GITERR_REFERENCE, "reflog UNIMPLEMENTED");
	return GIT_ERROR;
}

static int milliways_refdb_backend__reflog_delete(git_refdb_backend *backend_, const char *name)
{
	std::cerr << "ERROR[REFDB]: reflog UNIMPLEMENTED" << std::endl;
	giterr_set_str(GITERR_REFERENCE, "reflog UNIMPLEMENTED");
	return GIT_ERROR;
}


int git_odb_backend_milliways(git_odb_backend **backend_out, const char *pathname)
{
	// std::cerr << "START MILLIWAYS BACKEND\n";

#if 0
	milliways_backend* backend = new milliways_backend();
	if (! backend)
		return GIT_ENOMEM;
	assert(backend);

	if (! backend->open(pathname))
		return GIT_ERROR;

	*backend_out = (git_odb_backend *) backend;
#endif
	// use a singleton mapping on 'pathname'
	milliways_backend* backend = milliways_backend::GetInstance(pathname);
	if (! backend)
		return GIT_ENOMEM;
	assert(backend);
	if (! backend->isOpen())
		return GIT_ERROR;
	assert(backend->isOpen());

	backend->incref();
	*backend_out = backend->odb_backend();

	return GIT_SUCCESS;
}

int git_refdb_backend_milliways(git_refdb_backend **backend_out, const char *pathname)
{
	// std::cerr << "START MILLIWAYS BACKEND\n";

	// use a singleton mapping on 'pathname'
	milliways_backend* backend = milliways_backend::GetInstance(pathname);
	if (! backend)
		return GIT_ENOMEM;
	assert(backend);
	if (! backend->isOpen())
		return GIT_ERROR;
	assert(backend->isOpen());

	backend->incref();
	*backend_out = backend->refdb_backend();

	return GIT_SUCCESS;
}
