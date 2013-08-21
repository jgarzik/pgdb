
#include <string.h>
#include <alloca.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <errno.h>

#include "pgdb-internal.h"

void pgdb_destroy_db(
    const pgdb_options_t* options,
    const char* name,
    char** errptr)
{
	DIR *dir;
	struct dirent *de;

	size_t fn_len = strlen(name) + 256 + 2;
	char *fn = alloca(fn_len);

	snprintf(fn, fn_len, "%s/" PGDB_SB_FN, name);
	if (access(fn, R_OK | W_OK)) {
		*errptr = strdup("not a pgdb database");
		return;
	}

	dir = opendir(name);
	if (!dir) {
		*errptr = strdup(strerror(errno));
		return;
	}

	while ((de = readdir(dir)) != NULL) {
		if ((!strcmp(de->d_name, ".")) ||
		    (!strcmp(de->d_name, "..")))
			continue;

		snprintf(fn, fn_len, "%s/%s", name, de->d_name);

		if (unlink(fn) < 0) {
			*errptr = strdup(strerror(errno));
			return;
		}
	}

	closedir(dir);

	if (rmdir(name) < 0) {
		*errptr = strdup(strerror(errno));
		return;
	}

	*errptr = NULL;
}

