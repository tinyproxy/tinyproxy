/* $Id: conns.c,v 1.1.1.1 2000-02-16 17:32:22 sdyoung Exp $
 *
 * These functions handle the various stages a connection will go through in
 * the course of its life in tinyproxy. New connections are initialized and
 * added to the linked list of active connections. As these connections are
 * completed, they are closed, and the a garbage collection process removes
 * them from the list.
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

#ifdef HAVE_CONFIG_H
#include <defines.h>
#endif

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sysexits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <assert.h>

#include <adns.h>

extern adns_state adns;

#include "config.h"
#include "log.h"
#include "utils.h"
#include "conns.h"
#include "buffer.h"
#include "dnscache.h"

struct conn_s *connections = NULL;

/*
 * Add a new connection to the linked list
 */
struct conn_s *new_conn(int fd)
{
	struct conn_s **rpConnptr = &connections;
	struct conn_s *connptr = connections;
	struct conn_s *newconn;

	assert(fd >= 0);

	while (connptr) {
		rpConnptr = &connptr->next;
		connptr = connptr->next;
	}

	if (!(newconn = xmalloc(sizeof(struct conn_s)))) {
		log("ERROR new_conn: could not allocate memory for conn");
		return NULL;
	}

	/* Allocate the new buffer */
	newconn->cbuffer = NULL;
	newconn->sbuffer = NULL;
	if (!(newconn->cbuffer = new_buffer())
	    || !(newconn->sbuffer = new_buffer())) {
		log("ERROR new_conn: could not allocate memory for buffer");
		safefree(newconn->cbuffer);
		safefree(newconn->sbuffer);

		newconn->next = NULL;
		safefree(newconn);
		return NULL;
	}

	newconn->client_fd = fd;
	newconn->server_fd = -1;
	newconn->type = NEWCONN;
	newconn->inittime = newconn->actiontime = time(NULL);

	newconn->clientheader = newconn->serverheader = FALSE;
	newconn->simple_req = FALSE;

	*rpConnptr = newconn;
	newconn->next = connptr;

	stats.num_cons++;

	return newconn;
}

/*
 * Delete a connection from the linked list
 */
int del_conn(struct conn_s *delconn)
{
	struct conn_s **rpConnptr = &connections;
	struct conn_s *connptr = connections;

	assert(delconn);

	if (delconn->cbuffer) {
		delete_buffer(delconn->cbuffer);
		delconn->cbuffer = NULL;
	}
	if (delconn->sbuffer) {
		delete_buffer(delconn->sbuffer);
		delconn->sbuffer = NULL;
	}

	close(delconn->client_fd);
	close(delconn->server_fd);

	while (connptr && (connptr != delconn)) {
		rpConnptr = &connptr->next;
		connptr = connptr->next;
	}

	if (connptr == delconn) {
		*rpConnptr = delconn->next;
		safefree(delconn);
	}

	return 0;
}

/*
 * Check for connections that have been idle too long
 */
void conncoll(void)
{
	struct conn_s *connptr = connections;

	while (connptr) {
		if (
		    (difftime(time(NULL), connptr->actiontime) >
		     STALECONN_TIME) && connptr->type != FINISHCONN) {
			connptr->type = FINISHCONN;
			stats.num_idles++;
		}
		connptr = connptr->next;
	}
}

/*
 * Actually remove all entries in the linked list that have been marked for
 * deletion.
 */
void garbcoll(void)
{
	struct conn_s *connptr = connections;
	struct conn_s *tmp;

	static unsigned int dnscount = 0;

#ifdef __DEBUG__
	log("Garbage collecting (%lu)", stats.num_garbage);
#endif

	stats.num_garbage++;

	while (connptr) {
		tmp = connptr->next;
		if (connptr->type == FINISHCONN) {
#ifdef __DEBUG__
			log("Deleting connection: %p", connptr);
#endif
			del_conn(connptr);
		}
		connptr = tmp;
	}

	if (dnscount++ > DNS_GARBAGE_COL) {
		dnscount = 0;
		dnsclean();
	}
}
