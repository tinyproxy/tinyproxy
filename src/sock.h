/* $Id: sock.h,v 1.13 2004-04-27 18:53:14 rjkaes Exp $
 *
 * See 'sock.c' for a detailed description.
 *
 * Copyright (C) 1998  Steven Young
 * Copyright (C) 1999,2004  Robert James Kaes (rjkaes@users.sourceforge.net)
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

/* The IP length is set to 48, since IPv6 can be that long */
#define IP_LENGTH		48
#define HOSTNAME_LENGTH		1024

#define MAXLINE (1024 * 4)

extern int opensock(const char* host, int port, const char* bind_to);
extern int listen_sock(uint16_t port, socklen_t* addrlen);

extern int socket_nonblocking(int sock);
extern int socket_blocking(int sock);

extern int getsock_ip(int fd, char* ipaddr);
extern int getpeer_information(int fd, char* ipaddr, char* string_addr);

#endif
