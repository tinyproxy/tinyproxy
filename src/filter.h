/* $Id: filter.h,v 1.2 2000-09-11 23:43:59 rjkaes Exp $
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

extern void filter_init(void);
extern void filter_destroy(void);
extern int filter_host(char *host);

#endif
