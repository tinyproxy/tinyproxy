/* $Id: anonymous.c,v 1.14 2002-05-23 18:20:27 rjkaes Exp $
 *
 * Handles insertion and searches for headers which should be let through when
 * the anonymous feature is turned on.
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

#include "tinyproxy.h"

#include "anonymous.h"
#include "hashmap.h"
#include "heap.h"
#include "log.h"

static hashmap_t anonymous_map = NULL;

short int
is_anonymous_enabled(void)
{
	return (anonymous_map != NULL) ? 1 : 0;
}

/*
 * Search for the header.  This function returns a positive value greater than
 * zero if the string was found, zero if it wasn't and negative upon error.
 */
int
anonymous_search(char *s)
{
	assert(s != NULL);
	assert(anonymous_map != NULL);

	return hashmap_search(anonymous_map, s);
}

/*
 * Insert a new header.
 *
 * Return -1 if there is an error, otherwise a 0 is returned if the insert was
 * successful.
 */
int
anonymous_insert(char *s)
{
	char data = 1;

	assert(s != NULL);

	if (!anonymous_map) {
		anonymous_map = hashmap_create(32);
		if (!anonymous_map)
			return -1;
	}

	if (hashmap_search(anonymous_map, s) > 0) {
		/* The key was already found, so return a positive number. */
		return 0;
	}

	/* Insert the new key */
	return hashmap_insert(anonymous_map, s, &data, sizeof(data));
}
