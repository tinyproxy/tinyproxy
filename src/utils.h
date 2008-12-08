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

/* See 'utils.c' for detailed information. */

#ifndef TINYPROXY_UTILS_H
#define TINYPROXY_UTILS_H

/*
 * Forward declaration.
 */
struct conn_s;

extern int send_http_message (struct conn_s *connptr, int http_code,
                              const char *error_title, const char *message);

extern int pidfile_create (const char *path);
extern int create_file_safely (const char *filename,
                               unsigned int truncate_file);

#endif
