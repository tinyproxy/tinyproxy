/* $Id: utils.h,v 1.1.1.1 2000-02-16 17:32:24 sdyoung Exp $
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

#ifndef __UTILS_H_
#define __UTILS_H_	1

#include "conns.h"

#define safefree(x) free(x); x = NULL

extern char *xstrdup(char *st);
extern void *xmalloc(unsigned long int sz);
extern char *xstrstr(char *haystack, char *needle, unsigned int length,
		     int case_sensitive);

extern int showstats(struct conn_s *connptr);
extern int httperr(struct conn_s *connptr, int err, char *msg);

extern int calcload(void);

extern void makedaemon(void);

#endif
