/* $Id: conns.c,v 1.3 2001-10-25 16:58:09 rjkaes Exp $
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

void initialize_conn(struct conn_s *connptr)
{
	connptr->client_fd = connptr->server_fd = -1;
	connptr->cbuffer = new_buffer();
	connptr->sbuffer = new_buffer();

	connptr->send_message = FALSE;
	connptr->simple_req = FALSE;

	connptr->ssl = FALSE;
	connptr->upstream = FALSE;

	update_stats(STAT_OPEN);
}

void destroy_conn(struct conn_s *connptr)
{
	if (connptr->client_fd != -1)
		close(connptr->client_fd);
	if (connptr->server_fd != -1)
		close(connptr->server_fd);

	if (connptr->cbuffer)
		delete_buffer(connptr->cbuffer);
	if (connptr->sbuffer)
		delete_buffer(connptr->sbuffer);

	safefree(connptr);

	update_stats(STAT_CLOSE);
}
