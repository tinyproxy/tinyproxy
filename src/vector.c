/* $Id: vector.c,v 1.4 2002-05-13 23:32:16 rjkaes Exp $
 *
 * A vector implementation.  The vector can be of an arbritrary length, and
 * the data for each entry is an lump of data (the size is stored in the
 * vector.)
 *
 * Copyright (C) 2002  Robert James Kaes (rjkaes@flarenet.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "tinyproxy.h"

#include "vector.h"
#include "utils.h"

/*
 * These structures are the storage for the "vector".  Entries are
 * stored in struct vectorentry_s (the data and the length), and the
 * "vector" structure is implemented as a linked-list.  The struct
 * vector_s stores a pointer to the first vector (vector[0]) and a
 * count of the number of enteries (or how long the vector is.)
 */
struct vectorentry_s {
	void *data;
	size_t len;

	struct vectorentry_s *next;
};

struct vector_s {
	size_t num_entries;
	struct vectorentry_s *head;
	struct vectorentry_s *tail;
};

/*
 * Create an vector.  The vector initially has no elements and no
 * storage has been allocated for the entries.
 *
 * A NULL is returned if memory could not be allocated for the
 * vector.
 */
vector_t
vector_create(void)
{
	vector_t vector;

	vector = safemalloc(sizeof(struct vector_s));
	if (!vector)
		return NULL;

	vector->num_entries = 0;
	vector->head = vector->tail = NULL;
	
	return vector;
}

/*
 * Deletes an vector.  All the enteries when this function is run.
 *
 * Returns: 0 on success
 *          negative if a NULL vector is supplied
 */
int
vector_delete(vector_t vector)
{
	struct vectorentry_s *ptr, *next;

	if (!vector)
		return -EINVAL;
       
	ptr = vector->head;
	while (ptr) {
		next = ptr->next;
		safefree(ptr->data);
		safefree(ptr);

		ptr = next;
	}

	safefree(vector);

	return 0;
}

/*
 * Inserts an entry into the vector.  The entry is an arbitrary
 * collection of bytes of _len_ octets.  The data is copied into the
 * vector, so the original data must be freed to avoid a memory leak.
 * The "data" must be non-NULL and the "len" must be greater than zero.
 *
 * Returns: 0 on success
 *          negative number if there are errors
 */
int
vector_insert(vector_t vector, void *data, ssize_t len)
{
	struct vectorentry_s *entry;

	if (!vector || !data || len <= 0)
		return -EINVAL;

	entry = safemalloc(sizeof(struct vectorentry_s));
	if (!entry)
		return -ENOMEM;

	entry->data = safemalloc(len);
	if (!entry->data) {
		safefree(entry);
		return -ENOMEM;
	}

	memcpy(entry->data, data, len);
	entry->len = len;
	entry->next = NULL;

	if (vector->tail) {
		/* If "tail" is not NULL, it points to the last entry */
		vector->tail->next = entry;
		vector->tail = entry;
	} else {
		/* No "tail", meaning no entries.  Create both. */
		vector->head = vector->tail = entry;
	}

	vector->num_entries++;

	return 0;
}

/*
 * A pointer to the data at position "pos" (zero based) is returned in the
 * "data" pointer.  If the vector is out of bound, data is set to NULL.
 *
 * Returns: negative upon an error
 *          length of data if position is valid
 */
ssize_t
vector_getentry(vector_t vector, size_t pos, void **data)
{
	struct vectorentry_s *ptr;
	size_t loc;

	if (!vector || !data)
		return -EINVAL;

	if (pos < 0 || pos >= vector->num_entries)
		return -ERANGE;

	loc = 0;
	ptr = vector->head;

	while (loc != pos) {
		ptr = ptr->next;
		loc++;
	}

	*data = ptr->data;
	return ptr->len;
}

/*
 * Returns the number of enteries (or the length) of the vector.
 *
 * Returns: negative if vector is not valid
 *          positive length of vector otherwise
 */
ssize_t
vector_length(vector_t vector)
{
	if (!vector)
		return -EINVAL;

	return vector->num_entries;
}
