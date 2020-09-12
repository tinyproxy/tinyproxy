#ifndef ORDEREDMAP_H
#define ORDEREDMAP_H

#include <stdlib.h>
#include "sblist.h"
#include "hsearch.h"

typedef struct orderedmap {
	sblist* values;
	struct htab *map;
} *orderedmap;

struct orderedmap *orderedmap_create(size_t nbuckets);
void* orderedmap_destroy(struct orderedmap *o);
int orderedmap_append(struct orderedmap *o, const char *key, char *value );
char* orderedmap_find(struct orderedmap *o, const char *key);
int orderedmap_remove(struct orderedmap *o, const char *key);
size_t orderedmap_next(struct orderedmap *o, size_t iter, char** key, char** value);

#endif
