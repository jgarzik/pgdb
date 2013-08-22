
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "adt.h"

#define CHECK(cond) { if (!(cond)) exit(1); }

static void test_dstring(void)
{
	struct dstring *s = dstr_new("Hello", 0, 0);
	CHECK(s != NULL);
	CHECK(s->len == 5);
	CHECK(!strcmp(s->s, "Hello"));

	bool rc = dstr_append(s, " world", 0);
	CHECK(rc);
	CHECK(!strcmp(s->s, "Hello world"));

	dstr_free(s);
}

static void test_dlist(void)
{
	struct dlist *l = dlist_new(0, free);
	CHECK(l != NULL);
	CHECK(l->len == 0);

	char *s = strdup("one");
	dlist_push(l, s, 0);
	CHECK(l->len == 1);

	s = strdup("two");
	dlist_push(l, s, 0);
	CHECK(l->len == 2);

	s = dlist_get(l, 0, NULL);
	CHECK(!strcmp(s, "one"));

	struct dbuffer *buf = dlist_buf(l, 1);
	CHECK(!strcmp(buf->data, "two"));

	dlist_free(l);
}

int main (int argc, char *argv[])
{
	test_dstring();
	test_dlist();
	return 0;
}

