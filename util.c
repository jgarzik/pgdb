
#include <string.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <openssl/sha.h>

#include "pgdb-internal.h"

bool pg_wrap_file(int fd, char *magic, const void *data,
	      size_t data_len, char **errptr)
{
	struct pgdb_file_header hdr;

	// compute file header
	memcpy(&hdr.magic, magic, sizeof(hdr.magic));
	hdr.len = htole32(data_len);
	hdr.reserved = 0;

	// compute file trailer
	SHA256_CTX ctx;
	unsigned char md[SHA256_DIGEST_LENGTH];

	SHA256_Init(&ctx);
	SHA256_Update(&ctx, &hdr, sizeof(hdr));
	SHA256_Update(&ctx, data, data_len);
	SHA256_Final(md, &ctx);

	// build output data list
	struct iovec iov[3];
	iov[0].iov_base = &hdr;		// header
	iov[0].iov_len = sizeof(hdr);
	iov[1].iov_base = (void *) data;// data
	iov[1].iov_len = data_len;
	iov[2].iov_base = &md[0];	// trailer
	iov[2].iov_len = sizeof(md);

	size_t total_len =
		iov[0].iov_len +
		iov[1].iov_len +
		iov[2].iov_len;

	// output data to fd
	ssize_t bwrite = writev(fd, iov, 3);
	if (bwrite != total_len) {
		*errptr = strdup(strerror(errno));
		return false;
	}

	return true;
}

bool pg_verify_file(char *magic, const void *file_data,
		unsigned long file_len, char **errptr)
{
	const struct pgdb_file_header *hdr = file_data;

	// is header present?
	if (file_len < sizeof(*hdr)) {
		*errptr = strdup("file too short for header");
		return false;
	}
	
	// magic numbers must match
	if (memcmp(hdr->magic, magic, sizeof(hdr->magic))) {
		*errptr = strdup("magic mismatch");
		return false;
	}

	// encapsulated data length
	uint32_t len = le32toh(hdr->len);
	if (len > file_len) {
		*errptr = strdup("file too short for data");
		return false;
	}

	// total == header + data + trailer
	unsigned long want_len = sizeof(*hdr) + len + PGDB_TRAIL_SZ;
	if (want_len > file_len) {
		*errptr = strdup("file too short for data and metadata");
		return false;
	}

	// verify hash(hdr + data)
	const unsigned char *data = file_data + sizeof(*hdr);
	const unsigned char *trailer = data + len;
	unsigned char md[SHA256_DIGEST_LENGTH];

	SHA256(file_data, sizeof(*hdr) + len, md);
	if (memcmp(md, trailer, SHA256_DIGEST_LENGTH)) {
		*errptr = strdup("checksum mismatch");
		return false;
	}

	return true;
}

