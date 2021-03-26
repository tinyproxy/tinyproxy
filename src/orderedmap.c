#ifdef HAVE_CONFIG_H
#  include <config.h>
#else
#  define _ALL_SOURCE
#  define _GNU_SOURCE
#endif
#include <string.h>
#include "sblist.h"
#include "orderedmap.h"

static void orderedmap_destroy_contents(struct orderedmap *o) {
	char **p, *q;
	size_t i;
	htab_value *v;
	if(!o) return;
	if(o->values) {
		while(sblist_getsize(o->values)) {
			p = sblist_get(o->values, 0);
			if(p) free(*p);
			sblist_delete(o->values, 0);
		}
		sblist_free(o->values);
	}
	if(o->map) {
		i = 0;
		while((i = htab_next(o->map, i, &q, &v)))
			free(q);
		htab_destroy(o->map);
	}
}

struct orderedmap *orderedmap_create(size_t nbuckets) {
	struct orderedmap o = {0}, *new;
	o.values = sblist_new(sizeof(void*), 32);
	if(!o.values) goto oom;
	o.map = htab_create(nbuckets);
	if(!o.map) goto oom;
	new = malloc(sizeof o);
	if(!new) goto oom;
	memcpy(new, &o, sizeof o);
	return new;
	oom:;
	orderedmap_destroy_contents(&o);
	return 0;
}

void* orderedmap_destroy(struct orderedmap *o) {
	orderedmap_destroy_contents(o);
	free(o);
	return 0;
}

int orderedmap_append(struct orderedmap *o, const char *key, char *value) {
	size_t index;
	char *nk, *nv;
	nk = nv = 0;
	nk = strdup(key);
	nv = strdup(value);
	if(!nk || !nv) goto oom;
	index = sblist_getsize(o->values);
	if(!sblist_add(o->values, &nv)) goto oom;
	if(!htab_insert(o->map, nk, HTV_N(index))) {
		sblist_delete(o->values, index);
		goto oom;
	}
	return 1;
oom:;
	free(nk);
	free(nv);
	return 0;
}

char* orderedmap_find(struct orderedmap *o, const char *key) {
	char **p;
	htab_value *v = htab_find(o->map, key);
	if(!v) return 0;
	p = sblist_get(o->values, v->n);
	return p?*p:0;
}

int orderedmap_remove(struct orderedmap *o, const char *key) {
	size_t i;
	char *lk;
	char *sk;
	char **sv;
	htab_value *lv, *v = htab_find2(o->map, key, &sk);
	if(!v) return 0;
	sv = sblist_get(o->values, v->n);
	free(*sv);
	sblist_delete(o->values, v->n);
	i = 0;
	while((i = htab_next(o->map, i, &lk, &lv))) {
		if(lv->n > v->n) lv->n--;
	}
	htab_delete(o->map, key);
	free(sk);
	return 1;
}

size_t orderedmap_next(struct orderedmap *o, size_t iter, char** key, char** value) {
	size_t h_iter;
	htab_value* hval;
	char **p;
	if(iter < sblist_getsize(o->values)) {
		h_iter = 0;
		while((h_iter = htab_next(o->map, h_iter, key, &hval))) {
			if(hval->n == iter) {
				p = sblist_get(o->values, iter);
				*value = p?*p:0;
				return iter+1;
			}
		}
	}
	return 0;
}
