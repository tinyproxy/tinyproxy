/* $Id: acl.c,v 1.16 2002-06-05 16:59:21 rjkaes Exp $
 *
 * This system handles Access Control for use of this daemon. A list of
 * domains, or IP addresses (including IP blocks) are stored in a list
 * which is then used to compare incoming connections.
 *
 * Copyright (C) 2000,2002  Robert James Kaes (rjkaes@flarenet.com)
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

#include "tinyproxy.h"

#include "acl.h"
#include "heap.h"
#include "log.h"
#include "sock.h"

struct acl_s {
	acl_access_t acl_access;
	enum { ACL_STRING, ACL_NUMERIC } type;
	char *location;
	int netmask;
	struct acl_s *next;
};

static struct acl_s *access_list = NULL;

/*
 * Take a netmask number (between 0 and 32) and returns a network ordered
 * value for comparison.
 */
static in_addr_t
make_netmask(int netmask_num)
{
	assert(netmask_num >= 0 && netmask_num <= 32);

	return htonl(~((1 << (32 - netmask_num)) - 1));
}

/*
 * Inserts a new access control into the list. The function will figure out
 * whether the location is an IP address (with optional netmask) or a
 * domain name.
 *
 * Returns:
 *    -1 on failure
 *     0 otherwise.
 */
int
insert_acl(char *location, acl_access_t access_type)
{
	size_t i;
	struct acl_s **rev_acl_ptr, *acl_ptr, *new_acl_ptr;
	char *nptr;

	assert(location != NULL);

	/*
	 * First check to see if the location is a string or numeric.
	 */
	for (i = 0; location[i] != '\0'; i++) {
		/*
		 * Numeric strings can not contain letters, so test on it.
		 */
		if (isalpha((unsigned char) location[i])) {
			break;
		}
	}

	/*
	 * Add a new ACL to the list.
	 */
	rev_acl_ptr = &access_list;
	acl_ptr = access_list;
	while (acl_ptr) {
		rev_acl_ptr = &acl_ptr->next;
		acl_ptr = acl_ptr->next;
	}
	new_acl_ptr = safemalloc(sizeof(struct acl_s));
	if (!new_acl_ptr) {
		return -1;
	}

	new_acl_ptr->acl_access = access_type;

	if (location[i] == '\0') {
		DEBUG2("ACL \"%s\" is a number.", location);

		/*
		 * We did not break early, so this a numeric location.
		 * Check for a netmask.
		 */
		new_acl_ptr->type = ACL_NUMERIC;
		nptr = strchr(location, '/');
		if (nptr) {
			*nptr++ = '\0';

			new_acl_ptr->netmask = strtol(nptr, NULL, 10);
			if (new_acl_ptr->netmask < 0
			    || new_acl_ptr->netmask > 32) {
				safefree(new_acl_ptr);
				return -1;
			}
		} else {
			new_acl_ptr->netmask = 32;
		}
	} else {
		DEBUG2("ACL \"%s\" is a string.", location);

		new_acl_ptr->type = ACL_STRING;
		new_acl_ptr->netmask = 32;
	}

	new_acl_ptr->location = safestrdup(location);
	if (!new_acl_ptr->location) {
		safefree(new_acl_ptr);
		return -1;
	}

	*rev_acl_ptr = new_acl_ptr;
	new_acl_ptr->next = acl_ptr;

	return 0;
}

/*
 * This function is called whenever a "string" access control is found in
 * the ACL.  From here we do both a text based string comparison, along with
 * a reverse name lookup comparison of the IP addresses.
 *
 * Return: 0 if host is denied
 *         1 if host is allowed
 *        -1 if no tests match, so skip
 */
static inline int
acl_string_processing(struct acl_s* aclptr,
		      const char* ip_address,
		      const char* string_address)
{
	int i;
	struct hostent* result;
	size_t test_length, match_length;

	/*
	 * If the first character of the ACL string is a period, we need to
	 * do a string based test only; otherwise, we can do a reverse
	 * lookup test as well.
	 */
	if (aclptr->location[0] != '.') {
		/* It is not a partial domain, so do a reverse lookup. */
		result = gethostbyname(aclptr->location);
		if (!result)
			goto STRING_TEST;
				
		for (i = 0; result->h_addr_list[i]; ++i) {
			if (strcmp(ip_address,
				   inet_ntoa(*((struct in_addr*)result->h_addr_list[i]))) == 0) {
				/* We have a match */
				if (aclptr->acl_access == ACL_DENY) {
					return 0;
				} else {
					DEBUG2("Matched using reverse domain lookup: %s", ip_address);
					return 1;
				}
			}
		}

		/*
		 * If we got this far, the reverse didn't match, so drop down
		 * to a standard string test.
		 */
	}

STRING_TEST:
	test_length = strlen(string_address);
	match_length = strlen(aclptr->location);

	/*
	 * If the string length is shorter than AC string, return a -1 so
	 * that the "driver" will skip onto the next control in the list.
	 */
	if (test_length < match_length)
		return -1;

	if (strcasecmp(string_address + (test_length - match_length), aclptr->location) == 0) {
		if (aclptr->acl_access == ACL_DENY)
			return 0;
		else
			return 1;
	}

	/* Indicate that no tests succeeded, so skip to next control. */
	return -1;
}

/*
 * Checks whether file descriptor is allowed.
 *
 * Returns:
 *     1 if allowed
 *     0 if denied
 */
int
check_acl(int fd, const char* ip_address, const char* string_address)
{
	struct acl_s* aclptr;
	int ret;

	assert(fd >= 0);
	assert(ip_address != NULL);
	assert(string_address != NULL);

	/*
	 * If there is no access list allow everything.
	 */
	aclptr = access_list;
	if (!aclptr)
		return 1;

	while (aclptr) {
		if (aclptr->type == ACL_STRING) {
			ret = acl_string_processing(aclptr,
						    ip_address,
						    string_address);
			if (ret == 0)
				goto UNAUTHORIZED;
			else if (ret == 1)
				return 1;

			aclptr = aclptr->next;
			continue;
		} else {
			struct in_addr test_addr, match_addr;
			in_addr_t netmask_addr;

			if (ip_address[0] == 0) {
				aclptr = aclptr->next;
				continue;
			}

			inet_aton(ip_address, &test_addr);
			inet_aton(aclptr->location, &match_addr);

			netmask_addr = make_netmask(aclptr->netmask);

			if ((test_addr.s_addr & netmask_addr) ==
			    (match_addr.s_addr & netmask_addr)) {
				if (aclptr->acl_access == ACL_DENY)
					goto UNAUTHORIZED;
				else
					return 1;
			}
		}

		/*
		 * Dropped through... go on to the next one.
		 */
		aclptr = aclptr->next;
	}

	/*
	 * Deny all connections by default.
	 */
UNAUTHORIZED:
	log_message(LOG_NOTICE, "Unauthorized connection from \"%s\" [%s].",
		    string_address, ip_address);
	return 0;
}
