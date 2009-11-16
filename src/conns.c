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

/* Create and free the connection structure. One day there could be
 * other connection related tasks put here, but for now the header
 * file and this file are only used for create/free functions and the
 * connection structure definition.
 */

#include "main.h"

#include "buffer.h"
#include "conns.h"
#include "heap.h"
#include "log.h"
#include "stats.h"

struct conn_s *initialize_conn (int client_fd, const char *ipaddr,
                                const char *string_addr,
                                const char *sock_ipaddr)
{
        struct conn_s *connptr;
        struct buffer_s *cbuffer, *sbuffer;

        assert (client_fd >= 0);

        /*
         * Allocate the memory for all the internal components
         */
        cbuffer = new_buffer ();
        sbuffer = new_buffer ();

        if (!cbuffer || !sbuffer)
                goto error_exit;

        /*
         * Allocate the space for the conn_s structure itself.
         */
        connptr = (struct conn_s *) safemalloc (sizeof (struct conn_s));
        if (!connptr)
                goto error_exit;

        connptr->client_fd = client_fd;
        connptr->server_fd = -1;

        connptr->cbuffer = cbuffer;
        connptr->sbuffer = sbuffer;

        connptr->request_line = NULL;

        /* These store any error strings */
        connptr->error_variables = NULL;
        connptr->error_string = NULL;
        connptr->error_number = -1;

        connptr->connect_method = FALSE;
        connptr->show_stats = FALSE;

        connptr->protocol.major = connptr->protocol.minor = 0;

        /* There is _no_ content length initially */
        connptr->content_length.server = connptr->content_length.client = -1;

        connptr->server_ip_addr = (sock_ipaddr ?
                                   safestrdup (sock_ipaddr) : NULL);
        connptr->client_ip_addr = safestrdup (ipaddr);
        connptr->client_string_addr = safestrdup (string_addr);

        connptr->upstream_proxy = NULL;

        update_stats (STAT_OPEN);

#ifdef REVERSE_SUPPORT
        connptr->reversepath = NULL;
#endif

        return connptr;

error_exit:
        /*
         * If we got here, there was a problem allocating memory
         */
        if (cbuffer)
                delete_buffer (cbuffer);
        if (sbuffer)
                delete_buffer (sbuffer);

        return NULL;
}

void destroy_conn (struct conn_s *connptr)
{
        assert (connptr != NULL);

        if (connptr->client_fd != -1)
                if (close (connptr->client_fd) < 0)
                        log_message (LOG_INFO, "Client (%d) close message: %s",
                                     connptr->client_fd, strerror (errno));
        if (connptr->server_fd != -1)
                if (close (connptr->server_fd) < 0)
                        log_message (LOG_INFO, "Server (%d) close message: %s",
                                     connptr->server_fd, strerror (errno));

        if (connptr->cbuffer)
                delete_buffer (connptr->cbuffer);
        if (connptr->sbuffer)
                delete_buffer (connptr->sbuffer);

        if (connptr->request_line)
                safefree (connptr->request_line);

        if (connptr->error_variables)
                hashmap_delete (connptr->error_variables);

        if (connptr->error_string)
                safefree (connptr->error_string);

        if (connptr->server_ip_addr)
                safefree (connptr->server_ip_addr);
        if (connptr->client_ip_addr)
                safefree (connptr->client_ip_addr);
        if (connptr->client_string_addr)
                safefree (connptr->client_string_addr);

#ifdef REVERSE_SUPPORT
        if (connptr->reversepath)
                safefree (connptr->reversepath);
#endif

        safefree (connptr);

        update_stats (STAT_CLOSE);
}
