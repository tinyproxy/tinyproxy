/* $Id: hashmap.c,v 1.3 2002-04-09 20:05:15 rjkaes Exp $
 *
 * A hashmap implementation.  The keys are case-insensitive NULL terminated
 * strings, and the data is arbitrary lumps of data.  Copies of both the
 * key and the data in the hashmap itself, so you must free the original
 * key and data to avoid a memory leak.  The hashmap returns a pointer
 * to the data when a key is searched for, so take care in modifying the
 * data as it's modifying the data stored in the hashmap.  (In other words,
 * don't try to free the data, or realloc the memory. :)
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#if defined(HAVE_STDINT_H)
#  include <stdint.h>
#endif
#if defined(HAVE_INTTYPES_H)
#  include <inttypes.h>
#endif

#if defined(HAVE_CTYPE_H)
#  include <ctype.h>
#endif
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "hashmap.h"
#include "vector.h"

/*
 * These structures are the storage for the hashmap.  Entries are stored in
 * struct hashentry_s (the key, data, and length), and all the "buckets" are
 * grouped together in hashmap_s.  The hashmap_s.size member is for
 * internal use.  It stores the number of buckets the hashmap was created
 * with.
 */
struct hashentry_s {
	char *key;
	void *data;
	size_t len;

	struct hashentry_s *prev, *next;
};
struct hashmap_s {
	unsigned int size;
	struct hashentry_s **maps;
};

/*
 * A NULL terminated string is passed to this function and a "hash" value
 * is produced within the range of [0 .. size)  (In other words, 0 to one
 * less than size.)
 * The contents of the key are converted to lowercase, so this function
 * is not case-sensitive.
 *
 * If any of the arguments are invalid a negative number is returned.
 */
static int
hashfunc(const char *key, unsigned int size)
{
	uint32_t hash;

	if (key == NULL)
		return -EINVAL;
	if (size == 0)
		return -ERANGE;
     
	for (hash = tolower(*key++); *key != '\0'; key++) {
		uint32_t bit =
			(hash & 1) ? (1 << (sizeof(uint32_t) - 1)) : 0;
		hash >>= 1;

		hash += tolower(*key) + bit;
	}

	/* Keep the hash within the table limits */
	return hash % size;
}

/*
 * Create a hashmap with the requested number of buckets.  If "nbuckets" is
 * not greater than zero a NULL is returned; otherwise, a _token_ to the
 * hashmap is returned.
 *
 * NULLs are also returned if memory could not be allocated for hashmap.
 */
hashmap_t
hashmap_create(unsigned int nbuckets)
{
	struct hashmap_s* ptr;

	if (nbuckets == 0)
		return NULL;

	ptr = calloc(1, sizeof(struct hashmap_s));
	if (!ptr)
		return NULL;

	ptr->size = nbuckets;
	ptr->maps = calloc(nbuckets, sizeof(struct hashentry_s *));
	if (!ptr->maps) {
		free(ptr);
		return NULL;
	}

	return ptr;
}

/*
 * Follow the chain of hashentries and delete them (including the data and
 * the key.)
 *
 * Returns: 0 if the function completed successfully
 *          negative number is returned if "entry" was NULL
 */
static inline int
delete_hashentries(struct hashentry_s* entry)
{
	struct hashentry_s *nextptr;
	struct hashentry_s *ptr;

	if (entry == NULL)
		return -EINVAL;

	ptr = entry;
	while (ptr) {
		nextptr = ptr->next;
		
		free(ptr->key);
		free(ptr->data);
		free(ptr);

		ptr = nextptr;
	}

	return 0;
}

/*
 * Deletes a hashmap.  All the key/data pairs are also deleted.
 *
 * Returns: 0 on success
 *          negative if a NULL "map" was supplied
 */
int
hashmap_delete(hashmap_t map)
{
	unsigned int i;
   
	if (map == NULL)
		return -EINVAL;

	for (i = 0; i < map->size; i++) {
		if (map->maps[i] != NULL)
			delete_hashentries(map->maps[i]);
	}

	free(map);

	return 0;
}

/*
 * Inserts a NULL terminated string (as the key), plus any arbitrary "data"
 * of "len" bytes.  Both the key and the data are copied, so the original
 * key/data must be freed to avoid a memory leak.
 * The "data" must be non-NULL and "len" must be greater than zero.  You
 * cannot insert NULL data in association with the key.
 *
 * Returns: 0 on success
 *          negative number if there are errors
 */
int
hashmap_insert(hashmap_t map, const char *key,
	       const void *data, size_t len)
{
	struct hashentry_s *ptr, *prevptr;
	int hash;
	char *key_copy;
	void *data_copy;

	if (map == NULL || key == NULL)
		return -EINVAL;
	if (!data || len < 1)
		return -ERANGE;

	hash = hashfunc(key, map->size);
	if (hash < 0)
		return hash;

	ptr = map->maps[hash];

	/*
	 * First make copies of the key and data in case there is a memory
	 * problem later.
	 */
	key_copy = strdup(key);
	if (!key_copy)
		return -ENOMEM;

	if (data) {
		data_copy = malloc(len);
		if (!data_copy) {
			free(key_copy);
			return -ENOMEM;
		}
		memcpy(data_copy, data, len);
	} else {
		data_copy = NULL;
	}

	if (ptr) {
		/* There is already an entry, so tack onto the end */
		while (ptr) {
			prevptr = ptr;
			ptr = ptr->next;
		}

		ptr = calloc(1, sizeof(struct hashentry_s));
		ptr->prev = prevptr;
		prevptr->next = ptr;
	} else {
		ptr = map->maps[hash] = calloc(1, sizeof(struct hashentry_s));
	}

	if (!ptr) {
		free(key_copy);
		free(data_copy);
		return -ENOMEM;
	}

	ptr->key = key_copy;
	ptr->data = data_copy;
	ptr->len = len;

	return 0;
}

/*
 * A pointer to data is returned based on a case-insensitive NULL terminated
 * "key".  If the "key" is not found, "data" is set to NULL.  A NULL "data"
 * argument indicates that the data associated with the key is to be ignored.
 *
 * Returns: negative upon an error
 *          zero if no key is found
 *          length of data if key is found.
 */
ssize_t
hashmap_search(hashmap_t map, const char *key, void **data)
{
	int hash;
	struct hashentry_s* ptr;

	if (map == NULL || key == NULL)
		return -EINVAL;

	hash = hashfunc(key, map->size);
	if (hash < 0)
		return hash;

	ptr = map->maps[hash];

	/* Okay, there is an entry here, now see if it's the one we want */
	while (ptr) {
		if (strcasecmp(ptr->key, key) == 0) {
			/* Found it, return a pointer to the data */
			if (data)
				*data = ptr->data;
			return ptr->len;
		}

		/* This entry didn't contain the key; move to the next one */
		ptr = ptr->next;
	}

	/* The key was not found, so return NULL */
	if (data)
		*data = NULL;
	return 0;
}

/*
 * Produce a vector of all the keys in the hashmap.
 *
 * Returns: NULL upon error
 *          a valid vector_t if everything is fine
 */
vector_t
hashmap_keys(hashmap_t map)
{
	vector_t vector;
	unsigned int i;
	struct hashentry_s *ptr;

	if (!map)
		return NULL;

	vector = vector_create();
	if (!vector)
		return NULL;

	/*
	 * Iterate through all the entries and add the keys to the
	 * vector.
	 */
	for (i = 0; i < map->size; i++) {
		ptr = map->maps[i];

		while (ptr) {
			if (vector_insert(vector, ptr->key, strlen(ptr->key) + 1) < 0) {
				/* There's a problem, so delete the vector */
				vector_delete(vector);
				return NULL;
			}

			ptr = ptr->next;
		}
	}

	return vector;
}

/*
 * Go through the hashmap and remove the particular key.
 * NOTE: This _will_ invalidate any vectors which might have been created
 *       by the hashmap_keys() function.
 *
 * Remove: negative upon error
 *         0 if the key was not found
 *         1 if the entry was deleted
 */
ssize_t
hashmap_remove(hashmap_t map, const char *key)
{
	int hash;
	struct hashentry_s* ptr;

	if (map == NULL || key == NULL)
		return -EINVAL;

	hash = hashfunc(key, map->size);
	if (hash < 0)
		return hash;

	ptr = map->maps[hash];

	while (ptr) {
		if (strcasecmp(ptr->key, key) == 0) {
			/*
			 * Found the data, now need to remove everything
			 * and update the hashmap.
			 */
			struct hashentry_s* prevptr = ptr->prev;
			if (prevptr) {
				prevptr->next = ptr->next;
				if (ptr->next)
					ptr->next->prev = prevptr;
			} else {
				map->maps[hash] = ptr->next;
				ptr->prev = NULL;
			}

			free(ptr->key);
			free(ptr->data);
			free(ptr);

			return 1;
		}

		/* This entry didn't contain the key; move to the next one */
		ptr = ptr->next;
	}

	/* The key was not found, so return 0 */
	return 0;
}
