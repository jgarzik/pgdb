
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "pgdb-internal.h"

bool pg_write_root(pgdb_t *db, PGcodec__RootIdx *root, unsigned int n,
		   char **errptr)
{
	// build pathname
	size_t fn_len = strlen(db->pathname) + 64 + 2;
	char *fn = alloca(fn_len);
	snprintf(fn, fn_len, "%s/%u", db->pathname, n);

	bool rc = false;

	// serialize root index
	size_t plen = pgcodec__root_idx__get_packed_size(root);
	void *pbuf = malloc(plen);
	if (!pbuf) {
		*errptr = strdup("OOM");	// irony, but recoverable
		goto out;
	}
	pgcodec__root_idx__pack(root, pbuf);

	// open new file
	int fd = open(fn, O_WRONLY | O_CREAT | O_EXCL, 0666);
	if (fd < 0) {
		*errptr = strdup(strerror(errno));
		goto out_pbuf;
	}

	// write serialized data to temp file
	if (!pg_wrap_file(fd, PGDB_ROOT_MAGIC, pbuf, plen, errptr))
		goto out_fd;

	close(fd);
	fd = -1;

	rc = true;

out_fd:
	if (fd >= 0)
		close(fd);
	if (!rc)
		unlink(fn);
out_pbuf:
	free(pbuf);
out:
	return rc;
}

bool pg_read_root(pgdb_t *db, PGcodec__RootIdx **root, unsigned int n,
		  char **errptr)
{
	size_t fn_len = strlen(db->pathname) + strlen(PGDB_SB_FN) + 2;
	char *fn = alloca(fn_len);
	snprintf(fn, fn_len, "%s/%u", db->pathname, n);

	struct pgdb_map *map = pgmap_open(fn, errptr);
	if (!map)
		return false;

	if (!pg_verify_file(PGDB_ROOT_MAGIC, map->mem, map->st.st_size, errptr))
		goto err_out;

	struct pgdb_file_header *hdr = map->mem;

	*root = pgcodec__root_idx__unpack(NULL,
					  le32toh(hdr->len),
					  map->mem + sizeof(*hdr));
	if (!*root) {
		*errptr = strdup("rootidx deser failed");
		goto err_out;
	}
	
	pgmap_free(map);
	return true;

err_out:
	pgmap_free(map);
	return false;
}

int pg_find_rootent(PGcodec__RootIdx *root, const void *key, size_t klen)
{
	unsigned int i;

	for (i = 0; i < root->n_entries; i++) {
		PGcodec__RootEnt *ent = root->entries[i];
		unsigned int len = (klen < ent->key.len) ? klen : ent->key.len;
		int cmp = memcmp(key, ent->key.data, len);
		if (cmp <= 0)
			return i;
	}

	return -1;
}

