/* tinyproxy - A fast light-weight HTTP proxy
 * Copyright (C) 2002 Robert James Kaes <rjkaes@users.sourceforge.net>
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/* See 'hashmap.c' for detailed information. */

#ifndef _HASHMAP_H
#define _HASHMAP_H

#include "common.h"

/*
 * We're using a typedef here to "hide" the implementation details of the
 * hash map.  Sure, it's a pointer, but the struct is hidden in the C file.
 * So, just use the hashmap_t like it's a cookie. :)
 */
typedef struct hashmap_s *hashmap_t;
typedef int hashmap_iter;

/*
 * hashmap_create() takes one argument, which is the number of buckets to
 * use internally.  hashmap_delete() is self explanatory.
 */
extern hashmap_t hashmap_create (unsigned int nbuckets);
extern int hashmap_delete (hashmap_t map);

/*
 * When the you insert a key/data pair into the hashmap it will the key
 * and data are duplicated, so you must free your copy if it was created
 * on the heap.  The key must be a NULL terminated string.  "data" must be
 * non-NULL and length must be greater than zero.
 *
 * Returns: negative on error
 *          0 upon successful insert
 */
extern int hashmap_insert (hashmap_t map, const char *key,
                           const void *data, size_t len);

/*
 * Get an iterator to the first entry.
 *
 * Returns: an negative value upon error.
 */
extern hashmap_iter hashmap_first (hashmap_t map);

/*
 * Checks to see if the iterator is pointing at the "end" of the entries.
 *
 * Returns: 1 if it is the end
 *          0 otherwise
 */
extern int hashmap_is_end (hashmap_t map, hashmap_iter iter);

/*
 * Return a "pointer" to the first instance of the particular key.  It can
 * be tested against hashmap_is_end() to see if the key was not found.
 *
 * Returns: negative upon an error
 *          an "iterator" pointing at the first key
 *          an "end-iterator" if the key wasn't found
 */
extern hashmap_iter hashmap_find (hashmap_t map, const char *key);

/*
 * Retrieve the key/data associated with a particular iterator.
 * NOTE: These are pointers to the actual data, so don't mess around with them
 *       too much.
 *
 * Returns: the length of the data block upon success
 *          negative upon error
 */
extern ssize_t hashmap_return_entry (hashmap_t map, hashmap_iter iter,
                                     char **key, void **data);

/*
 * Get the first entry (assuming there is more than one) for a particular
 * key.  The data MUST be non-NULL.
 *
 * Returns: negative upon error
 *          zero if no entry is found
 *          length of data for the entry
 */
extern ssize_t hashmap_entry_by_key (hashmap_t map, const char *key,
                                     void **data);

/*
 * Searches for _any_ occurrances of "key" within the hashmap and returns the
 * number of matching entries.
 *
 * Returns: negative upon an error
 *          zero if no key is found
 *          count found (positive value)
 */
extern ssize_t hashmap_search (hashmap_t map, const char *key);

/*
 * Go through the hashmap and remove the particular key.
 * NOTE: This will invalidate any iterators which have been created.
 *
 * Remove: negative upon error
 *         0 if the key was not found
 *         positive count of entries deleted
 */
extern ssize_t hashmap_remove (hashmap_t map, const char *key);

#endif /* _HASHMAP_H */
