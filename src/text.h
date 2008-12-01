/* tinyproxy - A fast light-weight HTTP proxy
 * Copyright (C) 2002 Robert James Kaes <rjkaes@users.sourceforge.net>
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

/* See 'text.c' for detailed information. */

#ifndef TINYPROXY_TEXT_H
#define TINYPROXY_TEXT_H

#ifndef HAVE_STRLCAT
extern size_t strlcat (char *dst, const char *src, size_t size);
#endif /* HAVE_STRLCAT */

#ifndef HAVE_STRLCPY
extern size_t strlcpy (char *dst, const char *src, size_t size);
#endif /* HAVE_STRLCPY */

extern ssize_t chomp (char *buffer, size_t length);

#endif
