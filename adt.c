
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "adt.h"

#define unlikely(x)     __builtin_expect(!!(x), 0)

void dstr_free(struct dstring *dstr)
{
	if (!dstr)
		return;
	
	free(dstr->s);
	free(dstr);
}

struct dstring *dstr_new(const void *init_str, size_t init_len,
			 size_t alloc_len_)
{
	size_t is_len, alloc_len;

	if (init_str) {
		is_len = init_len ? init_len : strlen(init_str) + 1;
		alloc_len = is_len;
	} else {
		is_len = 0;
		alloc_len = 0;
	}
	if (alloc_len < 32)
		alloc_len = 32;
	if (alloc_len < alloc_len_)
		alloc_len = alloc_len_;

	struct dstring *dstr = malloc(sizeof(struct dstring));
	if (unlikely(!dstr))
		return NULL;

	dstr->s = calloc(1, alloc_len);
	if (unlikely(!dstr->s)) {
		free(dstr);
		return NULL;
	}

	if (init_str)
		memcpy(dstr->s, init_str, is_len);

	dstr->alloc_len = alloc_len;

	return dstr;
}

static bool dstr_grow(struct dstring *dstr, unsigned int target)
{
	unsigned int new_alloc_len = 32;
	do {
		new_alloc_len <<= 1;
	} while (new_alloc_len < target);

	void *new_mem = calloc(1, new_alloc_len);
	if (unlikely(!new_mem))
		return false;
	
	memcpy(new_mem, dstr->s, dstr->len);

	free(dstr->s);

	dstr->s = new_mem;
	dstr->alloc_len = new_alloc_len;

	return true;
}

bool dstr_append(struct dstring *dstr, void *s, size_t s_len)
{
	if (!s_len)
		s_len = strlen(s);

	unsigned int wanted = dstr->len + s_len + 1;
	if ((wanted < dstr->alloc_len) &&
	    !dstr_grow(dstr, wanted))
		return false;
		
	memcpy(&dstr->s[dstr->len], s, s_len);
	dstr->s[dstr->len + s_len] = 0;
	dstr->len += s_len;
	return true;
}

void dlist_free(struct dlist *dl, bool free_elements)
{
	if (!dl)
		return;

	if (free_elements) {
		unsigned int i;
		for (i = 0; i < dl->len; i++)
			free(dl->v[i].data);
	}

	free(dl->v);
	free(dl);
}

struct dlist *dlist_new(size_t alloc_len)
{
	if (alloc_len < 32)
		alloc_len = 32;
	
	struct dlist *dl = calloc(1, sizeof(struct dlist));
	if (unlikely(!dl))
		return NULL;
	
	dl->v = calloc(alloc_len, sizeof(struct dbuffer));
	if (unlikely(!dl->v)) {
		free(dl);
		return NULL;
	}

	return dl;
}

static bool dlist_grow(struct dlist *dl)
{
	unsigned int new_alloc_len = dl->alloc_len * 2;
	void *new_mem = calloc(new_alloc_len, sizeof(struct dbuffer));
	if (unlikely(!new_mem))
		return false;

	memcpy(new_mem, dl->v, dl->len * sizeof(struct dbuffer));

	free(dl->v);

	dl->v = new_mem;
	dl->alloc_len = new_alloc_len;
	
	return true;
}

bool dlist_append(struct dlist *dl, void *data, size_t data_len)
{
	if ((dl->len == dl->alloc_len) &&
	    !dlist_grow(dl))
		return false;

	struct dbuffer *buf = &dl->v[dl->len];
	buf->data = data;
	buf->len = data_len;
	dl->len++;

	return true;
}

