/* $Id: hashmap.h,v 1.1 2002-04-07 21:30:02 rjkaes Exp $
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

#ifndef _HASHMAP_H
#define _HASHMAP_H

#include "vector.h"

/* Allow the use in C++ code. */
#if defined(__cplusplus)
extern "C" {
#endif

/*
 * We're using a typedef here to "hide" the implementation details of the
 * hash map.  Sure, it's a pointer, but the struct is hidden in the C file.
 * So, just use the hashmap_t like it's a cookie. :)
 */
typedef struct hashmap_s* hashmap_t;

/*
 * hashmap_create() takes one argument, which is the number of buckets to
 * use internally.  hashmap_delete() is self explanatory.
 */
extern hashmap_t hashmap_create(unsigned int nbuckets);
extern int hashmap_delete(hashmap_t map);

/*
 * When the you insert a key/data pair into the hashmap it will the key
 * and data are duplicated, so you must free your copy if it was created
 * on the heap.  The key must be a NULL terminated string.  "data" must be
 * non-NULL and length must be greater than zero.
 *
 * Returns: negative on error
 *          0 upon successful insert
 */
extern int hashmap_insert(hashmap_t map, const char *key,
			  const void *data, size_t len);

/*
 * If a valid key is found in the hash map you will get a pointer to the
 * data stored in the hash map.  In other words, you have the power to change
 * the data a key is associated with, but do it responsibly since the
 * library doesn't take any steps to prevent you from messing up the hash
 * map.  Don't try to realloc or free the data though; doing so will break
 * the hashmap.  If you are only interested in whether the key is present
 * or not, supply a NULL for the "data" argument.
 *
 * Returns: negative on error
 *          zero if the key was not found
 *          positive (length of data) if key is found
 */
extern ssize_t hashmap_search(hashmap_t map, const char *key, void **data);

/*
 * Produce a vector of all the keys in the hashmap.
 *
 * Returns: NULL upon error
 *          a valid vector_t if everything is fine
 */
extern vector_t hashmap_keys(hashmap_t map);

/*
 * Go through the hashmap and remove the particular key.
 * NOTE: This _will_ invalidate any vectors which might have been created
 *       by the hashmap_keys() function.
 *
 * Remove: negative upon error
 *         0 if the key was not found
 *         1 if the entry was deleted
 */
extern ssize_t hashmap_remove(hashmap_t map, const char *key);

#if defined(__cplusplus)
}
#endif /* C++ */

#endif /* _HASHMAP_H */
