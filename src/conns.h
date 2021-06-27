/* tinyproxy - A fast light-weight HTTP proxy
 * Copyright (C) 2001 Robert James Kaes <rjkaes@users.sourceforge.net>
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

/* See 'conns.c' for detailed information. */

#ifndef TINYPROXY_CONNS_H
#define TINYPROXY_CONNS_H

#include "main.h"
#include "hsearch.h"

enum connect_method_e {
        CM_FALSE = 0,
        CM_TRUE = 1,
        CM_UPGRADE = 2,
};

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

        enum connect_method_e connect_method;

        /* Booleans */
        unsigned int show_stats;

        /*
         * This structure stores key -> value mappings for substitution
         * in the error HTML files.
         */
        struct htab *error_variables;

        int error_number;
        char *error_string;

        /* A Content-Length value from the remote server */
        struct {
                long int server;
                long int client;
        } content_length;

        /*
         * Store the server's IP (for BindSame)
         */
        char *server_ip_addr;

        /*
         * Store the client's IP information
         */
        char *client_ip_addr;

        /*
         * Store the incoming request's HTTP protocol.
         */
        struct {
                unsigned int major;
                unsigned int minor;
        } protocol;

#ifdef REVERSE_SUPPORT
        /*
         * Place to store the current per-connection reverse proxy path
         */
        char *reversepath;
#endif

        /*
         * Pointer to upstream proxy.
         */
        struct upstream *upstream_proxy;
};

/* expects pointer to zero-initialized struct, set up struct
   with default values for initial use */
extern void conn_struct_init(struct conn_s *connptr);

/* second stage initializiation, sets up buffers and connection details */
extern int conn_init_contents (struct conn_s *connptr, const char *ipaddr,
                                       const char *sock_ipaddr);
extern void conn_destroy_contents (struct conn_s *connptr);

#endif
