/* $Id: anonymous.c,v 1.6 2001-08-26 21:07:27 rjkaes Exp $
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
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#include "anonymous.h"
#include "log.h"
#include "ternary.h"
#include "tinyproxy.h"

static TERNARY anonymous_tree = 0;
/*
 * Keep track of whether the Anonymous filtering is enabled. Off by
 * default.
 */
static short int anonymous_is_enabled = 0;

short int is_anonymous_enabled(void)
{
	return anonymous_is_enabled;
}

int anonymous_search(char *s)
{
	assert(s != NULL);
	assert(anonymous_is_enabled == 1);
	assert(anonymous_tree > 0);

	return ternary_search(anonymous_tree, s, NULL);
}

int anonymous_insert(char *s)
{
	assert(s != NULL);

	/*
	 * If this is the first time we're inserting a word, create the
	 * search tree.
	 */
	if (!anonymous_is_enabled) {
		anonymous_tree = ternary_new();
		if (anonymous_tree < 0)
			return -1;

		anonymous_is_enabled = 1;

		DEBUG1("Starting the Anonymous header subsytem.");
	}

	return ternary_insert(anonymous_tree, s, NULL);
}
