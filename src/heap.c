/* $Id: heap.c,v 1.1 2002-05-23 04:41:10 rjkaes Exp $
 *
 * Debugging versions of various heap related functions are combined
 * here.  The debugging versions include assertions and also print
 * (to standard error) the function called along with the amount
 * of memory allocated, and where the memory is pointing.  The
 * format of the log message is standardized.
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

#include "tinyproxy.h"
#include "heap.h"

void *
debugging_calloc(size_t nmemb, size_t size, const char *file,
		 unsigned long line)
{
	void *ptr;

	assert(nmemb > 0);
	assert(size > 0);

	ptr = calloc(nmemb, size);
	fprintf(stderr, "{calloc: %p:%u x %u} %s:%lu\n", ptr, nmemb, size, file,
		line);
	return ptr;
}

void *
debugging_malloc(size_t size, const char *file, unsigned long line)
{
	void *ptr;

	assert(size > 0);

	ptr = malloc(size);
	fprintf(stderr, "{malloc: %p:%u} %s:%lu\n", ptr, size, file, line);
	return ptr;
}

void *
debugging_realloc(void *ptr, size_t size, const char *file, unsigned long line)
{
	void *newptr;
	
	assert(ptr != NULL);
	assert(size > 0);
	
	newptr = realloc(ptr, size);
	fprintf(stderr, "{realloc: %p -> %p:%u} %s:%lu\n", ptr, newptr, size,
		file, line);
	return newptr;
}

void
debugging_free(void *ptr, const char *file, unsigned long line)
{
	assert(ptr != NULL);

	fprintf(stderr, "{free: %p} %s:%lu\n", ptr, file, line);
	free(ptr);
	return;
}

char*
debugging_strdup(const char* s, const char* file, unsigned long line)
{
	char* ptr;
	size_t len;

	assert(s != NULL);

	len = strlen(s) + 1;
	ptr = malloc(len);
	if (!ptr)
		return NULL;
	memcpy(ptr, s, len);

	fprintf(stderr, "{strdup: %p:%u} %s:%lu\n", ptr, len, file, line);
	return ptr;
}
