/* $Id: anonymous.c,v 1.1 2000-03-31 19:56:55 rjkaes Exp $
 *
 * Handles insertion and searches for headers which should be let through when
 * the anonymous feature is turned on. The headers are stored in a Ternary
 * Search Tree. The initial code came from Dr. Dobb's Journal, April 1998
 * "Ternary Search Trees", Jon Bentley and Bob Sedgewick, pg 20-25.
 *
 * Copyright (C) 2000  Robert James Kaes (rjkaes@flarenet.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#ifdef HAVE_CONFIG_H
#include <defines.h>
#endif

#include <stdlib.h>
#include <ctype.h>

#include "anonymous.h"

typedef struct tnode *Tptr;
typedef struct tnode {
	char splitchar;
	Tptr lokid, eqkid, hikid;
} Tnode;

static Tptr anon_root = NULL;

static Tptr intern_insert(Tptr p, char *s)
{
	if (p == NULL) {
		p = (Tptr) malloc(sizeof(Tnode));
		p->splitchar = tolower(*s);
		p->lokid = p->eqkid = p->hikid = NULL;
	}

	if (tolower(*s) < p->splitchar)
		p->lokid = intern_insert(p->lokid, s);
	else if (tolower(*s) == p->splitchar) {
		if (tolower(*s) != 0)
			p->eqkid = intern_insert(p->eqkid, ++s);
	} else
		p->hikid = intern_insert(p->hikid, s);

	return p;
}

int anon_search(char *s)
{
	Tptr p = anon_root;
	
	while (p) {
		if (tolower(*s) < p->splitchar)
			p = p->lokid;
		else if (tolower(*s) == p->splitchar) {
			if (*s++ == 0)
				return 1;
			p = p->eqkid;
		} else
			p = p->hikid;
	}

	return 0;
}

void anon_insert(char *s)
{
	anon_root = intern_insert(anon_root, s);
}
