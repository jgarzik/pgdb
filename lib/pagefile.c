
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <alloca.h>

#include "pgdb-internal.h"

static struct pgdb_map *open_map(pgdb_t *db, unsigned int n, char **errptr)
{
	size_t fn_len = strlen(db->pathname) + 64 + 2;
	char *fn = alloca(fn_len);
	snprintf(fn, fn_len, "%s/%u", db->pathname, n);

	return pgmap_open(fn, errptr);
}

void pg_pagefile_close(struct pgdb_pagefile *pf)
{
	if (!pf)
		return;

	pgmap_free(pf->map);
	
	memset(pf, 0xff, sizeof(*pf));
	free(pf);
}

struct pgdb_pagefile *pg_pagefile_open(pgdb_t *db, unsigned int n,
					char **errptr)
{
	struct pgdb_pagefile *pf = calloc(1, sizeof(struct pgdb_pagefile));
	if (!pf) {
		*errptr = strdup("OOM");	// irony, but recoverable
		return NULL;
	}

	pf->map = open_map(db, n, errptr);
	if (!pf->map)
		goto err_out;

	unsigned int check_len = sizeof(struct pgdb_page_hdr);
	if (pf->map->st.st_size < check_len) {
		*errptr = strdup("pagefile too small");
		goto err_out;
	}

	struct pgdb_page_hdr *phdr = pf->map->mem;
	if (memcmp(phdr->magic, PGDB_PAGE_MAGIC, sizeof(phdr->magic))) {
		*errptr = strdup("pagefile magic mismatch");
		goto err_out;
	}

	pf->n_entries = le32toh(phdr->n_entries);
	check_len += (pf->n_entries * sizeof(struct pgdb_page_index));
	if (pf->map->st.st_size < check_len) {
		*errptr = strdup("pagefile too small 2");
		goto err_out;
	}

	struct pgdb_page_index *pi = pf->map->mem;
	assert(sizeof(*phdr) == sizeof(*pi));
	pi++;
	pf->pi = pi;

	return pf;

err_out:
	pg_pagefile_close(pf);
	return NULL;
}

int pg_pagefile_find(struct pgdb_pagefile *pf, const void *key_a, size_t alen,
		     bool exact_match)
{
	unsigned int i;

	for (i = 0; i < pf->n_entries; i++) {
		struct pgdb_page_index *pi = &pf->pi[i];
		void *key_b = pf->map->mem + le32toh(pi->k_offset);
		size_t blen = le32toh(pi->k_len);

		size_t len = (alen < blen) ? alen : blen;
		int cmp = memcmp(key_a, key_b, len);
		if (cmp <= 0) {
			if (!exact_match)
				return i;
			if (exact_match && (cmp == 0) && (alen == blen))
				return i;
			return -1;
		}
	}

	return -1;
}

