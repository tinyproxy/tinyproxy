/* $Id: dnscache.h,v 1.1.1.1 2000-02-16 17:32:22 sdyoung Exp $
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

#ifndef _DNSCACHE_H_
#define _DNSCACHE_H_	1

#ifdef HAVE_CONFIG_H
#include <defines.h>
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DNS_GARBAGE_COL	100

extern int dnscache(struct in_addr *addr, char *domain);
extern void dnsclean(void);

/* chris - Access these from reqs.c because of ADNS. Ugly. */
extern int lookup(struct in_addr *addr, char *domain);
extern int insert(struct in_addr *addr, char *domain);

#endif
