
#include <string.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
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

bool pg_iterate_dir(const char *dirname,
		 bool (*actor)(const struct dirent *de, void *priv,
		 	       char **errptr),
		 void *priv, char **errptr)
{
	DIR *dir;
	struct dirent *de;

	*errptr = NULL;

	// ideally we use O_DIRECTORY, fchdir() etc.

	dir = opendir(dirname);
	if (!dir) {
		*errptr = strdup(strerror(errno));
		return false;
	}

	bool arc = true;
	while ((de = readdir(dir)) != NULL) {
		if ((!strcmp(de->d_name, ".")) ||
		    (!strcmp(de->d_name, "..")))
			continue;


		arc = actor(de, priv, errptr);
		if (!arc)
			break;
	}

	closedir(dir);

	return arc;
}

bool pg_uuid(pg_uuid_t uuid)
{
	unsigned char *s = uuid;
	if (!pg_rand_bytes(s, sizeof(pg_uuid_t)))
		return false;
	
	s[6] = 0x40 | (s[6] & 0xf);	// hi = 4
	s[8] = 0x80 | (s[8] & 0x3f);	// hi = 8/9/a/b

	return true;
}

static const char _hexchars[0x10] = "0123456789abcdef";

static void bin2hex(char *out, const void *in, size_t len)
{
	const unsigned char *p = in;
	while (len--)
	{
		(out++)[0] = _hexchars[p[0] >> 4];
		(out++)[0] = _hexchars[p[0] & 0xf];
		++p;
	}
	out[0] = '\0';
}

void pg_uuid_str(char *uuid, const pg_uuid_t uuid_in)
{
	char hexstr[64];
	bin2hex(hexstr, uuid_in, sizeof(pg_uuid_t));

	memset(uuid, '\0', 36+1);
	strncpy(uuid, hexstr, 8);
	strcat(uuid, "-");
	strncat(uuid, &hexstr[8], 4);
	strcat(uuid, "-");
	strncat(uuid, &hexstr[12], 4);
	strcat(uuid, "-");
	strncat(uuid, &hexstr[16], 4);
	strcat(uuid, "-");
	strncat(uuid, &hexstr[20], 12);
}

