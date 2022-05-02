#ifndef PSEUDOMAP_H
#define PSEUDOMAP_H

#include <stdlib.h>
#include "sblist.h"

struct pseudomap_entry {
	char *key;
	char *value;
};

typedef sblist pseudomap;

pseudomap *pseudomap_create(void);
void pseudomap_destroy(pseudomap *o);
int pseudomap_append(pseudomap *o, const char *key, char *value );
char* pseudomap_find(pseudomap *o, const char *key);
int pseudomap_remove(pseudomap *o, const char *key);
size_t pseudomap_next(pseudomap *o, size_t iter, char** key, char** value);

#endif

