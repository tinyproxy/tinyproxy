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

/* A hashmap implementation.  The keys are case-insensitive NULL terminated
 * strings, and the data is arbitrary lumps of data.  Copies of both the
 * key and the data in the hashmap itself, so you must free the original
 * key and data to avoid a memory leak.  The hashmap returns a pointer
 * to the data when a key is searched for, so take care in modifying the
 * data as it's modifying the data stored in the hashmap.  (In other words,
 * don't try to free the data, or realloc the memory. :)
 */

#include "main.h"

#include "hashmap.h"
#include "heap.h"

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

struct hashbucket_s {
        struct hashentry_s *head, *tail;
};

struct hashmap_s {
        unsigned int size;
        hashmap_iter end_iterator;

        struct hashbucket_s *buckets;
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
static int hashfunc (const char *key, unsigned int size)
{
        uint32_t hash;

        if (key == NULL)
                return -EINVAL;
        if (size == 0)
                return -ERANGE;

        for (hash = tolower (*key++); *key != '\0'; key++) {
                uint32_t bit = (hash & 1) ? (1 << (sizeof (uint32_t) - 1)) : 0;

                hash >>= 1;

                hash += tolower (*key) + bit;
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
hashmap_t hashmap_create (unsigned int nbuckets)
{
        struct hashmap_s *ptr;

        if (nbuckets == 0)
                return NULL;

        ptr = (struct hashmap_s *) safecalloc (1, sizeof (struct hashmap_s));
        if (!ptr)
                return NULL;

        ptr->size = nbuckets;
        ptr->buckets = (struct hashbucket_s *) safecalloc (nbuckets,
                                                           sizeof (struct
                                                                   hashbucket_s));
        if (!ptr->buckets) {
                safefree (ptr);
                return NULL;
        }

        /* This points to "one" past the end of the hashmap. */
        ptr->end_iterator = 0;

        return ptr;
}

/*
 * Follow the chain of hashentries and delete them (including the data and
 * the key.)
 *
 * Returns: 0 if the function completed successfully
 *          negative number is returned if "entry" was NULL
 */
static int delete_hashbucket (struct hashbucket_s *bucket)
{
        struct hashentry_s *nextptr;
        struct hashentry_s *ptr;

        if (bucket == NULL || bucket->head == NULL)
                return -EINVAL;

        ptr = bucket->head;
        while (ptr) {
                nextptr = ptr->next;

                safefree (ptr->key);
                safefree (ptr->data);
                safefree (ptr);

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
int hashmap_delete (hashmap_t map)
{
        unsigned int i;

        if (map == NULL)
                return -EINVAL;

        for (i = 0; i != map->size; i++) {
                if (map->buckets[i].head != NULL) {
                        delete_hashbucket (&map->buckets[i]);
                }
        }

        safefree (map->buckets);
        safefree (map);

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
hashmap_insert (hashmap_t map, const char *key, const void *data, size_t len)
{
        struct hashentry_s *ptr;
        int hash;
        char *key_copy;
        void *data_copy;

        assert (map != NULL);
        assert (key != NULL);
        assert (data != NULL);
        assert (len > 0);

        if (map == NULL || key == NULL)
                return -EINVAL;
        if (!data || len < 1)
                return -ERANGE;

        hash = hashfunc (key, map->size);
        if (hash < 0)
                return hash;

        /*
         * First make copies of the key and data in case there is a memory
         * problem later.
         */
        key_copy = safestrdup (key);
        if (!key_copy)
                return -ENOMEM;

        data_copy = safemalloc (len);
        if (!data_copy) {
                safefree (key_copy);
                return -ENOMEM;
        }
        memcpy (data_copy, data, len);

        ptr = (struct hashentry_s *) safemalloc (sizeof (struct hashentry_s));
        if (!ptr) {
                safefree (key_copy);
                safefree (data_copy);
                return -ENOMEM;
        }

        ptr->key = key_copy;
        ptr->data = data_copy;
        ptr->len = len;

        /*
         * Now add the entry to the end of the bucket chain.
         */
        ptr->next = NULL;
        ptr->prev = map->buckets[hash].tail;
        if (map->buckets[hash].tail)
                map->buckets[hash].tail->next = ptr;

        map->buckets[hash].tail = ptr;
        if (!map->buckets[hash].head)
                map->buckets[hash].head = ptr;

        map->end_iterator++;
        return 0;
}

/*
 * Get an iterator to the first entry.
 *
 * Returns: an negative value upon error.
 */
hashmap_iter hashmap_first (hashmap_t map)
{
        assert (map != NULL);

        if (!map)
                return -EINVAL;

        if (map->end_iterator == 0)
                return -1;
        else
                return 0;
}

/*
 * Checks to see if the iterator is pointing at the "end" of the entries.
 *
 * Returns: 1 if it is the end
 *          0 otherwise
 */
int hashmap_is_end (hashmap_t map, hashmap_iter iter)
{
        assert (map != NULL);
        assert (iter >= 0);

        if (!map || iter < 0)
                return -EINVAL;

        if (iter == map->end_iterator)
                return 1;
        else
                return 0;
}

/*
 * Return a "pointer" to the first instance of the particular key.  It can
 * be tested against hashmap_is_end() to see if the key was not found.
 *
 * Returns: negative upon an error
 *          an "iterator" pointing at the first key
 *          an "end-iterator" if the key wasn't found
 */
hashmap_iter hashmap_find (hashmap_t map, const char *key)
{
        unsigned int i;
        hashmap_iter iter = 0;
        struct hashentry_s *ptr;

        assert (map != NULL);
        assert (key != NULL);

        if (!map || !key)
                return -EINVAL;

        /*
         * Loop through all the keys and look for the first occurrence
         * of a particular key.
         */
        for (i = 0; i != map->size; i++) {
                ptr = map->buckets[i].head;

                while (ptr) {
                        if (strcasecmp (ptr->key, key) == 0) {
                                /* Found it, so return the current count */
                                return iter;
                        }

                        iter++;
                        ptr = ptr->next;
                }
        }

        return iter;
}

/*
 * Retrieve the data associated with a particular iterator.
 *
 * Returns: the length of the data block upon success
 *          negative upon error
 */
ssize_t
hashmap_return_entry (hashmap_t map, hashmap_iter iter, char **key, void **data)
{
        unsigned int i;
        struct hashentry_s *ptr;
        hashmap_iter count = 0;

        assert (map != NULL);
        assert (iter >= 0);
        assert (iter != map->end_iterator);
        assert (key != NULL);
        assert (data != NULL);

        if (!map || iter < 0 || !key || !data)
                return -EINVAL;

        for (i = 0; i != map->size; i++) {
                ptr = map->buckets[i].head;
                while (ptr) {
                        if (count == iter) {
                                /* This is the data so return it */
                                *key = ptr->key;
                                *data = ptr->data;
                                return ptr->len;
                        }

                        ptr = ptr->next;
                        count++;
                }
        }

        return -EFAULT;
}

/*
 * Searches for _any_ occurrences of "key" within the hashmap.
 *
 * Returns: negative upon an error
 *          zero if no key is found
 *          count found
 */
ssize_t hashmap_search (hashmap_t map, const char *key)
{
        int hash;
        struct hashentry_s *ptr;
        ssize_t count = 0;

        if (map == NULL || key == NULL)
                return -EINVAL;

        hash = hashfunc (key, map->size);
        if (hash < 0)
                return hash;

        ptr = map->buckets[hash].head;

        /* All right, there is an entry here, now see if it's the one we want */
        while (ptr) {
                if (strcasecmp (ptr->key, key) == 0)
                        ++count;

                /* This entry didn't contain the key; move to the next one */
                ptr = ptr->next;
        }

        return count;
}

/*
 * Get the first entry (assuming there is more than one) for a particular
 * key.  The data MUST be non-NULL.
 *
 * Returns: negative upon error
 *          zero if no entry is found
 *          length of data for the entry
 */
ssize_t hashmap_entry_by_key (hashmap_t map, const char *key, void **data)
{
        int hash;
        struct hashentry_s *ptr;

        if (!map || !key || !data)
                return -EINVAL;

        hash = hashfunc (key, map->size);
        if (hash < 0)
                return hash;

        ptr = map->buckets[hash].head;

        while (ptr) {
                if (strcasecmp (ptr->key, key) == 0) {
                        *data = ptr->data;
                        return ptr->len;
                }

                ptr = ptr->next;
        }

        return 0;
}

/*
 * Go through the hashmap and remove the particular key.
 * NOTE: This will invalidate any iterators which have been created.
 *
 * Remove: negative upon error
 *         0 if the key was not found
 *         positive count of entries deleted
 */
ssize_t hashmap_remove (hashmap_t map, const char *key)
{
        int hash;
        struct hashentry_s *ptr, *next;
        short int deleted = 0;

        if (map == NULL || key == NULL)
                return -EINVAL;

        hash = hashfunc (key, map->size);
        if (hash < 0)
                return hash;

        ptr = map->buckets[hash].head;
        while (ptr) {
                if (strcasecmp (ptr->key, key) == 0) {
                        /*
                         * Found the data, now need to remove everything
                         * and update the hashmap.
                         */
                        next = ptr->next;

                        if (ptr->prev)
                                ptr->prev->next = ptr->next;
                        if (ptr->next)
                                ptr->next->prev = ptr->prev;

                        if (map->buckets[hash].head == ptr)
                                map->buckets[hash].head = ptr->next;
                        if (map->buckets[hash].tail == ptr)
                                map->buckets[hash].tail = ptr->prev;

                        safefree (ptr->key);
                        safefree (ptr->data);
                        safefree (ptr);

                        ++deleted;
                        --map->end_iterator;

                        ptr = next;
                        continue;
                }

                /* This entry didn't contain the key; move to the next one */
                ptr = ptr->next;
        }

        /* The key was not found, so return 0 */
        return deleted;
}
