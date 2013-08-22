
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include "pgdb-internal.h"

bool pg_rand_bytes(void *p, size_t len)
{
	int fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0)
		return false;
	
	while (len > 0) {
		ssize_t bread = read(fd, p, len);
		if (bread < 0) {
			close(fd);
			return false;
		}

		p += bread;
		len -= bread;
	}

	close(fd);

	return true;
}

bool pg_seed_libc_rng(void)
{
	unsigned int v = 0;

	if (!pg_rand_bytes(&v, sizeof(v)))
		return false;

	srand(v);
	
	return true;
}

