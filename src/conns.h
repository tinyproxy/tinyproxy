/* $Id: conns.h,v 1.14.2.1 2004-08-06 16:56:55 rjkaes Exp $
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

	/* Booleans */
	unsigned int connect_method;
	unsigned int show_stats;

	/*
	 * Store the error response if there is one.
	 * This structure stores key -> value mappings for substitution
	 * in the error HTML files.
	 */
	struct error_variable_s {
		char *error_key;
		char *error_val;
	} **error_variables;
	int error_variable_count;

	int error_number;
	char *error_string;

	/* A Content-Length value from the remote server */
	struct {
		long int server;
		long int client;
	} content_length;

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
        /*
         * Pointer to upstream proxy.
         */
        struct upstream *upstream_proxy;
};

/*
 * Functions for the creation and destruction of a connection structure.
 */
extern struct conn_s* initialize_conn(int client_fd, const char* ipaddr,
				      const char* string_addr);
extern void destroy_conn(struct conn_s *connptr);

#endif
