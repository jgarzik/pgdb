
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include "pgdb-internal.h"

void pgmap_free(struct pgdb_map *map)
{
	if (!map)
		return;

	if (map->mem)
		munmap(map->mem, map->st.st_size);

	if (map->fd >= 0)
		close(map->fd);

	free(map->pathname);
	
	memset(map, 0xff, sizeof(*map));
	free(map);
}

struct pgdb_map *pgmap_open(const char *pathname, char **errptr)
{
	struct pgdb_map *map = calloc(1, sizeof(struct pgdb_map));
	if (!map) {
		*errptr = strdup("OOM");
		goto err_out;
	}

	map->pathname = strdup(pathname);
	if (!map->pathname) {
		*errptr = strdup("OOM");
		goto err_out_map;
	}

	map->fd = open(pathname, O_RDONLY);
	if (map->fd < 0)
		goto err_out_errno;

	if (fstat(map->fd, &map->st) < 0)
		goto err_out_errno;

	if (map->st.st_size < sizeof(struct pgdb_file_header)) {
		*errptr = strdup("File too small for header");
		goto err_out_map;
	}

	map->mem = mmap(NULL, map->st.st_size, PROT_READ, MAP_SHARED, map->fd, 0);
	if (map->mem == MAP_FAILED)
		goto err_out_errno;

	return map;

err_out_map:
	map_free(map);
err_out:
	return NULL;

err_out_errno:
	*errptr = strdup(strerror(errno));
	goto err_out_map;
	
}

