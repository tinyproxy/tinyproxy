/* $Id: dnscache.c,v 1.2 2000-03-11 20:37:44 rjkaes Exp $
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
#include <defines.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>

#include "utils.h"
#include "dnscache.h"

#define HASH_BOX 25
#define DNSEXPIRE (5 * 60)

struct dnscache_s {
	struct in_addr ipaddr;
	char *domain;
	time_t expire;
	struct dnscache_s *next;
};

struct dnscache_s *cache[HASH_BOX];

static unsigned int hash(unsigned char *keystr, unsigned int box)
{
	unsigned long hashc = 0;
	unsigned char ch;

	assert(keystr);
	assert(box > 0);

	while ((ch = *keystr++))
		hashc += tolower(ch);

	return hashc % box;
}

int lookup(struct in_addr *addr, char *domain)
{
	unsigned int box = hash(domain, HASH_BOX);
	struct dnscache_s **rptr = &cache[box];
	struct dnscache_s *ptr = cache[box];

	assert(domain);

	while (ptr && strcasecmp(ptr->domain, domain)) {
		rptr = &ptr->next;
		ptr = ptr->next;
	}

	if (ptr && !strcasecmp(ptr->domain, domain)) {
		/* Woohoo... found it. Make sure it hasn't expired */
		if (difftime(time(NULL), ptr->expire) > DNSEXPIRE) {
			/* Oops... expired */
			*rptr = ptr->next;
			safefree(ptr->domain);
			safefree(ptr);
			return -1;
		}

		/* chris - added this so that the routine can be used to just
		 * look stuff up.
		*/
		if (addr)
			*addr = ptr->ipaddr;
		return 0;
	}

	return -1;
}

int insert(struct in_addr *addr, char *domain)
{
	unsigned int box = hash(domain, HASH_BOX);
	struct dnscache_s **rptr = &cache[box];
	struct dnscache_s *ptr = cache[box];
	struct dnscache_s *newptr;

	assert(addr);
	assert(domain);

	while (ptr) {
		rptr = &ptr->next;
		ptr = ptr->next;
	}

	if (!(newptr = xmalloc(sizeof(struct dnscache_s)))) {
		return -1;
	}

	if (!(newptr->domain = xstrdup(domain))) {
		safefree(newptr);
		return -1;
	}

	newptr->ipaddr = *addr;

	newptr->expire = time(NULL);

	*rptr = newptr;
	newptr->next = ptr;

	return 0;
}

int dnscache(struct in_addr *addr, char *domain)
{
	struct hostent *resolv;

	assert(addr);
	assert(domain);

	if (inet_aton(domain, (struct in_addr *) addr) != 0)
		return 0;

	/* Well, we're not dotted-decimal so we need to look it up */
	if (lookup(addr, domain) == 0)
		return 0;

	/* Okay, so not in the list... need to actually look it up. */
	if (!(resolv = gethostbyname(domain)))
		return -1;

	memcpy(addr, resolv->h_addr_list[0], resolv->h_length);
	insert(addr, domain);

	return 0;
}

static void dnsdelete(unsigned int c, struct dnscache_s *del)
{
	struct dnscache_s **rptr;
	struct dnscache_s *ptr;

	assert(c > 0);
	assert(del);

	rptr = &cache[c];
	ptr = cache[c];

	while (ptr && (ptr != del)) {
		rptr = &ptr->next;
		ptr = ptr->next;
	}

	if (ptr == del) {
		*rptr = ptr->next;
		safefree(ptr->domain);
		safefree(ptr);
	}
}

void dnsclean(void)
{
	unsigned int c;
	struct dnscache_s *ptr, *tmp;

	for (c = 0; c < HASH_BOX; c++) {
		ptr = cache[c];

		while (ptr) {
			tmp = ptr->next;

			if (difftime(time(NULL), ptr->expire) > DNSEXPIRE)
				dnsdelete(c, ptr);

			ptr = tmp;
		}
	}
}
