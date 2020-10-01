/* tinyproxy - A fast light-weight HTTP proxy
 * Copyright (C) 2000, 2002 Robert James Kaes <rjkaes@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/* This system handles Access Control for use of this daemon. A list of
 * domains, or IP addresses (including IP blocks) are stored in a list
 * which is then used to compare incoming connections.
 */

#include "main.h"

#include "acl.h"
#include "heap.h"
#include "log.h"
#include "network.h"
#include "sock.h"
#include "sblist.h"

#include <limits.h>

/* Define how long an IPv6 address is in bytes (128 bits, 16 bytes) */
#define IPV6_LEN 16

enum acl_type {
        ACL_STRING,
        ACL_NUMERIC
};

/*
 * Hold the information about a particular access control.  We store
 * whether it's an ALLOW or DENY entry, and also whether it's a string
 * entry (like a domain name) or an IP entry.
 */
struct acl_s {
        acl_access_t access;
        enum acl_type type;
        union {
                char *string;
                struct {
                        unsigned char network[IPV6_LEN];
                        unsigned char mask[IPV6_LEN];
                } ip;
        } address;
};

/*
 * Fills in the netmask array given a numeric value.
 *
 * Returns:
 *   0 on success
 *  -1 on failure (invalid mask value)
 *
 */
static int
fill_netmask_array (char *bitmask_string, int v6,
                    unsigned char array[], size_t len)
{
        unsigned int i;
        unsigned long int mask;
        char *endptr;

        errno = 0;              /* to distinguish success/failure after call */
        mask = strtoul (bitmask_string, &endptr, 10);

        /* check for various conversion errors */
        if ((errno == ERANGE && mask == ULONG_MAX)
            || (errno != 0 && mask == 0) || (endptr == bitmask_string))
                return -1;

        if (v6 == 0) {
                /* The mask comparison is done as an IPv6 address, so
                 * convert to a longer mask in the case of IPv4
                 * addresses. */
                mask += 12 * 8;
        }

        /* check valid range for a bit mask */
        if (mask > (8 * len))
                return -1;

        /* we have a valid range to fill in the array */
        for (i = 0; i != len; ++i) {
                if (mask >= 8) {
                        array[i] = 0xff;
                        mask -= 8;
                } else if (mask > 0) {
                        array[i] = (unsigned char) (0xff << (8 - mask));
                        mask = 0;
                } else {
                        array[i] = 0;
                }
        }

        return 0;
}

/**
 * If the access list has not been set up, create it.
 */
static int init_access_list(acl_list_t *access_list)
{
        if (!*access_list) {
                *access_list = sblist_new(sizeof(struct acl_s), 16);
                if (!*access_list) {
                        log_message (LOG_ERR,
                                     "Unable to allocate memory for access list");
                        return -1;
                }
        }

        return 0;
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
insert_acl (char *location, acl_access_t access_type, acl_list_t *access_list)
{
        struct acl_s acl;
        char *mask, ip_dst[IPV6_LEN];

        assert (location != NULL);

        if (init_access_list(access_list) != 0)
                return -1;

        /*
         * Start populating the access control structure.
         */
        memset (&acl, 0, sizeof (struct acl_s));
        acl.access = access_type;

        if ((mask = strrchr(location, '/')))
                *(mask++) = 0;

        /*
         * Check for a valid IP address (the simplest case) first.
         */
        if (full_inet_pton (location, ip_dst) > 0) {
                acl.type = ACL_NUMERIC;
                memcpy (acl.address.ip.network, ip_dst, IPV6_LEN);
                if(!mask) memset (acl.address.ip.mask, 0xff, IPV6_LEN);
                else {
                        char dst[sizeof(struct in6_addr)];
                        int v6, i;
                        /* Check if the IP address before the netmask is
                         * an IPv6 address */
                        if (inet_pton(AF_INET6, location, dst) > 0)
                                v6 = 1;
                        else
                                v6 = 0;

                        if (fill_netmask_array
                            (mask, v6, &(acl.address.ip.mask[0]), IPV6_LEN)
                            < 0)
                                goto err;

                        for (i = 0; i < IPV6_LEN; i++)
                                acl.address.ip.network[i] = ip_dst[i] &
                                        acl.address.ip.mask[i];
                }
        } else {
                /* either bogus IP or hostname */
                            /* bogus ipv6 ? */
                if (mask || strchr (location, ':'))
                        goto err;

                /* In all likelihood a string */
                acl.type = ACL_STRING;
                acl.address.string = safestrdup (location);
                if (!acl.address.string)
                        goto err;
        }

        if(!sblist_add(*access_list, &acl)) return -1;
        return 0;
err:;
	/* restore mask for proper error message */
	if(mask) *(--mask) = '/';
	return -1;
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
static int
acl_string_processing (struct acl_s *acl, const char *ip_address,
                       union sockaddr_union *addr, char *string_addr)
{
        int match;
        struct addrinfo hints, *res, *ressave;
        size_t test_length, match_length;
        char ipbuf[512];

        assert (acl && acl->type == ACL_STRING);
        assert (ip_address && strlen (ip_address) > 0);

        /*
         * If the first character of the ACL string is a period, we need to
         * do a string based test only; otherwise, we can do a reverse
         * lookup test as well.
         */
        if (acl->address.string[0] != '.') {
                memset (&hints, 0, sizeof (struct addrinfo));
                hints.ai_family = AF_UNSPEC;
                hints.ai_socktype = SOCK_STREAM;
                if (getaddrinfo (acl->address.string, NULL, &hints, &res) != 0)
                        goto STRING_TEST;

                ressave = res;

                match = FALSE;
                do {
                        get_ip_string (res->ai_addr, ipbuf, sizeof (ipbuf));
                        if (strcmp (ip_address, ipbuf) == 0) {
                                match = TRUE;
                                break;
                        }
                } while ((res = res->ai_next) != NULL);

                freeaddrinfo (ressave);

                if (match) {
                        if (acl->access == ACL_DENY)
                                return 0;
                        else
                                return 1;
                }
        }

STRING_TEST:
        if(string_addr[0] == 0) {
                /* only do costly hostname resolution when it is absolutely needed,
                   and only once */
                if(getnameinfo ((void *) addr, sizeof (*addr),
                                string_addr, HOSTNAME_LENGTH, NULL, 0, 0) != 0)
                        return -1;
        }

        test_length = strlen (string_addr);
        match_length = strlen (acl->address.string);

        /*
         * If the string length is shorter than AC string, return a -1 so
         * that the "driver" will skip onto the next control in the list.
         */
        if (test_length < match_length)
                return -1;

        if (strcasecmp
            (string_addr + (test_length - match_length),
             acl->address.string) == 0) {
                if (acl->access == ACL_DENY)
                        return 0;
                else
                        return 1;
        }

        /* Indicate that no tests succeeded, so skip to next control. */
        return -1;
}

/*
 * Compare the supplied numeric IP address with the supplied ACL structure.
 *
 * Return:
 *   1  IP address is allowed
 *   0  IP address is denied
 *  -1  neither allowed nor denied.
 */
static int check_numeric_acl (const struct acl_s *acl, uint8_t addr[IPV6_LEN])
{
        uint8_t x, y;
        int i;

        assert (acl && acl->type == ACL_NUMERIC);

        for (i = 0; i != IPV6_LEN; ++i) {
                x = addr[i] & acl->address.ip.mask[i];
                y = acl->address.ip.network[i];

                /* If x and y don't match, the IP addresses don't match */
                if (x != y)
                        return -1;
        }

        /* The addresses match, return the permission */
        return (acl->access == ACL_ALLOW);
}

/*
 * Checks whether a connection is allowed.
 *
 * Returns:
 *     1 if allowed
 *     0 if denied
 */
int check_acl (const char *ip, union sockaddr_union *addr, acl_list_t access_list)
{
        struct acl_s *acl;
        int perm = 0, is_numeric_addr;
        size_t i;
        char string_addr[HOSTNAME_LENGTH];
        uint8_t numeric_addr[IPV6_LEN];

        assert (ip != NULL);
        assert (addr != NULL);

        string_addr[0] = 0;

        /*
         * If there is no access list allow everything.
         */
        if (!access_list)
                return 1;

        is_numeric_addr = (full_inet_pton (ip, &numeric_addr) > 0);

        for (i = 0; i < sblist_getsize (access_list); ++i) {
                acl = sblist_get (access_list, i);
                switch (acl->type) {
                case ACL_STRING:
                        perm = acl_string_processing (acl, ip, addr, string_addr);
                        break;

                case ACL_NUMERIC:
                        if (ip[0] == '\0')
                                continue;

                        perm = is_numeric_addr
                               ? check_numeric_acl (acl, numeric_addr)
                               : -1;
                        break;
                }

                /*
                 * Check the return value too see if the IP address is
                 * allowed or denied.
                 */
                if (perm == 0)
                        break;
                else if (perm == 1)
                        return perm;
        }

        /*
         * Deny all connections by default.
         */
        log_message (LOG_NOTICE, "Unauthorized connection from \"%s\".",
                     ip);
        return 0;
}

void flush_access_list (acl_list_t access_list)
{
        struct acl_s *acl;
        size_t i;

        if (!access_list) {
                return;
        }

        /*
         * We need to free allocated data hanging off the acl entries
         * before we can free the acl entries themselves.
         * A hierarchical memory system would be great...
         */
        for (i = 0; i < sblist_getsize (access_list); ++i) {
                acl = sblist_get (access_list, i);
                if (acl->type == ACL_STRING) {
                        safefree (acl->address.string);
                }
        }

        sblist_free (access_list);
}
