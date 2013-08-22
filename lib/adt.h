#ifndef __ADT_H__
#define __ADT_H__

#include <sys/types.h>
#include <stdbool.h>

struct dbuffer {
	void		*data;
	size_t		len;
};

struct dstring {
	char		*s;
	size_t		len;
	size_t		alloc_len;
};

struct dlist {
	struct dbuffer	*v;
	size_t		len;
	size_t		alloc_len;
};

extern void dstr_free(struct dstring *dstr);
extern struct dstring *dstr_new(const void *init_str, size_t init_len,
			 size_t alloc_len_);
extern bool dstr_append(struct dstring *dstr, void *s, size_t s_len);

extern void dlist_free(struct dlist *dl, bool free_elements);
extern struct dlist *dlist_new(size_t alloc_len);
extern bool dlist_push(struct dlist *dl, void *data, size_t data_len);

static inline void *dlist_get(struct dlist *dl, unsigned int index,
			      size_t *vallen)
{
	if (index >= dl->len)
		return NULL;
	
	if (vallen)
		*vallen = dl->v[index].len;
	
	return dl->v[index].data;
}

static inline struct dbuffer *dlist_getbuf(struct dlist *dl, unsigned int index)
{
	if (index >= dl->len)
		return NULL;
	
	return &dl->v[index];
}

#endif // __ADT_H__
