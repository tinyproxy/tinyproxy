/* $Id: stats.c,v 1.3 2001-05-23 17:59:08 rjkaes Exp $
 *
 * This module handles the statistics for tinyproxy. There are only two
 * public API functions. The reason for the functions, rather than just a
 * external structure is that tinyproxy is now multi-threaded and we can
 * not allow more than one thread to access the statistics at the same
 * time. This is prevented by a mutex. If there is a need for more
 * statistics in the future, just add to the structure, enum (in the header),
 * and the switch statement in update_stats().
 *
 * Copyright (C) 2000  Robert James Kaes (rjkaes@flarenet.com)
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

#include "log.h"
#include "stats.h"

struct stat_s {
	unsigned long int num_reqs;
	unsigned long int num_badcons;
	unsigned long int num_open;
	unsigned long int num_refused;
	unsigned long int num_denied;
};

static struct stat_s stats;

/*
 * Locking when we're accessing the statistics, since I don't want multiple
 * threads changing the information at the same time.
 */
pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;
#define LOCK()   pthread_mutex_lock(&stats_mutex)
#define UNLOCK() pthread_mutex_unlock(&stats_mutex)

/*
 * Initialise the statistics information to zero.
 */
void init_stats(void)
{
	LOCK();
	memset(&stats, 0, sizeof(stats));
	UNLOCK();
}

/*
 * Display the statics of the tinyproxy server.
 */
int showstats(struct conn_s *connptr)
{
	static char *msg = "HTTP/1.0 200 OK\r\n" \
	    "Content-type: text/html\r\n\r\n" \
	    "<html><head><title>%s (%s) stats</title></head>\r\n" \
	    "<body>\r\n" \
	    "<center><h2>%s (%s) run-time statistics</h2></center><hr>\r\n" \
	    "<blockquote>\r\n" \
	    "Number of open connections: %lu<br>\r\n" \
	    "Number of requests: %lu<br>\r\n" \
	    "Number of bad connections: %lu<br>\r\n" \
            "Number of denied connections: %lu<br>\r\n" \
	    "Number of refused connections due to high load: %lu\r\n" \
	    "</blockquote>\r\n</body></html>\r\n";

	connptr->output_message = malloc(MAXBUFFSIZE);
	if (!connptr->output_message) {
		log(LOG_CRIT, "Out of memory!");
		return -1;
	}

	LOCK();
	snprintf(connptr->output_message, MAXBUFFSIZE, msg,
		PACKAGE, VERSION, PACKAGE, VERSION,
		stats.num_open,
		stats.num_reqs,
		stats.num_badcons,
		stats.num_denied,
		stats.num_refused);
	UNLOCK();

	return 0;
}

/*
 * Update the value of the statistics. The update_level is defined in
 * stats.h
 */
int update_stats(status_t update_level)
{
	LOCK();
	switch(update_level) {
	case STAT_BADCONN:
		stats.num_badcons++;
		break;
	case STAT_OPEN:
		stats.num_open++;
		stats.num_reqs++;
		break;
	case STAT_CLOSE:
		stats.num_open--;
		break;
	case STAT_REFUSE:
		stats.num_refused++;
		break;
	case STAT_DENIED:
		stats.num_denied++;
		break;
	default:
		UNLOCK();
		return -1;
	}
	UNLOCK();

	return 0;
}
