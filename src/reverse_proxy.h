/* $Id: reverse_proxy.h,v 1.1 2005-08-16 04:03:19 rjkaes Exp $
 *
 * See 'reverse_proxy.c' for a detailed description.
 *
 * Copyright (C) 2005  Robert James Kaes (rjkaes@users.sourceforge.net)
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

#ifndef TINYPROXY_REVERSE_PROXY_H
#define TINYPROXY_REVERSE_PROXY_H

#include "conns.h"

struct reversepath {
        struct reversepath *next;
        char *path;
        char *url;
};

#define REVERSE_COOKIE "yummy_magical_cookie"

extern void reversepath_add(const char *path, const char *url);
extern struct reversepath *reversepath_get(char *url);
extern char *reverse_rewrite_url(struct conn_s *connptr,
                                 hashmap_t hashofheaders, char *url);

#endif
