/* $Id: conns.h,v 1.10 2002-05-23 18:23:29 rjkaes Exp $
 *
 * See 'conns.c' for a detailed description.
 *
 * Copyright (C) 2001  Robert James Kaes (rjkaes@flarenet.com)
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

#ifndef TINYPROXY_CONNS_H
#define TINYPROXY_CONNS_H

#include "tinyproxy.h"

/*
 * Connection Definition
 */
struct conn_s {
	int client_fd;
	int server_fd;

	struct buffer_s *cbuffer;
	struct buffer_s *sbuffer;

	/* The request line (first line) from the client */
	char *request_line;

	bool_t connect_method;
	bool_t show_stats;

	/* Store the error response if there is one */
	char *error_string;
	int error_number;

	/* A Content-Length value from the remote server */
	long remote_content_length;

	/*
	 * Store the client's IP and hostname information
	 */
	char* client_ip_addr;
	char* client_string_addr;

	/*
	 * Store the incoming request's HTTP protocol.
	 */
	struct {
		unsigned int major;
		unsigned int minor;
	} protocol;
};

/*
 * Functions for the creation and destruction of a connection structure.
 */
extern struct conn_s* initialize_conn(int client_fd, const char* ipaddr,
				      const char* string_addr);
extern void destroy_conn(struct conn_s *connptr);

#endif
