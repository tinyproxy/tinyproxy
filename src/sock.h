/* $Id: sock.h,v 1.7 2001-11-22 00:19:18 rjkaes Exp $
 *
 * See 'sock.c' for a detailed description.
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

#ifndef TINYPROXY_SOCK_H
#define TINYPROXY_SOCK_H

#include "tinyproxy.h"

#define PEER_IP_LENGTH		16
#define PEER_STRING_LENGTH	256

#define MAXLINE (1024 * 4)

extern int opensock(char *ip_addr, uint16_t port);
extern int listen_sock(uint16_t port, socklen_t * addrlen);

extern int socket_nonblocking(int sock);
extern int socket_blocking(int sock);

extern char *getpeer_ip(int fd, char *ipaddr);
extern char *getpeer_string(int fd, char *string);

extern ssize_t safe_write(int fd, const void *buffer, size_t count);
extern ssize_t safe_read(int fd, void *buffer, size_t count);

extern ssize_t readline(int fd, char **whole_buffer);

#endif
