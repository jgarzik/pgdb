
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <alloca.h>
#include <endian.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <openssl/sha.h>

#include "pgdb-internal.h"

static void __pgdb_free(pgdb_t *db)
{
	if (!db)
		return;

	free(db->pathname);

	if (db->superblock)
		pgcodec__superblock__free_unpacked(db->superblock, NULL);

	memset(db, 0xff, sizeof(*db));
	free(db);
}

void pgdb_close(pgdb_t* db)
{
	__pgdb_free(db);
}

static bool verify_file(char *magic, const void *file_data,
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
	
	pgmap_free(map);
	return true;

err_out:
	pgmap_free(map);
	return false;
}

static bool wrap_file(int fd, char *magic, const void *data,
		      size_t data_len, char **errptr)
{
	struct pgdb_file_header hdr;

	// compute file header
	memcpy(&hdr.magic, magic, sizeof(hdr.magic));
	hdr.len = htole32(data_len);
	hdr.reserved = 0;

	// compute file trailer
	SHA256_CTX ctx;
	unsigned char md[SHA256_DIGEST_LENGTH];

	SHA256_Init(&ctx);
	SHA256_Update(&ctx, &hdr, sizeof(hdr));
	SHA256_Update(&ctx, data, data_len);
	SHA256_Final(md, &ctx);

	// build output data list
	struct iovec iov[3];
	iov[0].iov_base = &hdr;		// header
	iov[0].iov_len = sizeof(hdr);
	iov[1].iov_base = (void *) data;// data
	iov[1].iov_len = data_len;
	iov[2].iov_base = &md[0];	// trailer
	iov[2].iov_len = sizeof(md);

	size_t total_len =
		iov[0].iov_len +
		iov[1].iov_len +
		iov[2].iov_len;

	// output data to fd
	ssize_t bwrite = writev(fd, iov, 3);
	if (bwrite != total_len) {
		*errptr = strdup(strerror(errno));
		return false;
	}

	return true;
}

static bool write_superblock(pgdb_t *db, char **errptr)
{
	// build pathnames
	size_t fn_len = strlen(db->pathname) + strlen(PGDB_SB_FN) + 4 + 2;
	char *tmp_fn = alloca(fn_len);
	char *fn = alloca(fn_len);
	snprintf(tmp_fn, fn_len, "%s/" PGDB_SB_FN ".tmp", db->pathname);
	snprintf(fn, fn_len, "%s/" PGDB_SB_FN, db->pathname);

	bool rc = false;

	// serialize superblock
	size_t plen = pgcodec__superblock__get_packed_size(db->superblock);
	void *pbuf = malloc(plen);
	if (!pbuf) {
		*errptr = strdup("OOM");	// irony, but recoverable
		goto out;
	}
	pgcodec__superblock__pack(db->superblock, pbuf);

	// open new temp file
	int fd = open(tmp_fn, O_WRONLY | O_CREAT | O_EXCL, 0666);
	if (fd < 0) {
		*errptr = strdup(strerror(errno));
		goto out_pbuf;
	}

	// write serialized data to temp file
	if (!wrap_file(fd, PGDB_SB_MAGIC, pbuf, plen, errptr))
		goto out_fd;

	close(fd);
	fd = -1;

	// rename into place
	if (rename(tmp_fn, fn) < 0) {
		*errptr = strdup(strerror(errno));
		goto out_fd;
	}

	rc = true;

out_fd:
	if (fd >= 0)
		close(fd);
	if (!rc)
		unlink(tmp_fn);
out_pbuf:
	free(pbuf);
out:
	return rc;
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
	if (!db->pathname) {
		*errptr = strdup("OOM");	// irony, but recoverable
		goto err_out;
	}

	if (!read_superblock(db, errptr))
		goto err_out;

	return db;

err_out:
	pgdb_close(db);
	return NULL;

oom:
	*errptr = strdup("OOM");	// irony, but recoverable
	return NULL;
}

