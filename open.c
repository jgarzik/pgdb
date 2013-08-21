
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
#include <uuid/uuid.h>

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

static bool pg_create_db(pgdb_t *db, char **errptr)
{
	// create database directory
	if (mkdir(db->pathname, 0666) < 0) {
		*errptr = strdup(strerror(errno));
		return false;
	}

	// generate root UUID
	uuid_t uuid;
	char uuid_s[128];
	uuid_generate_random(uuid);
	uuid_unparse(uuid, uuid_s);

	// generate template superblock
	PGcodec__Superblock sb = PGCODEC__SUPERBLOCK__INIT;
	sb.uuid = &uuid_s[0];

	// write superblock file
	if (!pg_write_superblock(db, &sb, errptr))
		return false;

	return true;
}

pgdb_t* pgdb_open(
    const pgdb_options_t* options,
    const char* name,
    char** errptr)
{
	bool have_dir = access(name, F_OK) == 0;

	if (have_dir && options->error_if_exists) {
		*errptr = strdup("database already exists");
		return NULL;
	}

	bool create = false;
	if (!have_dir) {
		if (!options->create_missing) {
			*errptr = strdup("database missing");
			return NULL;
		} else
			create = true;
	}

	if (create && options->readonly) {
		*errptr = strdup("cannot create RO database");
		return NULL;
	}

	if (!create && !is_dir(name, options->readonly, errptr))
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

	if (create && !pg_create_db(db, errptr))
		goto err_out;

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

