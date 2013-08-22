
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <alloca.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <errno.h>

#include "pgdb-internal.h"

bool pg_have_superblock(const char *dirname)
{
	size_t fn_len = strlen(dirname) + strlen(PGDB_SB_FN) + 2;
	char *fn = alloca(fn_len);

	snprintf(fn, fn_len, "%s/" PGDB_SB_FN, dirname);
	return access(fn, R_OK | W_OK) == 0;
}

bool pg_write_superblock(pgdb_t *db, PGcodec__Superblock *superblock,
			 char **errptr)
{
	// build pathnames
	size_t fn_len = strlen(db->pathname) + strlen(PGDB_SB_FN) + 4 + 2;
	char *tmp_fn = alloca(fn_len);
	char *fn = alloca(fn_len);
	snprintf(tmp_fn, fn_len, "%s/" PGDB_SB_FN ".tmp", db->pathname);
	snprintf(fn, fn_len, "%s/" PGDB_SB_FN, db->pathname);

	bool rc = false;

	// serialize superblock
	size_t plen = pgcodec__superblock__get_packed_size(superblock);
	void *pbuf = malloc(plen);
	if (!pbuf) {
		*errptr = strdup("OOM");	// irony, but recoverable
		goto out;
	}
	pgcodec__superblock__pack(superblock, pbuf);

	// open new temp file
	int fd = open(tmp_fn, O_WRONLY | O_CREAT | O_EXCL, 0666);
	if (fd < 0) {
		*errptr = strdup(strerror(errno));
		goto out_pbuf;
	}

	// write serialized data to temp file
	if (!pg_wrap_file(fd, PGDB_SB_MAGIC, pbuf, plen, errptr))
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

bool pg_read_superblock(pgdb_t *db, char **errptr)
{
	size_t fn_len = strlen(db->pathname) + strlen(PGDB_SB_FN) + 2;
	char *fn = alloca(fn_len);
	snprintf(fn, fn_len, "%s/" PGDB_SB_FN, db->pathname);

	struct pgdb_map *map = pgmap_open(fn, errptr);
	if (!map)
		return false;

	if (!pg_verify_file(PGDB_SB_MAGIC, map->mem, map->st.st_size, errptr))
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

