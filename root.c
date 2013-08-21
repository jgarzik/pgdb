
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

