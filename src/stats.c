/* $Id: stats.c,v 1.13 2003-03-13 21:31:03 rjkaes Exp $
 *
 * This module handles the statistics for tinyproxy. There are only two
 * public API functions. The reason for the functions, rather than just a
 * external structure is that tinyproxy is now multi-threaded and we can
 * not allow more than one child to access the statistics at the same
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
#include "heap.h"
#include "htmlerror.h"
#include "stats.h"
#include "utils.h"

struct stat_s {
	unsigned long int num_reqs;
	unsigned long int num_badcons;
	unsigned long int num_open;
	unsigned long int num_refused;
	unsigned long int num_denied;
};

static struct stat_s *stats;

/*
 * Initialize the statistics information to zero.
 */
void
init_stats(void)
{
	stats = malloc_shared_memory(sizeof(struct stat_s));
	if (stats == MAP_FAILED)
		return;

	memset(stats, 0, sizeof(struct stat));
}

/*
 * Display the statics of the tinyproxy server.
 */
int
showstats(struct conn_s *connptr)
{
	static char *msg =
	    "<html><head><title>%s (%s) stats</title></head>\r\n"
	    "<body>\r\n"
	    "<center><h2>%s (%s) run-time statistics</h2></center><hr>\r\n"
	    "<blockquote>\r\n"
	    "Number of open connections: %lu<br>\r\n"
	    "Number of requests: %lu<br>\r\n"
	    "Number of bad connections: %lu<br>\r\n"
	    "Number of denied connections: %lu<br>\r\n"
	    "Number of refused connections due to high load: %lu\r\n"
	    "</blockquote>\r\n</body></html>\r\n";

	char *message_buffer;
	char opens[16], reqs[16], badconns[16], denied[16], refused[16];
	FILE *statfile;

	snprintf(opens, sizeof(opens), "%lu", stats->num_open);
	snprintf(reqs, sizeof(reqs), "%lu", stats->num_reqs);
	snprintf(badconns, sizeof(badconns), "%lu", stats->num_badcons);
	snprintf(denied, sizeof(denied), "%lu", stats->num_denied);
	snprintf(refused, sizeof(refused), "%lu", stats->num_refused);

	if (!config.statpage || (!(statfile = fopen(config.statpage, "r")))) {
		message_buffer = safemalloc(MAXBUFFSIZE);
		if (!message_buffer)
			return -1;

		snprintf(message_buffer, MAXBUFFSIZE, msg,
			 PACKAGE, VERSION, PACKAGE, VERSION,
			 stats->num_open,
			 stats->num_reqs,
			 stats->num_badcons, stats->num_denied, stats->num_refused);

		if (send_http_message(connptr, 200, "OK", message_buffer) < 0) {
			safefree(message_buffer);
			return -1;
		}

		safefree(message_buffer);
		return 0;
	}

	add_error_variable(connptr, "opens", opens);
	add_error_variable(connptr, "reqs", reqs);
	add_error_variable(connptr, "badconns", badconns);
	add_error_variable(connptr, "denied", denied);
	add_error_variable(connptr, "refused", refused);
	add_standard_vars(connptr);
	send_http_headers(connptr, 200, "Statistic requested");
	send_html_file(statfile, connptr);
	fclose(statfile);

	return 0;
}

/*
 * Update the value of the statistics. The update_level is defined in
 * stats.h
 */
int
update_stats(status_t update_level)
{
	switch (update_level) {
	case STAT_BADCONN:
		++stats->num_badcons;
		break;
	case STAT_OPEN:
		++stats->num_open;
		++stats->num_reqs;
		break;
	case STAT_CLOSE:
		--stats->num_open;
		break;
	case STAT_REFUSE:
		++stats->num_refused;
		break;
	case STAT_DENIED:
		++stats->num_denied;
		break;
	default:
		return -1;
	}

	return 0;
}
