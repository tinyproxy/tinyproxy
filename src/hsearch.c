/*
musl license, hsearch.c originally written by Szabolcs Nagy

Copyright © 2005-2020 Rich Felker, et al.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <stdlib.h>
#include <string.h>
#include "hsearch.h"

/*
open addressing hash table with 2^n table size
quadratic probing is used in case of hash collision
tab indices and hash are size_t
after resize fails with ENOMEM the state of tab is still usable
*/

typedef struct htab_entry {
	char *key;
	htab_value data;
} htab_entry;

struct elem {
	htab_entry item;
	size_t hash;
};

struct htab {
	struct elem *elems;
	size_t mask;
	size_t used;
	size_t seed;
	size_t dead;
};

#define MINSIZE 8
#define MAXSIZE ((size_t)-1/2 + 1)

#define CASE_INSENSITIVE
#ifdef CASE_INSENSITIVE
#include <ctype.h>
#include <strings.h>
#define LOWER_OR_NOT(X) tolower(X)
#define STRCMP(X, Y) strcasecmp(X, Y)
#else
#define LOWER_OR_NOT(X) X
#define STRCMP(X, Y) strcmp(X, Y)
#endif

static size_t keyhash(const char *k, size_t seed)
{
	const unsigned char *p = (const void *)k;
	size_t h = seed;

	while (*p)
		h = 31*h + LOWER_OR_NOT(*p++);
	return h;
}

static int resize(struct htab *htab, size_t nel)
{
	size_t newsize;
	size_t i, j;
	struct elem *e, *newe;
	struct elem *oldtab = htab->elems;
	struct elem *oldend = htab->elems + htab->mask + 1;

	if (nel > MAXSIZE)
		nel = MAXSIZE;
	for (newsize = MINSIZE; newsize < nel; newsize *= 2);
	htab->elems = calloc(newsize, sizeof *htab->elems);
	if (!htab->elems) {
		htab->elems = oldtab;
		return 0;
	}
	htab->mask = newsize - 1;
	if (!oldtab)
		return 1;
	for (e = oldtab; e < oldend; e++)
		if (e->item.key) {
			for (i=e->hash,j=1; ; i+=j++) {
				newe = htab->elems + (i & htab->mask);
				if (!newe->item.key)
					break;
			}
			*newe = *e;
		}
	free(oldtab);
	return 1;
}

static struct elem *lookup(struct htab *htab, const char *key, size_t hash, size_t dead)
{
	size_t i, j;
	struct elem *e;

	for (i=hash,j=1; ; i+=j++) {
		e = htab->elems + (i & htab->mask);
		if ((!e->item.key && (!e->hash || e->hash == dead)) ||
		    (e->hash==hash && STRCMP(e->item.key, key)==0))
			break;
	}
	return e;
}

struct htab *htab_create(size_t nel)
{
	struct htab *r = calloc(1, sizeof *r);
	if(r && !resize(r, nel)) {
		free(r);
		r = 0;
	}
	r->seed = rand();
	return r;
}

void htab_destroy(struct htab *htab)
{
	free(htab->elems);
	free(htab);
}

static struct elem *htab_find_elem(struct htab *htab, const char* key)
{
	size_t hash = keyhash(key, htab->seed);
	struct elem *e = lookup(htab, key, hash, 0);

	if (e->item.key) {
		return e;
	}
	return 0;
}

htab_value* htab_find(struct htab *htab, const char* key)
{
	struct elem *e = htab_find_elem(htab, key);
	if(!e) return 0;
	return &e->item.data;
}

htab_value* htab_find2(struct htab *htab, const char* key, char **saved_key)
{
	struct elem *e = htab_find_elem(htab, key);
	if(!e) return 0;
	*saved_key = e->item.key;
	return &e->item.data;
}

int htab_delete(struct htab *htab, const char* key)
{
	struct elem *e = htab_find_elem(htab, key);
	if(!e) return 0;
	e->item.key = 0;
	e->hash = 0xdeadc0de;
	--htab->used;
	++htab->dead;
	return 1;
}

int htab_insert(struct htab *htab, char* key, htab_value value)
{
	size_t hash = keyhash(key, htab->seed), oh;
	struct elem *e = lookup(htab, key, hash, 0xdeadc0de);
	if(e->item.key) {
		/* it's not allowed to overwrite existing data */
		return 0;
	}

	oh = e->hash; /* save old hash in case it's tombstone marker */
	e->item.key = key;
	e->item.data = value;
	e->hash = hash;
	if (++htab->used + htab->dead > htab->mask - htab->mask/4) {
		if (!resize(htab, 2*htab->used)) {
			htab->used--;
			e->item.key = 0;
			e->hash = oh;
			return 0;
		}
		htab->dead = 0;
	} else if (oh == 0xdeadc0de) {
		/* re-used tomb */
		--htab->dead;
	}
	return 1;
}

size_t htab_next(struct htab *htab, size_t iterator, char** key, htab_value **v)
{
	size_t i;
	for(i=iterator;i<htab->mask+1;++i) {
		struct elem *e = htab->elems + i;
		if(e->item.key) {
			*key = e->item.key;
			*v = &e->item.data;
			return i+1;
		}
	}
	return 0;
}
