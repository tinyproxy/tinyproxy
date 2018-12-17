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

/* Debugging versions of various heap related functions are combined
 * here.  The debugging versions include assertions and also print
 * (to standard error) the function called along with the amount
 * of memory allocated, and where the memory is pointing.  The
 * format of the log message is standardized.
 */

#include "main.h"
#include "heap.h"
#include "text.h"

#ifndef NDEBUG

void *debugging_calloc (size_t nmemb, size_t size, const char *file,
                        unsigned long line)
{
        void *ptr;

        assert (nmemb > 0);
        assert (size > 0);

        ptr = calloc (nmemb, size);
        fprintf (stderr, "{calloc: %p:%lu x %lu} %s:%lu\n", ptr,
                 (unsigned long) nmemb, (unsigned long) size, file, line);
        return ptr;
}

void *debugging_malloc (size_t size, const char *file, unsigned long line)
{
        void *ptr;

        assert (size > 0);

        ptr = malloc (size);
        fprintf (stderr, "{malloc: %p:%lu} %s:%lu\n", ptr,
                 (unsigned long) size, file, line);
        return ptr;
}

void *debugging_realloc (void *ptr, size_t size, const char *file,
                         unsigned long line)
{
        void *newptr;

        assert (size > 0);

        newptr = realloc (ptr, size);
        fprintf (stderr, "{realloc: %p -> %p:%lu} %s:%lu\n", ptr, newptr,
                 (unsigned long) size, file, line);
        return newptr;
}

void debugging_free (void *ptr, const char *file, unsigned long line)
{
        fprintf (stderr, "{free: %p} %s:%lu\n", ptr, file, line);

        if (ptr != NULL)
                free (ptr);
        return;
}

char *debugging_strdup (const char *s, const char *file, unsigned long line)
{
        char *ptr;
        size_t len;

        assert (s != NULL);

        len = strlen (s) + 1;
        ptr = (char *) malloc (len);
        if (!ptr)
                return NULL;
        memcpy (ptr, s, len);

        fprintf (stderr, "{strdup: %p:%lu} %s:%lu\n", ptr,
                 (unsigned long) len, file, line);
        return ptr;
}

#endif /* !NDEBUG */

