/* $Id: reqs.h,v 1.4 2003-05-29 19:43:57 rjkaes Exp $
 *
 * See 'reqs.c' for a detailed description.
 *
 * Copyright (C) 1998  Steven Young
 * Copyright (C) 1999  Robert James Kaes (rjkaes@flarenet.com)
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

#ifndef _TINYPROXY_REQS_H_
#define _TINYPROXY_REQS_H_

extern void handle_connection(int fd);
extern void add_connect_port_allowed(int port);
extern void upstream_add(const char *host, int port, const char *domain);

#endif
