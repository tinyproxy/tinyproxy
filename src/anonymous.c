/* $Id: anonymous.c,v 1.2 2000-09-11 23:38:36 rjkaes Exp $
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
#  include <config.h>
#endif

#include <sys/types.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#include "anonymous.h"
#include "ternary.h"

static TERNARY anonymous_tree;

int anon_search(char *s)
{
	return ternary_search(anonymous_tree, s, NULL);
}

void anon_insert(char *s)
{
	if (anonymous_tree == 0) {
		if (TE_ISERROR(anonymous_tree = ternary_new()))
			return;
	}

	ternary_insert(anonymous_tree, s, NULL);
}
