
#include <stdbool.h>
#include <string.h>

#include "pgdb-internal.h"

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

