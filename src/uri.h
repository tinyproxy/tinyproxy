/* $Id: uri.h,v 1.1.1.1 2000-02-16 17:32:24 sdyoung Exp $
 *
 * See 'uri.c' for a detailed description.
 *
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

#ifndef __URI_H_
#define __URI_H_

typedef struct {
	char *scheme;
	char *authority;
	char *path;
	char *query;
	char *fragment;
} URI;

extern URI *explode_uri(const char *string);
extern void free_uri(URI * uri);

#endif
