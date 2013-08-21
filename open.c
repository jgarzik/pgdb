
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

	if (!pg_read_superblock(db, errptr))
		goto err_out;

	return db;

err_out:
	pgdb_close(db);
	return NULL;

oom:
	*errptr = strdup("OOM");	// irony, but recoverable
	return NULL;
}

