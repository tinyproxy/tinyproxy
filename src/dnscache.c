/* $Id: dnscache.c,v 1.7 2000-10-23 21:42:31 rjkaes Exp $
 *
 * This is a caching DNS system. When a host name is needed we look it up here
 * and see if there is already an answer for it. The domains are placed in a
 * hashed linked list. If the name is not here, then we need to look it up and
 * add it to the system. This really speeds up the connection to servers since
 * the DNS name does not need to be looked up each time. It's kind of cool. :)
 *
 * Copyright (C) 1999  Robert James Kaes (rjkaes@flarenet.com)
 * Copyright (C) 2000  Chris Lightfoot (chris@ex-parrot.com)
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <ctype.h>
#include <unistd.h>

#include "dnscache.h"
#include "log.h"
#include "ternary.h"
#include "utils.h"

#define DNSEXPIRE (5 * 60)

struct dnscache_s {
	struct in_addr ipaddr;
	time_t expire;
};

static TERNARY dns_tree;

int new_dnscache(void)
{
	dns_tree = ternary_new();

	return dns_tree;
}

static int dns_lookup(struct in_addr *addr, char *domain)
{
	struct dnscache_s *ptr;

	if (TE_ISERROR(ternary_search(dns_tree, domain, (void *)&ptr))) 
		return -1;

	if (difftime(time(NULL), ptr->expire) > DNSEXPIRE) {
		return -1;
	}

	*addr = ptr->ipaddr;
	return 0;
}

static int dns_insert(struct in_addr *addr, char *domain)
{
	struct dnscache_s *newptr;

	if (!(newptr = malloc(sizeof(struct dnscache_s)))) {
		return -1;
	}

	newptr->ipaddr = *addr;
	newptr->expire = time(NULL);

	if (TE_ISERROR(ternary_insert(dns_tree, domain, newptr)))
		safefree(newptr);

	return 0;
}

int dnscache(struct in_addr *addr, char *domain)
{
	struct hostent *resolv;

	if (inet_aton(domain, (struct in_addr *) addr) != 0)
		return 0;

	/* Well, we're not dotted-decimal so we need to look it up */
	if (dns_lookup(addr, domain) == 0)
		return 0;

	/* Okay, so not in the list... need to actually look it up. */
	if (!(resolv = gethostbyname(domain)))
		return -1;

	memcpy(addr, resolv->h_addr_list[0], resolv->h_length);
	dns_insert(addr, domain);

	return 0;
}
