/* $Id: utils.c,v 1.1.1.1 2000-02-16 17:32:24 sdyoung Exp $
 *
 * Misc. routines which are used by the various functions to handle strings
 * and memory allocation and pretty much anything else we can think of. Also,
 * the load cutoff routine is in here, along with the HTML show stats
 * function. Could not think of a better place for it, so it's in here.
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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <sysexits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <assert.h>

#include "config.h"
#include "tinyproxy.h"
#include "utils.h"
#include "log.h"
#include "conns.h"
#include "buffer.h"

char *xstrdup(char *st)
{
	char *p;

	assert(st);

	if (!(p = strdup(st))) {
		log("ERROR xstrdup: out of memory (%s)", strerror(errno));
		return NULL;
	} else {
		return p;
	}
}

/*
 * Find the start of the needle in the haystack. Limits the search to less
 * than "length" characters. Returns NULL if the needle is not found.
 */
char *xstrstr(char *haystack, char *needle, unsigned int length,
	      int case_sensitive)
{
	unsigned int i;
	/* Used to specify which function to use... need the decl. */
	int (*fn) (const char *s1, const char *s2, unsigned int n);

	assert(haystack);
	assert(needle);
	assert(length > 0);
	assert(case_sensitive == FALSE || case_sensitive == TRUE);

	if (case_sensitive)
		fn = strncmp;
	else
		fn = strncasecmp;

	if (strlen(needle) > length)
		return NULL;

	for (i = 0; i <= length - strlen(needle); i++) {
		if ((*fn) (haystack + i, needle, strlen(needle)) == 0)
			return haystack + i;

	}

	return NULL;
}

/*
 * for-sure malloc
 */
void *xmalloc(unsigned long int sz)
{
	void *p;

	assert(sz > 0);

	if (!(p = malloc((size_t) sz))) {
		log("ERROR xmalloc: out of memory (%s)", strerror(errno));
		return NULL;
	}
	return p;
}

#ifdef USE_PROC
int calcload(void)
{
	char buf[BUFFER], *p;
	FILE *f;

	if (!config.cutoffload) {
		return -1;
	}

	if (!(f = fopen("/proc/loadavg", "rt"))) {
		log("unable to read /proc/loadavg");
		config.cutoffload = 0.0;
		return -1;
	}
	fgets(buf, BUFFER, f);
	p = strchr(buf, ' ');
	*p = '\0';
	load = atof(buf);
	fclose(f);
	return 0;
}

#else
int calcload(void)
{
	FILE *f;
	char buf[BUFFER];
	char *p, *y;

	if (!config.cutoffload) {
		return -1;
	}

	if (!(f = popen(UPTIME_PATH, "r"))) {
		log("calcload: unable to exec uptime");
		config.cutoffload = 0.0;
		return -1;
	}
	fgets(buf, BUFFER, f);
	p = strrchr(buf, ':');
	p += 2;
	y = strchr(p, ',');
	*y = '\0';
	load = atof(p);
	pclose(f);
	return 0;
}

#endif

/*
 * Delete the server's buffer and replace it with a premade message which will
 * be sent to the client.
 */
static void update_output_buffer(struct conn_s *connptr, char *outbuf)
{
	assert(connptr);
	assert(outbuf);

	delete_buffer(connptr->sbuffer);
	connptr->sbuffer = new_buffer();

	push_buffer(connptr->sbuffer, outbuf, strlen(outbuf));
	shutdown(connptr->server_fd, 2);
	connptr->type = CLOSINGCONN;
}

/*
 * Display the statics of the tinyproxy server.
 */
int showstats(struct conn_s *connptr)
{
	char *outbuf;
	static char *msg = "HTTP/1.0 200 OK\r\n" \
	    "Content-type: text/html\r\n\r\n" \
	    "<html><head><title>%s stats</title></head>\r\n" \
	    "<body>\r\n" \
	    "<center><h2>%s run-time statistics</h2></center><hr>\r\n" \
	    "<blockquote>\r\n" \
	    "Number of requests: %lu<br>\r\n" \
	    "Number of connections: %lu<br>\r\n" \
	    "Number of bad connections: %lu<br>\r\n" \
	    "Number of opens: %lu<br>\r\n" \
	    "Number of listens: %lu<br>\r\n" \
	    "Number of bytes (tx): %lu<br>\r\n" \
	    "Number of bytes (rx): %lu<br>\r\n" \
	    "Number of garbage collects:%lu<br>\r\n" \
	    "Number of idle connection kills:%lu<br>\r\n" \
	    "Number of refused connections due to high load:%lu<br>\r\n" \
	    "Current system load average:%.2f" \
	    "(recalculated every % lu seconds)<br>\r\n" \
	    "</blockquote>\r\n</body></html>\r\n";

	assert(connptr);

	outbuf = xmalloc(BUFFER);

	sprintf(outbuf, msg, VERSION, VERSION, stats.num_reqs,
		stats.num_cons, stats.num_badcons, stats.num_opens,
		stats.num_listens, stats.num_tx, stats.num_rx,
		stats.num_garbage, stats.num_idles, stats.num_refused, load,
		LOAD_RECALCTIMER);

	update_output_buffer(connptr, outbuf);

	return 0;
}

/*
 * Display an error to the client.
 */
int httperr(struct conn_s *connptr, int err, char *msg)
{
	char *outbuf;
	static char *premsg = "HTTP/1.0 %d %s\r\n" \
	    "Content-type: text/html\r\n\r\n" \
	    "<html><head><title>%s</title></head>\r\n" \
	    "<body>\r\n" \
	    "<font size=\"+2\">Cache Error!</font><br>\r\n" \
	    "An error of type %d occurred: %s\r\n" \
	    "<hr>\r\n" \
	    "<font size=\"-1\"><em>Generated by %s</em></font>\r\n" \
	    "</body></html>\r\n";

	assert(connptr);
	assert(err > 0);
	assert(msg);

	outbuf = xmalloc(BUFFER);
	sprintf(outbuf, premsg, err, msg, msg, err, msg, VERSION);

	update_output_buffer(connptr, outbuf);

	return 0;
}

void makedaemon(void)
{
	if (fork() != 0)
		exit(0);

	setsid();
	signal(SIGHUP, SIG_IGN);

	if (fork() != 0)
		exit(0);

	chdir("/");
	umask(0);

	close(0);
	close(1);
	close(2);
}
