/* $Id: filter.h,v 1.1.1.1 2000-02-16 17:32:24 sdyoung Exp $
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

#ifndef __FILTER_H_
#define __FILTER_H_	1

extern void filter_init(void);
extern void filter_destroy(void);
extern int filter_host(char *host);

#endif
