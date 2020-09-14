/* tinyproxy - A fast light-weight HTTP proxy
 * Copyright (C) 1998 Steven Young <sdyoung@miranda.org>
 * Copyright (C) 1999 Robert James Kaes <rjkaes@users.sourceforge.net>
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

/* See 'reqs.c' for detailed information. */

#ifndef _TINYPROXY_REQS_H_
#define _TINYPROXY_REQS_H_

#include "common.h"
#include "sock.h"
#include "conns.h"

/*
 * Port constants for HTTP (80) and SSL (443)
 */
#define HTTP_PORT 80
#define HTTP_PORT_SSL 443

/*
 * This structure holds the information pulled from a URL request.
 */
struct request_s {
        char *method;
        char *protocol;

        char *host;
        uint16_t port;

        char *path;
};

extern void handle_connection (struct conn_s *, union sockaddr_union* addr);

#endif
