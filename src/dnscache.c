/* $Id: dnscache.c,v 1.11 2001-08-30 16:50:42 rjkaes Exp $
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
#include <assert.h>
#include <ctype.h>
#include <unistd.h>

#include "dnscache.h"
#include "log.h"
#include "ternary.h"
#include "utils.h"

/*
 * The mutex is used for locking around accesses to the ternary tree.
 */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

#define LOCK()   pthread_mutex_lock(&mutex);
#define UNLOCK() pthread_mutex_unlock(&mutex);

#define DNSEXPIRE (5 * 60)

struct dnscache_s {
	struct in_addr ipaddr;
	time_t expire;
};

static TERNARY dns_tree = -1;

static int dns_lookup(struct in_addr *addr, char *domain)
{
	int ret;
	struct dnscache_s *ptr;

	assert(addr != NULL);
	assert(domain != NULL);

	ret = ternary_search(dns_tree, domain, (void *)&ptr);

	if (TE_ISERROR(ret)
	    || difftime(time(NULL), ptr->expire) > DNSEXPIRE) {
		return -1;
	}

	memcpy(addr, &ptr->ipaddr, sizeof(struct in_addr));

	return 0;
}

static int dns_insert(struct in_addr *addr, char *domain)
{
	struct dnscache_s *newptr;

	assert(addr != NULL);
	assert(domain != NULL);

	DEBUG2("Inserting [%s] into DNS cache", domain);

	if (!(newptr = malloc(sizeof(struct dnscache_s)))) {
		return -1;
	}

	memcpy(&newptr->ipaddr, addr, sizeof(struct in_addr));
	newptr->expire = time(NULL);

	if (TE_ISERROR(ternary_replace(dns_tree, domain, newptr))) {
		safefree(newptr);
		return -1;
	}

	return 0;
}

int dnscache(struct in_addr *addr, char *domain)
{
	struct hostent *resolv;

	assert(addr != NULL);
	assert(domain != NULL);

	LOCK();
	
	/* If the DNS tree doesn't exist, build a new one */
	if (dns_tree < 0)
		dns_tree = ternary_new();

	if (inet_aton(domain, (struct in_addr *)addr) != 0) {
		UNLOCK();
		return 0;
	}

	/* Well, we're not dotted-decimal so we need to look it up */
	if (dns_lookup(addr, domain) == 0) {
		UNLOCK();
		return 0;
	}

	/* Okay, so not in the list... need to actually look it up. */
	if (!(resolv = gethostbyname(domain))) {
		UNLOCK();
		return -1;
	}

	memcpy(addr, resolv->h_addr_list[0], (size_t)resolv->h_length);

	dns_insert(addr, domain);

	UNLOCK();

	return 0;
}
