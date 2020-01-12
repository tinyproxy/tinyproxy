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
#include "reqs.h"

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

typedef struct upstream_proxy_list {
        struct upstream_proxy_list *next;
        char *host;
        int port;
        time_t last_failed_connect;

} upstream_proxy_list_t;

struct upstream {
        struct upstream *next;
        char *domain;           /* optional */
        struct upstream_proxy_list *plist;
        char *host;
        int port;
        union {
                char *user;
                char *authstr;
        } ua;
        char *pass;
        in_addr_t ip, mask;
        proxy_type type;
#if defined(UPSTREAM_SUPPORT) && defined(UPSTREAM_REGEX)
        char *pat;
        regex_t *cpat;
#endif
};

#ifdef UPSTREAM_SUPPORT
const char *proxy_type_name(proxy_type type);
extern void upstream_add (const struct upstream_proxy_list *phost,
                          const char *domain, const char *user,
                          const char *pass, proxy_type type,
                          struct upstream **upstream_list);
extern struct upstream *upstream_get (struct request_s *request,
                                      struct upstream *up);
extern void free_upstream_list (struct upstream *up);
#endif /* UPSTREAM_SUPPORT */

#endif /* _TINYPROXY_UPSTREAM_H_ */
