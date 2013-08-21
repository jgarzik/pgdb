#ifndef __PGDB_INTERNAL_H__
#define __PGDB_INTERNAL_H__

#include <sys/stat.h>
#include <stdint.h>
#include "pgdb.h"
#include "PGcodec.pb-c.h"

#define PGDB_SB_FN		"superblock"
#define PGDB_SB_MAGIC		"PGDBSUPR"

enum {
	PGDB_TRAIL_SZ		= 32,		// sha256
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

struct pgdb_t {
	const struct pgdb_options_t	*opt;
	char				*pathname;

	PGcodec__Superblock		*superblock;
};

extern void pgmap_free(struct pgdb_map *map);
extern struct pgdb_map *pgmap_open(const char *pathname, char **errptr);

extern bool pg_have_superblock(const char *dirname);
extern bool pg_write_superblock(pgdb_t *db, PGcodec__Superblock *sb,
				char **errptr);
extern bool pg_read_superblock(pgdb_t *db, char **errptr);

extern bool pg_wrap_file(int fd, char *magic, const void *data,
	      size_t data_len, char **errptr);
extern bool pg_verify_file(char *magic, const void *file_data,
		unsigned long file_len, char **errptr);

#endif // __PGDB_INTERNAL_H__
