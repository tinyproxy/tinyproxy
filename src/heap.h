/* $Id: heap.h,v 1.3 2003-05-31 23:04:15 rjkaes Exp $
 *
 * See 'heap.c' for a detailed description.
 *
 * Copyright (C) 2002  Robert James Kaes (rjkaes@flarenet.com)
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

#ifndef TINYPROXY_HEAP_H
#define TINYPROXY_HEAP_H

/*
 * The following is to allow for better memory checking.
 */
#ifndef NDEBUG

extern void *debugging_calloc(size_t nmemb, size_t size, const char *file,
			      unsigned long line);
extern void *debugging_malloc(size_t size, const char *file,
			      unsigned long line);
extern void debugging_free(void *ptr, const char *file, unsigned long line);
extern void *debugging_realloc(void *ptr, size_t size, const char *file,
			       unsigned long line);
extern char *debugging_strdup(const char* s, const char* file,
			      unsigned long line);

#  define safecalloc(x, y) debugging_calloc(x, y, __FILE__, __LINE__)
#  define safemalloc(x) debugging_malloc(x, __FILE__, __LINE__)
#  define saferealloc(x, y) debugging_realloc(x, y, __FILE__, __LINE__)
#  define safestrdup(x) debugging_strdup(x, __FILE__, __LINE__)
#  define safefree(x) do { \
void **__safefree_tmp = (void *)&(x); \
debugging_free(*__safefree_tmp, __FILE__, __LINE__); \
*__safefree_tmp = NULL; \
} while (0)
#else
#  define safecalloc(x, y) calloc(x, y)
#  define safemalloc(x) malloc(x)
#  define saferealloc(x, y) realloc(x, y)
#  define safefree(x) do { \
void **__safefree_tmp = (void *)&(x); \
free(*__safefree_tmp); \
*__safefree_tmp = NULL; \
} while (0)
#  define safestrdup(x) strdup(x)
#endif

/*
 * Allocate memory from the "shared" region of memory.
 */
extern void* malloc_shared_memory(size_t size);
extern void* calloc_shared_memory(size_t nmemb, size_t size);

#endif
