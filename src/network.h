/* $Id: network.h,v 1.1 2002-05-23 04:41:48 rjkaes Exp $
 *
 * See 'network.c' for a detailed description.
 *
 * Copyright (C) 2002  Robert James Kaes (rjkaes@flarenet.com)
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

#ifndef TINYPROXY_NETWORK_H
#define TINYPROXY_NETWORK_H

extern ssize_t safe_write(int fd, const char *buffer, size_t count);
extern ssize_t safe_read(int fd, char *buffer, size_t count);

extern int write_message(int fd, const char *fmt, ...);
extern ssize_t readline(int fd, char **whole_buffer);

#endif
