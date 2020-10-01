/* tinyproxy - A fast light-weight HTTP proxy
 * Copyright (C) 1998 Steven Young <sdyoung@miranda.org>
 * Copyright (C) 1999 Robert James Kaes <rjkaes@users.sourceforge.net>
 * Copyright (C) 2009 Michael Adam <obnox@samba.org>
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

/*
 * Routines for handling the list of upstream proxies.
 */

#ifndef _TINYPROXY_UPSTREAM_H_
#define _TINYPROXY_UPSTREAM_H_

#include "common.h"

enum upstream_build_error {
	UBE_SUCCESS = 0,
	UBE_OOM,
	UBE_USERLEN,
	UBE_EDOMAIN,
	UBE_INVHOST,
	UBE_INVPARAMS,
	UBE_NETMASK,
};

/*
 * Even if upstream support is not compiled into tinyproxy, this
 * structure still needs to be defined.
 */
typedef enum proxy_type {
	PT_NONE = 0,
	PT_HTTP,
	PT_SOCKS4,
	PT_SOCKS5
} proxy_type;

struct upstream {
        struct upstream *next;
        char *domain;           /* optional */
        char *host;
        union {
                char *user;
                char *authstr;
        } ua;
        char *pass;
        int port;
        in_addr_t ip, mask;
        proxy_type type;
};

#ifdef UPSTREAM_SUPPORT
const char *proxy_type_name(proxy_type type);
extern enum upstream_build_error upstream_add (
                          const char *host, int port, const char *domain,
                          const char *user, const char *pass,
                          proxy_type type, struct upstream **upstream_list);
extern struct upstream *upstream_get (char *host, struct upstream *up);
extern void free_upstream_list (struct upstream *up);
extern const char* upstream_build_error_string(enum upstream_build_error);
#endif /* UPSTREAM_SUPPORT */

#endif /* _TINYPROXY_UPSTREAM_H_ */
