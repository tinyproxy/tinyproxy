/* $Id: dnscache.h,v 1.3 2000-10-23 21:42:31 rjkaes Exp $
 *
 * See 'dnscache.c' for a detailed description.
 *
 * Copyright (C) 1999  Robert James Kaes (rjkaes@flarenet.com
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

#ifndef _TINYPROXY_DNSCACHE_H_
#define _TINYPROXY_DNSCACHE_H_

#include <arpa/inet.h>

extern int new_dnscache(void);
extern int dnscache(struct in_addr *addr, char *domain);

#endif
