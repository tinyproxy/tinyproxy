/* $Id: conns.h,v 1.1.1.1 2000-02-16 17:32:22 sdyoung Exp $
 *
 * See 'conns.c' for a detailed description.
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

#ifndef _CONN_H_
#define _CONN_H_	1

#include "tinyproxy.h"

#include <adns.h>

/* Different connection types */
enum conn_type {
	NEWCONN,
	WAITCONN,
	DNS_WAITCONN,
	RELAYCONN,
	CLOSINGCONN,
	FINISHCONN
};

struct conn_s {
	struct conn_s *next;
	int client_fd, server_fd;
	enum conn_type type;
	struct buffer_s *cbuffer;
	struct buffer_s *sbuffer;
	time_t inittime, actiontime;
	flag clientheader, serverheader;
	flag simple_req;

	adns_query adns_qu;
	char *domain;
	int port_no;
};

extern struct conn_s *connections;

/* Handle the creation and deletion of connections */
extern struct conn_s *new_conn(int fd);
extern int del_conn(struct conn_s *connptr);
extern void conncoll(void);
extern void garbcoll(void);

#endif
