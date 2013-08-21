
#include <string.h>

#include "pgdb-internal.h"

static int find_rootent(PGcodec__RootIdx *root, const void *key, size_t klen)
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

static char* __pgdb_get(
    pgdb_t* db, unsigned int table_slot,
    const pgdb_readoptions_t* options,
    const char* key, size_t keylen,
    size_t* vallen,
    char** errptr)
{
	*errptr = NULL;

	struct pgdb_table *table = &db->tables[table_slot];

	int root_idx = find_rootent(table->root, key, keylen);
	if (root_idx < 0)
		return NULL;

	struct pgdb_pagefile *pf = pg_pagefile_open(db, root_idx, errptr);
	if (!pf)
		return NULL;

	int slot = pg_pagefile_find(pf, key, keylen, true);
	if (slot < 0)
		goto out;

	struct pgdb_page_index *pi = &pf->pi[slot];
	uint32_t v_offset = le32toh(pi->v_offset);
	uint32_t v_len = le32toh(pi->v_len);

	void *v_mem = malloc(v_len);
	if (!v_mem) {
		*errptr = strdup("OOM");	// irony, but recoverable
		goto out;
	}

	memcpy(v_mem, pf->map->mem + v_offset, v_len);

	pg_pagefile_close(pf);
	*vallen = v_len;
	return v_mem;

out:
	pg_pagefile_close(pf);
	return NULL;
}

char* pgdb_get(
    pgdb_t* db,
    const pgdb_readoptions_t* options,
    const char* key, size_t keylen,
    size_t* vallen,
    char** errptr)
{
	return __pgdb_get(db, 0, options, key, keylen, vallen, errptr);
}

