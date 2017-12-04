/* tinyproxy - A fast light-weight HTTP proxy
 * Copyright (C) 2002, 2004 Robert James Kaes <rjkaes@users.sourceforge.net>
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

/* See 'network.c' for detailed information. */

#ifndef TINYPROXY_NETWORK_H
#define TINYPROXY_NETWORK_H

extern ssize_t safe_write (int fd, const void *buf, size_t count);
extern ssize_t safe_read (int fd, void *buf, size_t count);

extern int write_message (int fd, const char *fmt, ...);
extern ssize_t readline (int fd, char **whole_buffer);

extern const char *get_ip_string (struct sockaddr *sa, char *buf, size_t len);
extern int full_inet_pton (const char *ip, void *dst);

#endif
