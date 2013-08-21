
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <alloca.h>
#include <endian.h>
#include <openssl/sha.h>

#include "pgdb-internal.h"

static void __pgdb_free(pgdb_t *db)
{
	if (!db)
		return;

	free(db->pathname);

	memset(db, 0xff, sizeof(*db));
	free(db);
}

void pgdb_close(pgdb_t* db)
{
	__pgdb_free(db);
}

static bool verify_file(unsigned char *magic, const void *file_data,
			unsigned long file_len, char **errptr)
{
	const struct pgdb_file_header *hdr = file_data;

	// is header present?
	if (file_len < sizeof(*hdr)) {
		*errptr = strdup("file too short for header");
		return false;
	}
	
	// magic numbers must match
	if (memcmp(hdr->magic, magic, sizeof(hdr->magic))) {
		*errptr = strdup("magic mismatch");
		return false;
	}

	// encapsulated data length
	uint32_t len = le32toh(hdr->len);
	if (len > file_len) {
		*errptr = strdup("file too short for data");
		return false;
	}

	// total == header + data + trailer
	unsigned long want_len = sizeof(*hdr) + len + PGDB_TRAIL_SZ;
	if (want_len > file_len) {
		*errptr = strdup("file too short for data and metadata");
		return false;
	}

	// verify hash(hdr + data)
	const unsigned char *data = file_data + sizeof(*hdr);
	const unsigned char *trailer = data + len;
	unsigned char md[SHA256_DIGEST_LENGTH];

	SHA256(file_data, sizeof(*hdr) + len, md);
	if (memcmp(md, trailer, SHA256_DIGEST_LENGTH)) {
		*errptr = strdup("checksum mismatch");
		return false;
	}

	return true;
}

static bool read_superblock(pgdb_t *db, char **errptr)
{
	size_t fn_len = strlen(db->pathname) + strlen(PGDB_SB_FN) + 2;
	char *fn = alloca(fn_len);
	snprintf(fn, fn_len, "%s/" PGDB_SB_FN, db->pathname);

	struct pgdb_map *map = pgmap_open(fn, errptr);
	if (!map)
		return false;

	if (!verify_file(PGDB_SB_MAGIC, map->mem, map->st.st_size, errptr))
		goto err_out;

	struct pgdb_file_header *hdr = map->mem;

	db->superblock = pgcodec__superblock__unpack(NULL,
						     le32toh(hdr->len),
						     map->mem + sizeof(*hdr));
	if (!db->superblock) {
		*errptr = strdup("sb deser failed");
		goto err_out;
	}
	
	return true;

err_out:
	pgmap_free(map);
	return false;
}

static bool is_dir(const char *pathname, bool readonly, char **errptr)
{
	struct stat st;

	if (stat(pathname, &st) < 0) {
		*errptr = strdup(strerror(errno));
		return false;
	}
	if (!S_ISDIR(st.st_mode)) {
		*errptr = "not a directory";
		return false;
	}

	int mode = R_OK;
	if (!readonly)
		mode |= W_OK;
	if (access(pathname, mode)) {
		*errptr = strdup(strerror(errno));
		return false;
	}

	return true;
}

pgdb_t* pgdb_open(
    const pgdb_options_t* options,
    const char* name,
    char** errptr)
{
	if (!is_dir(name, options->readonly, errptr))
		return NULL;

	pgdb_t *db = calloc(1, sizeof(*db));
	if (!db)
		goto oom;

	db->opt = options;

	db->pathname = strdup(name);
	if (!db->pathname)
		goto oom;

	if (!read_superblock(db, errptr))
		return NULL;

	return db;

err_out:
	pgdb_close(db);
	return NULL;

oom:
	*errptr = strdup("OOM");	// irony, but recoverable
	return NULL;
}

