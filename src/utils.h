/* $Id: utils.h,v 1.10 2001-10-25 16:58:50 rjkaes Exp $
 *
 * See 'utils.h' for a detailed description.
 *
 * Copyright (C) 1998  Steven Young
 * Copyright (C) 1999  Robert James Kaes (rjkaes@flarenet.com)
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

#ifndef _TINYPROXY_UTILS_H_
#define _TINYPROXY_UTILS_H_

#include "tinyproxy.h"

#include "conns.h"

extern int send_http_message(struct conn_s* connptr, int http_code,
			     const char *error_title, const char *message);
extern int httperr(struct conn_s *connptr, int err, const char *msg);

extern void makedaemon(void);
extern void pidfile_create(const char *path);

extern int create_file_safely(const char *filename);

#ifndef HAVE_STRLCAT
extern size_t strlcat(char *dst, const char *src, size_t size);
#endif /* HAVE_STRLCAT */

#ifndef HAVE_STRLCPY
extern size_t strlcpy(char *dst, const char *src, size_t size);
#endif /* HAVE_STRLCPY */

/*
 * The following is to allow for better memory checking.
 */
#ifndef NDEBUG

extern void *debugging_calloc(size_t nmemb, size_t size, const char *file, unsigned long line);
extern void *debugging_malloc(size_t size, const char *file, unsigned long line);
extern void debugging_free(void *ptr, const char *file, unsigned long line);
extern void *debugging_realloc(void *ptr, size_t size, const char *file, unsigned long line);

#  define safecalloc(x, y) debugging_calloc(x, y, __FILE__, __LINE__)
#  define safemalloc(x) debugging_malloc(x, __FILE__, __LINE__)
#  define saferealloc(x, y) debugging_realloc(x, y, __FILE__, __LINE__)
#  define safefree(x) debugging_free(x, __FILE__, __LINE__)
#else
#  define safecalloc(x, y) calloc(x, y)
#  define safemalloc(x) malloc(x)
#  define saferealloc(x, y) realloc(x, y)
#  define safefree(x) free(x)
#endif

#endif
