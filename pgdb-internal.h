#ifndef __PGDB_INTERNAL_H__
#define __PGDB_INTERNAL_H__

#include <sys/stat.h>
#include <stdint.h>
#include "pgdb.h"
#include "PGcodec.pb-c.h"

struct dirent;

#define PGDB_SB_FN		"superblock"
#define PGDB_SB_MAGIC		"PGDBSUPR"
#define PGDB_ROOT_MAGIC		"PGDBROOT"
#define PGDB_PAGE_MAGIC		"PGDBPAGE"

enum {
	PGDB_TRAIL_SZ		= 32,		// sha256

	PGDB_MAX_TABLES		= 1,
};

struct pgdb_file_header {
	unsigned char		magic[8];
	uint32_t		len;
	uint32_t		reserved;
};

struct pgdb_options_t {
	bool			readonly;
	bool			create_missing;
	bool			error_if_exists;
};

struct pgdb_map {
	char			*pathname;
	int			fd;
	struct stat		st;
	void			*mem;
};

struct pgdb_page_hdr {
	unsigned char		magic[8];
	uint32_t		n_entries;
	unsigned char		reserved[20];
};

struct pgdb_page_index {
	uint32_t		k_offset;
	uint32_t		k_len;
	unsigned char		k_csum[4];		// first 4 of sha256
	uint32_t		reserved1;

	uint32_t		v_offset;
	uint32_t		v_len;
	unsigned char		v_csum[4];		// first 4 of sha256
	uint32_t		reserved2;
};

struct pgdb_pagefile {
	struct pgdb_map		*map;
	uint32_t		n_entries;
	struct pgdb_page_index	*pi;
};

struct pgdb_table {
	char				*name;
	uint64_t			root_id;
	PGcodec__RootIdx		*root;
};

struct pgdb_t {
	const struct pgdb_options_t	*opt;
	char				*pathname;

	PGcodec__Superblock		*superblock;
	unsigned int			n_tables;
	struct pgdb_table		tables[PGDB_MAX_TABLES];
};

extern void pgmap_free(struct pgdb_map *map);
extern struct pgdb_map *pgmap_open(const char *pathname, char **errptr);

extern bool pg_write_root(pgdb_t *db, PGcodec__RootIdx *root, unsigned int n,
		   char **errptr);
extern bool pg_read_root(pgdb_t *db, PGcodec__RootIdx **root, unsigned int n,
		  char **errptr);

extern void pg_pagefile_close(struct pgdb_pagefile *pf);
extern struct pgdb_pagefile *pg_pagefile_open(pgdb_t *db, unsigned int n,
					char **errptr);
extern int pg_pagefile_find(struct pgdb_pagefile *pf, const void *key_a, size_t alen,
		     bool exact_match);

extern bool pg_have_superblock(const char *dirname);
extern bool pg_write_superblock(pgdb_t *db, PGcodec__Superblock *sb,
				char **errptr);
extern bool pg_read_superblock(pgdb_t *db, char **errptr);

extern bool pg_wrap_file(int fd, char *magic, const void *data,
	      size_t data_len, char **errptr);
extern bool pg_verify_file(char *magic, const void *file_data,
		unsigned long file_len, char **errptr);
extern bool pg_iterate_dir(const char *dirname,
		 bool (*actor)(const struct dirent *de, void *priv,
		 	       char **errptr),
		 void *priv, char **errptr);

#endif // __PGDB_INTERNAL_H__
