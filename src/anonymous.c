/* $Id: anonymous.c,v 1.10 2001-12-15 20:02:59 rjkaes Exp $
 *
 * Handles insertion and searches for headers which should be let through when
 * the anonymous feature is turned on. The headers are stored in a linked
 * list.
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
#include "log.h"
#include "utils.h"

/*
 * Structure holding the various anonymous headers.
 */
struct anonymous_header_s {
	char *header;
	struct anonymous_header_s *next;
};
struct anonymous_header_s *anonymous_root = NULL;

inline short int
is_anonymous_enabled(void)
{
	return (anonymous_root) ? 1 : 0;
}

/*
 * Search through the linked list for the header. If it's found return 0
 * else return -1.
 */
int
anonymous_search(char *s)
{
	struct anonymous_header_s *ptr = anonymous_root;

	assert(s != NULL);
	assert(anonymous_root != NULL);

	while (ptr) {
		if (ptr->header) {
			if (strcasecmp(ptr->header, s) == 0)
				return 0;
		} else
			return -1;

		ptr = ptr->next;
	}

	return -1;
}

/*
 * Insert a new header into the linked list.
 *
 * Return -1 if there is an error, 0 if the string already exists, and 1 if
 * it's been inserted.
 */
int
anonymous_insert(char *s)
{
	struct anonymous_header_s *ptr;
	struct anonymous_header_s **prev_ptr;

	assert(s != NULL);

	if (!anonymous_root) {
		anonymous_root = safemalloc(sizeof(struct anonymous_header_s));
		if (!anonymous_root)
			return -1;

		anonymous_root->header = strdup(s);
		anonymous_root->next = NULL;
	} else {
		ptr = anonymous_root;

		while (ptr) {
			if (ptr->header) {
				if (strcasecmp(ptr->header, s) == 0)
					return 0;
			}

			prev_ptr = &ptr;
			ptr = ptr->next;
		}

		ptr = (*prev_ptr)->next
			= safemalloc(sizeof(struct anonymous_header_s));
		if (!ptr)
			return -1;

		ptr->header = strdup(s);
		ptr->next = NULL;
	}
		
	return 1;
}
