#ifndef __PGDB_INTERNAL_H__
#define __PGDB_INTERNAL_H__

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

#endif // __PGDB_INTERNAL_H__
