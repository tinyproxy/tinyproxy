/* $Id: sock.h,v 1.1.1.1 2000-02-16 17:32:23 sdyoung Exp $
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

#ifndef _SOCK_H_
#define _SOCK_H_	1

#include "buffer.h"

#define PEER_IP_LENGTH		16
#define PEER_STRING_LENGTH	256

extern int setup_fd;

extern int opensock(char *ip_addr, int port);
extern int opensock_inaddr(struct in_addr *inaddr, int port);
extern int init_listen_sock(int port);
extern int listen_sock(void);
extern void de_init_listen_sock(void);
extern int setsocketopt(int *sock_fd, int options, int flip);

extern char *getpeer_ip(int fd, char *ipaddr);
extern char *getpeer_string(int fd, char *string);
extern int readline(int fd, struct buffer_s *buffer, char **line);

#endif
