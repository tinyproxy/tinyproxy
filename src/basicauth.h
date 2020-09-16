/* tinyproxy - A fast light-weight HTTP proxy
 * Copyright (C) 2005 Robert James Kaes <rjkaes@users.sourceforge.net>
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

/* See 'basicauth.c' for detailed information. */

#ifndef TINYPROXY_BASICAUTH_H
#define TINYPROXY_BASICAUTH_H

#include <stddef.h>
#include "sblist.h"

extern ssize_t basicauth_string(const char *user, const char *pass,
	char *buf, size_t bufsize);

extern void basicauth_add (sblist *authlist,
	const char *user, const char *pass);

extern int basicauth_check (sblist *authlist, const char *authstring);

#endif
