/* $Id: filter.h,v 1.5 2002-06-07 18:36:21 rjkaes Exp $
 *
 * See 'filter.c' for a detailed description.
 *
 * Copyright (c) 1999  George Talusan (gstalusan@uwaterloo.ca)
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

#ifndef _TINYPROXY_FILTER_H_
#define _TINYPROXY_FILTER_H_

typedef enum {
	FILTER_DEFAULT_ALLOW,
	FILTER_DEFAULT_DENY,
} filter_policy_t;

extern void filter_init(void);
extern void filter_destroy(void);
extern int filter_domain(const char *host);
extern int filter_url(const char *url);

extern void filter_set_default_policy(filter_policy_t policy);

#endif
