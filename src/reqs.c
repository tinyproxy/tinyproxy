/* $Id: reqs.c,v 1.7 2000-03-31 22:55:22 rjkaes Exp $
 *
 * This is where all the work in tinyproxy is actually done. Incoming
 * connections are added to the active list of connections and then the header
 * is processed to determine what is being requested. tinyproxy then connects
 * to the remote server and begins to relay the bytes back and forth between
 * the client and the remote server. Basically, we sit there and sling bytes
 * back and forth. Optionally, we can weed out headers we do not want to send,
 * and also add a header of our own.
 *
 * Copyright (C) 1998  Steven Young
 * Copyright (C) 1999  Robert James Kaes (rjkaes@flarenet.com)
 * Copyright (C) 2000  Chris Lightfoot (chris@ex-parrot.com)
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
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sysexits.h>
#include <assert.h>

#include "config.h"
#include "tinyproxy.h"
#include "sock.h"
#include "utils.h"
#include "conns.h"
#include "log.h"
#include "reqs.h"
#include "buffer.h"
#include "filter.h"
#include "uri.h"
#include "regexp.h"
#include "anonymous.h"

/* chris - for asynchronous DNS */
#include "dnscache.h"
#include <adns.h>
extern adns_state adns;

#ifdef XTINYPROXY

static void Add_XTinyproxy_Header(struct conn_s *connptr)
{
	char *header_line;
	char ipaddr[PEER_IP_LENGTH];
	int length;

	assert(connptr);

	if (!(header_line = xmalloc(sizeof(char) * 32)))
		return;

	length = sprintf(header_line, "X-Tinyproxy: %s\r\n",
			 getpeer_ip(connptr->client_fd, ipaddr));

	unshift_buffer(connptr->cbuffer, header_line, length);
}

#endif

#define HTTPPATTERN "^([a-z]+)[ \t]+([^ \t]+)([ \t]+(HTTP/[0-9]+\\.[0-9]+))?"
#define NMATCH 4
#define METHOD_IND   1
#define URI_IND      2
#define VERSION_MARK 3
#define VERSION_IND  4

#define HTTP400ERROR "Unrecognizable request. Only HTTP is allowed."
#define HTTP500ERROR "Unable to connect to remote server."
#define HTTP503ERROR "Internal server error."

/*
 * Parse a client HTTP request and then establish connection.
 */
static int clientreq(struct conn_s *connptr)
{
	URI *uri = NULL;
	char *inbuf, *buffer, *request, *port;
	char *inbuf_ptr;

	regex_t preg;
	regmatch_t pmatch[NMATCH];

	long len;
	int fd, port_no;

	char peer_ipaddr[PEER_IP_LENGTH];

	assert(connptr);

	getpeer_ip(connptr->client_fd, peer_ipaddr);
	/* chris - getpeer_string could block, so for the moment take it out */

	if ((len
	     = readline(connptr->client_fd, connptr->cbuffer, &inbuf)) <= 0) {
		return len;
	}

	inbuf_ptr = inbuf + len - 1;
	while (*inbuf_ptr == '\r' || *inbuf_ptr == '\n')
		*inbuf_ptr-- = '\0';

	/* Log the incoming connection */
	if (!config.restricted) {
		log("Connect: %s", peer_ipaddr);
		log("Request: %s", inbuf);
	}

	if (regcomp(&preg, HTTPPATTERN, REG_EXTENDED | REG_ICASE) != 0) {
		log("ERROR clientreq: regcomp");
		return 0;
	}
	if (regexec(&preg, inbuf, NMATCH, pmatch, 0) != 0) {
		log("ERROR clientreq: regexec");
		regfree(&preg);
		return -1;
	}
	regfree(&preg);

	if (pmatch[VERSION_MARK].rm_so == -1)
		connptr->simple_req = TRUE;

	if (pmatch[METHOD_IND].rm_so == -1 || pmatch[URI_IND].rm_so == -1) {
		log("ERROR clientreq: Incomplete line from %s (%s)",
		    peer_ipaddr, inbuf);
		httperr(connptr, 400, HTTP400ERROR);
		goto COMMON_EXIT;
	}

	len = pmatch[URI_IND].rm_eo - pmatch[URI_IND].rm_so;
	if (!(buffer = xmalloc(len + 1))) {
		log("ERROR clientreq: Cannot allocate buffer for request from %s",
		    peer_ipaddr);
		httperr(connptr, 503, HTTP503ERROR);
		goto COMMON_EXIT;
	}
	memcpy(buffer, inbuf + pmatch[URI_IND].rm_so, len);
	buffer[len] = '\0';
	if (!(uri = explode_uri(buffer))) {
		safefree(buffer);
		log("ERROR clientreq: Problem with explode_uri");
		httperr(connptr, 503, HTTP503ERROR);
		goto COMMON_EXIT;
	}
	safefree(buffer);

	if (!uri->scheme || strcasecmp(uri->scheme, "http") != 0) {
		char *error_string;
		if (uri->scheme) {
			error_string = xmalloc(strlen(uri->scheme) + 64);
			sprintf(error_string,
				"Invalid scheme (%s). Only HTTP is allowed.",
				uri->scheme);
		} else {
			error_string = strdup("Invalid scheme (NULL). Only HTTP is allowed.");
		}

		httperr(connptr, 400, error_string);
		safefree(error_string);
		goto COMMON_EXIT;
	}

	if (!uri->authority) {
		httperr(connptr, 400, "Invalid authority.");
		goto COMMON_EXIT;
	}

	if ((strlen(config.stathost) > 0) &&
	    strcasecmp(uri->authority, config.stathost) == 0) {
		showstats(connptr);
		goto COMMON_EXIT;
	}

	port_no = 80;
	if ((port = strchr(uri->authority, ':'))) {
		*port++ = '\0';
		if (strlen(port) > 0)
			port_no = atoi(port);
	}

	/* chris - so this can be passed on to clientreq_dnscomplete. */
	connptr->port_no = port_no;

#ifdef FILTER_ENABLE
	/* Filter domains out */
	if (config.filter) {
		if (filter_host(uri->authority)) {
			log("ERROR clientreq: Filtered connection (%s)",
			    peer_ipaddr);
			httperr(connptr, 404,
				"Unable to connect to filtered host.");
			goto COMMON_EXIT;
		}
	}
#endif				/* FILTER_ENABLE */

	/* Build a new request from the first line of the header */
	if (!(request = xmalloc(strlen(inbuf) + 1))) {
		log("ERROR clientreq: cannot allocate buffer for request from %s",
		    peer_ipaddr);
		httperr(connptr, 503, HTTP503ERROR);
		goto COMMON_EXIT;
	}

	/* We need to set the version number WE support */
	memcpy(request, inbuf, pmatch[METHOD_IND].rm_eo);
	request[pmatch[METHOD_IND].rm_eo] = '\0';
	strcat(request, " ");
	if (strlen(uri->path) > 0) {
		strcat(request, uri->path);
		if (uri->query) {
			strcat(request, "?");
			strcat(request, uri->query);
		}
	} else {
		strcat(request, "/");
	}
	strcat(request, " HTTP/1.0\r\n");

	/* chris - If domain is in dotted-quad format or is already in the
	 * DNS cache, then go straight to WAITCONN.
	 */
	if (inet_aton(uri->authority, NULL)
	    || lookup(NULL, uri->authority)) {
		if ((fd = opensock(uri->authority, port_no)) < 0) {
			safefree(request);
			httperr(connptr, 500,
				"Unable to connect to host (cannot create sock)");
			stats.num_badcons++;
			goto COMMON_EXIT;
		}

		connptr->server_fd = fd;
		connptr->type = WAITCONN;
	}
	/* Otherwise submit a DNS request and hope for the best. */
	else {
		if (adns_submit(adns, uri->authority, adns_r_a, adns_qf_quoteok_cname | adns_qf_cname_loose, connptr, &(connptr->adns_qu))) {
			safefree(request);
			httperr(connptr, 500, "Resolver error connecting to host");
			stats.num_badcons++;
			goto COMMON_EXIT;
		} else {
#ifdef __DEBUG__
			log("DNS request submitted");
#endif
			/* copy domain for later caching */
			connptr->domain = strdup(uri->authority);
			connptr->type = DNS_WAITCONN;
		}
	}

#ifdef XTINYPROXY
	/* Add a X-Tinyproxy header which contains our IP address */
	if (config.my_domain
	    && xstrstr(uri->authority, config.my_domain,
		       strlen(uri->authority), FALSE)) {
		Add_XTinyproxy_Header(connptr);
	}
#endif

	/* Add the rewritten request to the buffer */
	unshift_buffer(connptr->cbuffer, request, strlen(request));

      COMMON_EXIT:
	safefree(inbuf);
	free_uri(uri);
	return 0;
}

/* chris - added this to move a connection from the DNS_WAITCONN state
 * to the WAITCONN state by connecting it to the server, once the name
 * has been resolved.
 */
static int clientreq_dnscomplete(struct conn_s *connptr, struct in_addr *inaddr) {
	int fd;

	fd = opensock_inaddr(inaddr, connptr->port_no);

	if (fd < 0) {
#ifdef __DEBUG__
		log("Failed to open connection to server");
#endif
		httperr(connptr, 500,
			"Unable to connect to host (cannot create sock)");
		stats.num_badcons++;

		return 0;
	} else {
#ifdef __DEBUG__
		log("Connected to server");
#endif
		connptr->server_fd = fd;
		connptr->type = WAITCONN;
	}

	return 0;
}

/*
 * Finish the client request
 */
static int clientreq_finish(struct conn_s *connptr)
{
	int sockopt, len = sizeof(sockopt);

	assert(connptr);

	if (getsockopt
	    (connptr->server_fd, SOL_SOCKET, SO_ERROR, &sockopt, &len) < 0) {
		log("ERROR clientreq_finish: getsockopt error (%s)",
		    strerror(errno));
		return -1;
	}

	if (sockopt != 0) {
		if (sockopt == EINPROGRESS)
			return 0;
		else if (sockopt == ETIMEDOUT) {
			httperr(connptr, 408, "Connect Timed Out");
			return 0;
		} else {
			log("ERROR clientreq_finish: could not create connection (%s)",
			    strerror(sockopt));
			return -1;
		}
	}

	connptr->type = RELAYCONN;
	stats.num_opens++;
	return 0;
}

/*
 * Check to see if the line is allowed or not depending on the anonymous
 * headers which are to be allowed.
 */
static int anonheader(char *line)
{
	char *buffer, *ptr;
	int ret;

	assert(line);

	if ((ptr = xstrstr(line, ":", strlen(line), FALSE)) == NULL)
		return 0;

	ptr++;

	if ((buffer = xmalloc(ptr - line + 1)) == NULL)
		return 0;

	memcpy(buffer, line, ptr - line);
	buffer[ptr - line] = '\0';

	ret = anon_search(buffer);
	free(buffer);
	return ret;
}

/*
 * Used to read in the lines from the header (client side) when we're doing
 * the anonymous header reduction.
 */
static int readanonconn(struct conn_s *connptr)
{
	char *line = NULL;
	int retv;

	assert(connptr);

	if ((retv = readline(connptr->client_fd, connptr->cbuffer, &line)) <=
	    0) {
		return retv;
	}

	if ((line[0] == '\n') || (strncmp(line, "\r\n", 2) == 0)) {
		connptr->clientheader = TRUE;
	} else if (!anonheader(line)) {
		safefree(line);
		return 0;
	}
	
	push_buffer(connptr->cbuffer, line, strlen(line));
	return 0;
}

/*
 * Read in the bytes from the socket
 */
static int readconn(int fd, struct buffer_s *buffptr)
{
	int bytesin;

	assert(fd >= 0);
	assert(buffptr);

	if ((bytesin = readbuff(fd, buffptr)) < 0) {
		return -1;
	}
#ifdef __DEBUG__
	log("readconn [%d]: %d", fd, bytesin);
#endif

	stats.num_rx += bytesin;
	return bytesin;
}

/*
 * Write the bytes from the buffer to the socket
 */
static int writeconn(int fd, struct buffer_s *buffptr)
{
	int bytessent;

	assert(fd >= 0);
	assert(buffptr);

	if ((bytessent = writebuff(fd, buffptr)) < 0) {
		return -1;
	}

	stats.num_tx += bytessent;
	return bytessent;
}

/*
 * Factored out the common function to read from the client. It was used in
 * two different places with no change, so no point in having the same code
 * twice.
 */
static int read_from_client(struct conn_s *connptr, fd_set * readfds)
{
	assert(connptr);
	assert(readfds);

	if (FD_ISSET(connptr->client_fd, readfds)) {
		if (config.anonymous && !connptr->clientheader) {
			if (readanonconn(connptr) < 0) {
				shutdown(connptr->client_fd, 2);
				shutdown(connptr->server_fd, 2);
				connptr->type = FINISHCONN;
				return -1;
			}
		} else if (readconn(connptr->client_fd, connptr->cbuffer) < 0) {
			shutdown(connptr->client_fd, 2);
			shutdown(connptr->server_fd, 2);
			connptr->type = FINISHCONN;
			return -1;
		}
		connptr->actiontime = time(NULL);
	}

	return 0;
}

/*
 * Factored out the common write to client function since, again, it was used
 * in two different places with no changes.
 */
static int write_to_client(struct conn_s *connptr, fd_set * writefds)
{
	assert(connptr);
	assert(writefds);

	if (FD_ISSET(connptr->client_fd, writefds)) {
		if (writeconn(connptr->client_fd, connptr->sbuffer) < 0) {
			shutdown(connptr->client_fd, 2);
			shutdown(connptr->server_fd, 2);
			connptr->type = FINISHCONN;
			return -1;
		}
		connptr->actiontime = time(NULL);
	}

	return 0;
}

/*
 * All of the *_req functions handle the various stages a connection can go
 * through. I moved them out from getreqs because they handle a lot of error
 * code and it was indenting too far in. As you can see they are very simple,
 * and are only called once from getreqs, hence the inlining.
 */

inline static void newconn_req(struct conn_s *connptr, fd_set * readfds)
{
#ifdef UPSTREAM_PROXY
	int fd;
	char peer_ipaddr[PEER_IP_LENGTH];	
#endif	/* UPSTREAM_PROXY */

	assert(connptr);
	assert(readfds);

	if (FD_ISSET(connptr->client_fd, readfds)) {
#ifdef UPSTREAM_PROXY
		if (config.upstream_name && config.upstream_port != 0) {
			getpeer_ip(connptr->client_fd, peer_ipaddr);
			/* Log the incoming connection */
			if (!config.restricted) {
				log("Connect (upstream): %s", peer_ipaddr);
			}

			if (inet_aton(config.upstream_name, NULL)
			    || lookup(NULL, config.upstream_name)) {
				if ((fd = opensock(config.upstream_name, config.upstream_port)) < 0) {
					httperr(connptr, 500,
						"Unable to connect to host (cannot create sock)");
					stats.num_badcons++;
					return;
				}

				connptr->server_fd = fd;
				connptr->type = WAITCONN;
			}
			/* Otherwise submit a DNS request and hope for
			   the best. */
			else {
				if (adns_submit(adns, config.upstream_name, adns_r_a, adns_qf_quoteok_cname | adns_qf_cname_loose, connptr, &(connptr->adns_qu))) {
					httperr(connptr, 500, "Resolver error connecting to host");
					stats.num_badcons++;
					return;
				} else {
#ifdef __DEBUG__
					log("DNS request submitted");
#endif
					/* copy domain for later caching */
					connptr->domain = xstrdup(config.upstream_name);
					connptr->port_no = config.upstream_port;
					connptr->type = DNS_WAITCONN;
				}
			}
		} else {
#endif
			if (clientreq(connptr) < 0) {
				shutdown(connptr->client_fd, 2);
				connptr->type = FINISHCONN;
				return;
			}

			if (!connptr)
				abort();

			connptr->actiontime = time(NULL);
#ifdef UPSTREAM_PROXY
		}
#endif	/* UPSTREAM_PROXY */
	}
}

inline static void waitconn_req(struct conn_s *connptr, fd_set * readfds,
				fd_set * writefds)
{
	assert(connptr);
	assert(readfds);
	assert(writefds);

	if (read_from_client(connptr, readfds) < 0)
		return;

	if (FD_ISSET(connptr->server_fd, readfds)
	    || FD_ISSET(connptr->server_fd, writefds)) {
		if (clientreq_finish(connptr) < 0) {
			shutdown(connptr->server_fd, 2);
			shutdown(connptr->client_fd, 2);
			connptr->type = FINISHCONN;
			return;
		}
		connptr->actiontime = time(NULL);
	}
}

inline static void relayconn_req(struct conn_s *connptr, fd_set * readfds,
				 fd_set * writefds)
{
	assert(connptr);
	assert(readfds);
	assert(writefds);

	if (read_from_client(connptr, readfds) < 0)
		return;

	if (FD_ISSET(connptr->server_fd, readfds)) {
		if (connptr->serverheader) {
			if (readconn(connptr->server_fd, connptr->sbuffer) < 0) {
				shutdown(connptr->server_fd, 2);
				connptr->type = CLOSINGCONN;
				return;
			}
		} else {
			/*
			 * We need to read in the first line to rewrite the
			 * version back down to HTTP/1.0 (if needed)
			 */
			char *line = NULL, *ptr, *newline;
			int retv;

			if (
			    (retv =
			     readline(connptr->server_fd, connptr->sbuffer,
				      &line)) < 0) {
				shutdown(connptr->server_fd, 2);
				httperr(connptr, 500, "Server Closed Early");
				return;
			} else if (retv == 0)
				return;

			connptr->serverheader = TRUE;

			if (strncasecmp(line, "HTTP/1.0", 8)) {
				/* Okay, we need to rewrite it then */
				if (!(ptr = strchr(line, ' '))) {
					shutdown(connptr->server_fd, 2);
					httperr(connptr, 500,
						"There was Server Error");
					return;
				}
				ptr++;

				if (!(newline = xmalloc(strlen(line) + 1))) {
					shutdown(connptr->server_fd, 2);
					httperr(connptr, 503,
						"No Memory Available");
					return;
				}

				sprintf(newline, "HTTP/1.0 %s", ptr);
				safefree(line);
				line = newline;
			}

			push_buffer(connptr->sbuffer, line, strlen(line));
		}
		connptr->actiontime = time(NULL);
	}

	if (write_to_client(connptr, writefds) < 0)
		return;

	if (FD_ISSET(connptr->server_fd, writefds)) {
		if (writeconn(connptr->server_fd, connptr->cbuffer) < 0) {
			shutdown(connptr->server_fd, 2);
			connptr->type = CLOSINGCONN;
			return;
		}
		connptr->actiontime = time(NULL);
	}
}

inline static void closingconn_req(struct conn_s *connptr, fd_set * writefds)
{
	assert(connptr);
	assert(writefds);

	write_to_client(connptr, writefds);
}

/*
 * Check against the valid subnet to see if we should allow the access
 */
static int validuser(int fd)
{
	char ipaddr[PEER_IP_LENGTH];

	assert(fd >= 0);

	if (config.subnet == NULL)
		return 1;

	if (!strncmp(config.subnet, getpeer_ip(fd, ipaddr),
		     strlen(config.subnet))) {
		return 1;
	} else {
		return 0;
	}
}

/*
 * Loop that checks for new connections, dispatches to the correct
 * routines if bytes are pending, checks to see if it's time for a
 * garbage collect.
 */
int getreqs(void)
{
	static unsigned int garbc = 0;
	fd_set readfds, writefds, exceptfds; /* chris - ADNS expects exceptfds */
	struct conn_s *connptr;
	int fd;
	struct timeval tv, now; /* chris - for ADNS timeouts */

	char peer_ipaddr[PEER_IP_LENGTH];

	if (setup_fd < 0) {
		log("ERROR getreqs: setup_fd not a socket");
		return -1;
	}

	/* Garbage collect the dead connections and close any idle ones */
	if (garbc++ >= GARBCOLL_INTERVAL) {
		garbcoll();
		garbc = 0;
	}
	conncoll();

	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_SET(setup_fd, &readfds);

	for (connptr = connections; connptr; connptr = connptr->next) {
#ifdef __DEBUG__
		log("Connptr: %p - %d / client %d server %d", connptr,
		    connptr->type, connptr->client_fd, connptr->server_fd);
#endif
		switch (connptr->type) {
		case NEWCONN:
			if (buffer_size(connptr->cbuffer) < MAXBUFFSIZE)
				FD_SET(connptr->client_fd, &readfds);
			else {
				httperr(connptr, 414,
					"Your Request is way too long.");
			}
			break;

		/* no case here for DNS_WAITCONN */

		case WAITCONN:
			FD_SET(connptr->server_fd, &readfds);
			FD_SET(connptr->server_fd, &writefds);

			if (buffer_size(connptr->cbuffer) < MAXBUFFSIZE)
				FD_SET(connptr->client_fd, &readfds);
			break;

		case RELAYCONN:
			if (buffer_size(connptr->sbuffer) > 0)
				FD_SET(connptr->client_fd, &writefds);
			if (buffer_size(connptr->sbuffer) < MAXBUFFSIZE)
				FD_SET(connptr->server_fd, &readfds);

			if (buffer_size(connptr->cbuffer) > 0)
				FD_SET(connptr->server_fd, &writefds);
			if (buffer_size(connptr->cbuffer) < MAXBUFFSIZE)
				FD_SET(connptr->client_fd, &readfds);

			break;

		case CLOSINGCONN:
			if (buffer_size(connptr->sbuffer) > 0)
				FD_SET(connptr->client_fd, &writefds);
			else {
				shutdown(connptr->client_fd, 2);
				shutdown(connptr->server_fd, 2);
				connptr->type = FINISHCONN;
			}
			break;

		default:
			break;
		}
	}

	/* Set a 60 second time out */
	tv.tv_sec = 1;//60;
	tv.tv_usec = 0;

	/* chris - Make ADNS do its stuff, too. */
	{
		struct timeval *tv_mod = &tv;
		int foo = FD_SETSIZE;
		gettimeofday(&now, NULL);
		FD_ZERO(&exceptfds);
		adns_beforeselect(adns, &foo, &readfds, &writefds, &exceptfds, &tv_mod, NULL, &now);
	}

	if (select(FD_SETSIZE, &readfds, &writefds, &exceptfds, &tv) < 0) {
#ifdef __DEBUG__
		log("select error: %s", strerror(errno));
#endif
		return 0;
	}

	/* chris - see whether any ADNS lookups have completed */
	gettimeofday(&now, NULL);
	adns_afterselect(adns, FD_SETSIZE, &readfds, &writefds, &exceptfds, &now);

	for (connptr = connections; connptr; connptr = connptr->next) {
		adns_answer *ans;

		if (connptr->type == DNS_WAITCONN &&
			adns_check(adns, &(connptr->adns_qu), &ans, (void**)&connptr) == 0) {

			if (ans->status == adns_s_ok) {
				if (connptr->domain) {
					insert(ans->rrs.inaddr, connptr->domain);
					free(connptr->domain);
				}

				clientreq_dnscomplete(connptr, ans->rrs.inaddr);
				free(ans);

				/* hack */
				FD_SET(connptr->server_fd, &readfds);
#ifdef __DEBUG__
				log("DNS resolution successful");
#endif
			} else {
				free(ans);

				httperr(connptr, 500, "Unable to resolve hostname in URL");
#ifdef __DEBUG__
				log("DNS resolution failed");
#endif
			}
		}
	}

	/* Check to see if there are new connections pending */
	if (FD_ISSET(setup_fd, &readfds) && (fd = listen_sock()) >= 0) {
		new_conn(fd);	/* make a connection from the FD */

		if (validuser(fd)) {
			if (config.cutoffload && (load > config.cutoffload)) {
				stats.num_refused++;
				httperr(connptr, 503,
					"tinyproxy is not accepting connections due to high system load");
			}
		} else {
			httperr(connptr, 403,
				"You are not authorized to use the service.");
			log("AUTH Rejected connection from %s",
			    getpeer_ip(fd, peer_ipaddr));
		}
	}

	/* 
	 * Loop through the connections and dispatch them to the appropriate
	 * handler
	 */
	for (connptr = connections; connptr; connptr = connptr->next) {
		switch (connptr->type) {
		case NEWCONN:
			newconn_req(connptr, &readfds);
			break;

		case WAITCONN:
			waitconn_req(connptr, &readfds, &writefds);
			break;

		case RELAYCONN:
			relayconn_req(connptr, &readfds, &writefds);
			break;

		case CLOSINGCONN:
			closingconn_req(connptr, &writefds);
			break;

		default:
			break;
		}
	}

	return 0;
}
