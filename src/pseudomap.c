#include "config.h"
#include "pseudomap.h"
#include <strings.h>
#include <stdlib.h>
#include <string.h>

/* this data structure implements a pseudo hashmap.
   tinyproxy originally used a hashmap for keeping the key/value pairs
   in HTTP requests; however later it turned out that items need to be
   returned in order - so we implemented an "orderedmap".
   again, later it turned out that there are are special case headers,
   namely Set-Cookie that can happen more than once, so a hashmap isn't the
   right structure to hold the key-value pairs in HTTP headers.
   it's expected that:
   1) the number of headers in a HTTP request we have to process is
      not big enough to cause a noticable performance drop when we have
      to iterate through our list to find the right header; and
   2) use of plain HTTP is getting exceedingly extinct by the day, so
      in most usecases CONNECT method is used anyway.
*/

/* restrict the number of headers to 256 to prevent an attacker from
   launching a denial of service attack. */
#define MAX_SIZE 256

pseudomap *pseudomap_create(void) {
	return sblist_new(sizeof(struct pseudomap_entry), 64);
}

void pseudomap_destroy(pseudomap *o) {
	if(!o) return;
	while(sblist_getsize(o)) {
		/* retrieve latest element, and "shrink" list in place,
		   so we don't have to constantly rearrange list items
		   by using sblist_delete(). */
		struct pseudomap_entry *e = sblist_get(o, sblist_getsize(o)-1);
		free(e->key);
		free(e->value);
		--o->count;
	}
	sblist_free(o);
}

int pseudomap_append(pseudomap *o, const char *key, char *value ) {
	struct pseudomap_entry e;
	if(sblist_getsize(o) >= MAX_SIZE) return 0;
	e.key = strdup(key);
	e.value = strdup(value);
	if(!e.key || !e.value) goto oom;
	if(!sblist_add(o, &e)) goto oom;
	return 1;
oom:
	free(e.key);
	free(e.value);
	return 0;
}

static size_t pseudomap_find_index(pseudomap *o, const char *key) {
	size_t i;
	struct pseudomap_entry *e;
	for(i = 0; i < sblist_getsize(o); ++i) {
		e = sblist_get(o, i);
		if(!strcasecmp(key, e->key)) return i;
	}
	return (size_t)-1;
}

char* pseudomap_find(pseudomap *o, const char *key) {
	struct pseudomap_entry *e;
	size_t i = pseudomap_find_index(o, key);
	if(i == (size_t)-1) return 0;
	e = sblist_get(o, i);
	return e->value;
}

/* remove *all* entries that match key, to mimic behaviour of hashmap */
int pseudomap_remove(pseudomap *o, const char *key) {
	struct pseudomap_entry *e;
	size_t i;
	int ret = 0;
	while((i = pseudomap_find_index(o, key)) != (size_t)-1) {
		e = sblist_get(o, i);
		free(e->key);
		free(e->value);
		sblist_delete(o, i);
		ret = 1;
	}
	return ret;
}

size_t pseudomap_next(pseudomap *o, size_t iter, char** key, char** value) {
	struct pseudomap_entry *e;
	if(iter >= sblist_getsize(o)) return 0;
	e = sblist_get(o, iter);
	*key = e->key;
	*value = e->value;
	return iter + 1;
}

