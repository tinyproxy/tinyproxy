/* $Id: dnscache.h,v 1.7 2001-10-25 17:02:50 rjkaes Exp $
 *
 * See 'dnscache.c' for a detailed description.
 *
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

#ifndef _TINYPROXY_DNSCACHE_H_
#define _TINYPROXY_DNSCACHE_H_

#include "tinyproxy.h"

extern int dnscache(struct in_addr *addr, char *domain);

#endif
