/* $Id: conns.c,v 1.7 2002-04-07 21:32:01 rjkaes Exp $
 *
 * Create and free the connection structure. One day there could be
 * other connnection related tasks put here, but for now the header
 * file and this file are only used for create/free functions and the
 * connection structure definition.
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

#include "tinyproxy.h"

#include "buffer.h"
#include "conns.h"
#include "stats.h"
#include "utils.h"

struct conn_s *
initialize_conn(int client_fd)
{
	struct conn_s *connptr;
	struct buffer_s *cbuffer, *sbuffer;

	assert(client_fd >= 0);

	/*
	 * Allocate the memory for all the internal componets
	 */
	cbuffer = new_buffer();
	sbuffer = new_buffer();

	if (!cbuffer || !sbuffer)
		goto error_exit;

	/*
	 * Allocate the space for the conn_s structure itself.
	 */
	connptr = safemalloc(sizeof(struct conn_s));
	if (!connptr)
		goto error_exit;

	connptr->client_fd = client_fd;
	connptr->server_fd = -1;

	connptr->cbuffer = cbuffer;
	connptr->sbuffer = sbuffer;

	connptr->request_line = NULL;

	connptr->response_message_sent = FALSE;
	connptr->connect_method = FALSE;

	connptr->protocol.major = connptr->protocol.minor = 0;

	update_stats(STAT_OPEN);

	return connptr;

error_exit:
	/*
	 * If we got here, there was a problem allocating memory
	 */
	if (cbuffer)
		delete_buffer(cbuffer);
	if (sbuffer)
		delete_buffer(sbuffer);

	return NULL;
}

void
destroy_conn(struct conn_s *connptr)
{
	assert(connptr != NULL);

	if (connptr->client_fd != -1)
		close(connptr->client_fd);
	if (connptr->server_fd != -1)
		close(connptr->server_fd);

	if (connptr->cbuffer)
		delete_buffer(connptr->cbuffer);
	if (connptr->sbuffer)
		delete_buffer(connptr->sbuffer);

	if (connptr->request_line)
		safefree(connptr->request_line);

	safefree(connptr);

	update_stats(STAT_CLOSE);
}
