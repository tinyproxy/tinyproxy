/* $Id: acl.c,v 1.10 2001-11-03 06:08:37 rjkaes Exp $
 *
 * This system handles Access Control for use of this daemon. A list of
 * domains, or IP addresses (including IP blocks) are stored in a list
 * which is then used to compare incoming connections.
 *
 * Copyright (C) 2000  Robert James Kaes (rjkaes@flarenet.com)
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
#include "log.h"
#include "sock.h"
#include "utils.h"

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
 * value for comparison. Somebody please clean this up. :)
 */
static in_addr_t make_netmask(int netmask_num)
{
	static in_addr_t netmasks[] = {
		0x00000000, 0x80000000, 0xc0000000, 0xe0000000,
		0xf8000000, 0xfc000000, 0xfe000000, 0xff000000,
		0xff800000, 0xffc00000, 0xffe00000, 0xfff00000,
		0xfff00000, 0xfff80000, 0xfffc0000, 0xfffe0000,
		0xffff0000, 0xffff8000, 0xffffc000, 0xffffe000,
		0xfffff000, 0xfffff800, 0xfffffc00, 0xfffffe00,
		0xffffff00, 0xffffff80, 0xffffffc0, 0xffffffe0,
		0xfffffff0, 0xfffffff8, 0xfffffffc, 0xfffffffe,
		0xffffffff
	};

	assert(netmask_num >= 0 && netmask_num <= 32);

	return htonl(netmasks[netmask_num]);
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
int insert_acl(char *location, acl_access_t access_type)
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
		if (isalpha((unsigned char)location[i])) {
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
			if (new_acl_ptr->netmask < 0 || new_acl_ptr->netmask > 32) {
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

	new_acl_ptr->location = strdup(location);
	if (!new_acl_ptr->location) {
		safefree(new_acl_ptr);
		return -1;
	}

	*rev_acl_ptr = new_acl_ptr;
	new_acl_ptr->next = acl_ptr;
	
	return 0;
}

/*
 * Checks where file descriptor is allowed.
 *
 * Returns:
 *     1 if allowed
 *     0 if denied
 *    -1 if error
 */
int check_acl(int fd)
{
	struct acl_s *aclptr;
	char ip_address[PEER_IP_LENGTH];
	char string_address[PEER_STRING_LENGTH];

	assert(fd >= 0);

	/*
	 * If there is no access list allow everything.
	 */
	aclptr = access_list;
	if (!aclptr)
		return 1;

	/*
	 * Get the IP address and the string domain.
	 */
	getpeer_ip(fd, ip_address);
	getpeer_string(fd, string_address);

	while (aclptr) {
		if (aclptr->type == ACL_STRING) {
			size_t test_length = strlen(string_address);
			size_t match_length = strlen(aclptr->location);

			if (test_length < match_length) {
				aclptr = aclptr->next;
				continue;
			}

			if (strcasecmp(string_address + (test_length - match_length), aclptr->location) == 0) {
				if (aclptr->acl_access == ACL_DENY) {
					log_message(LOG_NOTICE, "Unauthorized access from \"%s\"", string_address);
					return 0;
				} else {
					return 1;
				}
			}
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

			if ((test_addr.s_addr & netmask_addr) == (match_addr.s_addr & netmask_addr)) {
				if (aclptr->acl_access == ACL_DENY) {
					log_message(LOG_NOTICE, "Unauthorized access from [%s].", ip_address);
					return 0;
				} else {
					return 1;
				}
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
	log_message(LOG_NOTICE, "Unauthorized connection from \"%s\" [%s].", string_address, ip_address);
	return 0;
}
