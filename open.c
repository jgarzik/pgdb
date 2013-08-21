
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
#include <assert.h>
#include <dirent.h>
#include <uuid/uuid.h>
#include <ctype.h>

#include "pgdb-internal.h"

static void pgdb_table_free(struct pgdb_table *table)
{
	free(table->name);
	table->name = NULL;

	if (table->root) {
		pgcodec__root_idx__free_unpacked(table->root, NULL);
		table->root = NULL;
	}
}

static void __pgdb_free(pgdb_t *db)
{
	if (!db)
		return;

	unsigned int i;
	for (i = 0; i < db->n_tables; i++)
		pgdb_table_free(&db->tables[i]);

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
	// generate (empty) root index, master table
	PGcodec__RootIdx root = PGCODEC__ROOT_IDX__INIT;

	// generate master table UUID
	uuid_t tab_uuid;
	char tab_uuid_s[128];
	uuid_generate_random(tab_uuid);
	uuid_unparse(tab_uuid, tab_uuid_s);

	// generate initial master table
	PGcodec__TableMeta table;
	table.name = "master";
	table.uuid = tab_uuid_s;
	table.root_id = 0;
	PGcodec__TableMeta *tables[1] = { &table };

	// generate root superblock UUID
	uuid_t sb_uuid;
	char sb_uuid_s[128];
	uuid_generate_random(sb_uuid);
	uuid_unparse(sb_uuid, sb_uuid_s);

	// generate initial superblock
	PGcodec__Superblock sb = PGCODEC__SUPERBLOCK__INIT;
	sb.uuid = sb_uuid_s;
	sb.n_tables = 1;
	sb.tables = tables;

	// create database directory
	if (mkdir(db->pathname, 0666) < 0) {
		*errptr = strdup(strerror(errno));
		return false;
	}

	// write superblock file
	if (!pg_write_superblock(db, &sb, errptr))
		return false;

	// write table's root index
	if (!pg_write_root(db, &root, 0, errptr))
		return false;

	return true;
}

struct id_scan_info {
	unsigned long long	file_id;
};

static bool id_scan_iter(const struct dirent *de, void *priv, char **errptr)
{
	unsigned int i;

	// only examine all-digit names
	for (i = 0; i < strlen(de->d_name); i++)
		if (!isdigit(de->d_name[i]))
			return true;		// continue dir iteration

	unsigned long long ll = strtoull(de->d_name, NULL, 10);

	struct id_scan_info *isi = priv;
	if (ll > isi->file_id)
		isi->file_id = ll;

	return true;		// continue dir iteration
}

static bool pg_scan_file_ids(pgdb_t *db, char **errptr)
{
	struct id_scan_info isi = { 0ULL };

	if (!pg_iterate_dir(db->pathname, id_scan_iter, &isi, errptr))
		return false;

	db->next_file_id = isi.file_id + 1;
	
	return true;
}

static PGcodec__TableMeta *find_tablemeta(PGcodec__Superblock *sb,
				      const char *tbl_name)
{
	unsigned int i;
	for (i = 0; i < sb->n_tables; i++) {
		PGcodec__TableMeta *tbl = sb->tables[i];
		if (!strcmp(tbl_name, tbl->name))
			return tbl;
	}

	return NULL;
}

static int pg_open_table(pgdb_t *db, const char *tbl_name, char **errptr)
{
	if (db->n_tables >= PGDB_MAX_TABLES) {
		*errptr = strdup("table limit reached");
		return -1;
	}

	struct pgdb_table *table = &db->tables[db->n_tables];
	memset(table, 0, sizeof(*table));

	PGcodec__TableMeta *tm = find_tablemeta(db->superblock, tbl_name);
	if (!tm) {
		*errptr = strdup("table not found");
		return -1;
	}

	if (!pg_read_root(db, &table->root, tm->root_id, errptr))
		return -1;
	
	table->root_id = tm->root_id;
	table->name = strdup(tm->name);
	if (!table->name) {
		pgcodec__root_idx__free_unpacked(table->root, NULL);
		memset(table, 0, sizeof(*table));
		*errptr = strdup("OOM");	// irony, but recoverable
		return -1;
	}

	int slot = db->n_tables;
	db->n_tables++;
	return slot;
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

	if (!pg_scan_file_ids(db, errptr))
		goto err_out;

	int slot = pg_open_table(db, "master", errptr);
	if (slot < 0)
		goto err_out;

	assert(slot == 0);

	return db;

err_out:
	pgdb_close(db);
	return NULL;

oom:
	*errptr = strdup("OOM");	// irony, but recoverable
	return NULL;
}

