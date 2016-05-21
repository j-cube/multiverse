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
#include <git2/odb_backend.h>

#include "git-milliways.h"

#include "milliways/Seriously.h"
#include "milliways/KeyValueStore.h"

#include "milliways/Utils.h"
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

/* ----------------------------------------------------------------- */
/*   LOCAL PROTOTYPES                                                */
/* ----------------------------------------------------------------- */

extern "C" {

static int milliways_backend__read_header(size_t *len_p, git_otype *type_p, git_odb_backend *backend_, const git_oid *oid);
static int milliways_backend__read(void **data_p, size_t *len_p, git_otype *type_p, git_odb_backend *backend_, const git_oid *oid);
static int milliways_backend__exists(git_odb_backend *backend_, const git_oid *oid);
static int milliways_backend__write(git_odb_backend *backend_, const git_oid *oid_, const void *data, size_t len, git_otype type);

} /* extern "C" */

/* ----------------------------------------------------------------- */
/*   CODE                                                            */
/* ----------------------------------------------------------------- */

static int notified_first_write = 0;
typedef milliways::KeyValueStore kv_store_t;
typedef typename kv_store_t::block_storage_type kv_blockstorage_t;
typedef typename kv_store_t::Search kv_search_t;

struct milliways_backend
{
public:
	git_odb_backend parent;
	kv_blockstorage_t* bs;
	kv_store_t* kv;
	bool init;
	bool cleaned;

	milliways_backend() :
		bs(NULL), kv(NULL), init(false), cleaned(false) { memset(&parent, 0, sizeof(git_odb_backend)); }
	~milliways_backend() { if (init) cleanup(); cleaned = true; }

	bool cleanedup() const { return cleaned; }

	bool open(const char *pathname);
	void cleanup();
};

bool milliways_backend::open(const char *pathname)
{
	std::cerr << "milliways_backend::open()\n";

	assert(! bs);
	assert(! kv);
	assert(! init);
	init = true;

	bs = new kv_blockstorage_t(pathname);
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
	std::cerr << "milliways_backend::cleanup()\n";
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
}

#define BUFFER_SIZE 512
#define LEN_BUFFER_SIZE 16

static int milliways_backend__read_header(size_t *len_p, git_otype *type_p, git_odb_backend *backend_, const git_oid *oid)
{
	assert(len_p && type_p && backend_ && oid);

	milliways_backend *backend = reinterpret_cast<milliways_backend*>(backend_);
	assert(backend);

	std::string s_oid(reinterpret_cast<const char*>(oid->id), 20);
	// std::cerr << "milliways_backend__read_header('" << milliways::hexify(s_oid) << "')" << std::endl;
	kv_search_t search;
    if (! backend->kv->find(s_oid, search)) {
		return GIT_ENOTFOUND;
	}

	assert(search.found());

	std::string s_type, s_size;
	uint32_t v_type, v_size;

	seriously::Packer<LEN_BUFFER_SIZE> packer;

	/* extract type (int) */
	bool ok = backend->kv->get(search, s_type, sizeof(uint32_t));
	if (! ok) {
		return GIT_ENOTFOUND;
	}
	packer.data(s_type.data(), s_type.size());
	packer >> v_type;

	*type_p = static_cast<git_otype>(v_type);

	/* extract value size (int) (but don't extract the value string itself) */
	*len_p = 0;
	ok = backend->kv->get(search, s_size, sizeof(uint32_t));
	if (! ok) {
		return GIT_ENOTFOUND;
	}
	packer.data(s_size.data(), s_size.size());
	packer >> v_size;

	*len_p = static_cast<size_t>(v_size);

	return GIT_SUCCESS;
}

static int milliways_backend__read(void **data_p, size_t *len_p, git_otype *type_p, git_odb_backend *backend_, const git_oid *oid)
{
	assert(data_p && len_p && type_p && backend_ && oid);

	milliways_backend *backend = reinterpret_cast<milliways_backend*>(backend_);
	assert(backend);

	std::string s_oid(reinterpret_cast<const char*>(oid->id), 20);
	// std::cerr << "milliways_backend__read('" << milliways::hexify(s_oid) << "')" << std::endl;
	kv_search_t search;
    if (! backend->kv->find(s_oid, search)) {
		// std::cerr << "  not found '" << milliways::hexify(s_oid) << "'" << std::endl;
		return GIT_ENOTFOUND;
	}

	assert(search.found());
	// std::cerr << "  found '" << milliways::hexify(s_oid) << "'" << std::endl;

	std::string blob;
	bool ok = backend->kv->get(search, blob);
	if (! ok) {
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

	return GIT_SUCCESS;
}

static int milliways_backend__exists(git_odb_backend *backend_, const git_oid *oid)
{
	assert(backend_ && oid);

	milliways_backend *backend = reinterpret_cast<milliways_backend*>(backend_);
	assert(backend);

	std::string s_oid(reinterpret_cast<const char*>(oid->id), 20);
	// std::cerr << "milliways_backend__exists('" << milliways::hexify(s_oid) << "')" << std::endl;
	kv_search_t search;
    return backend->kv->has(s_oid) ? 1 : 0;
}

static int milliways_backend__write(git_odb_backend *backend_, const git_oid *oid_, const void *data, size_t len, git_otype type)
{
	git_oid oid_data, *oid = NULL;
	if (oid_)
	{
		oid = (git_oid *)oid_;
	} else
	{
		int status;
		oid = &oid_data;
		if ((status = git_odb_hash(oid, data, len, type)) < 0)
			return status;
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
		return GIT_ERROR;
	}

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
	std::cerr << "CLEANUP MILLIWAYS BACKEND\n";
	assert(backend_);

	milliways_backend* backend = reinterpret_cast<milliways_backend*>(backend_);

	if (! backend)
		return;
	assert(backend);

	if (! backend->cleanedup())
		backend->cleanup();

	delete backend;
	backend = NULL;
}

int git_odb_backend_milliways(git_odb_backend **backend_out, const char *pathname)
{
	// std::cerr << "START MILLIWAYS BACKEND\n";

	milliways_backend* backend = new milliways_backend();
	if (! backend)
		return GIT_ENOMEM;
	assert(backend);

	if (! backend->open(pathname))
		return GIT_ERROR;

	*backend_out = (git_odb_backend *) backend;

	return GIT_SUCCESS;
}
