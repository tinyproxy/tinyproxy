/* $Id: reqs.c,v 1.15 2001-08-26 21:11:55 rjkaes Exp $
 *
 * This is where all the work in tinyproxy is actually done. Incoming
 * connections have a new thread created for them. The thread then
 * processes the headers from the client, the response from the server,
 * and then relays the bytes between the two.
 * If the UPSTEAM_PROXY is enabled, then tinyproxy will actually work
 * as a simple buffering TCP tunnel. Very cool! (Robert actually uses
 * this feature for a buffering NNTP tunnel.)
 *
 * Copyright (C) 1998	    Steven Young
 * Copyright (C) 1999,2000  Robert James Kaes (rjkaes@flarenet.com)
 * Copyright (C) 2000       Chris Lightfoot (chris@ex-parrot.com)
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

#include "acl.h"
#include "anonymous.h"
#include "buffer.h"
#include "filter.h"
#include "log.h"
#include "regexp.h"
#include "reqs.h"
#include "sock.h"
#include "stats.h"
#include "uri.h"
#include "utils.h"

#define HTTPPATTERN "^([a-z]+)[ \t]+([^ \t]+)([ \t]+(HTTP/[0-9]+\\.[0-9]+))?"
#define NMATCH 4
#define METHOD_IND   1
#define URI_IND      2
#define VERSION_MARK 3
#define VERSION_IND  4

#define HTTP400ERROR "Unrecognizable request. Only HTTP is allowed."
#define HTTP500ERROR "Unable to connect to remote server."
#define HTTP503ERROR "Internal server error."

#define LINE_LENGTH (MAXBUFFSIZE / 3)
#define HTTP_PORT 80

/*
 * Write the buffer to the socket. If an EINTR occurs, pick up and try
 * again.
 */
static ssize_t safe_write(int fd, void *buffer, size_t count)
{
	ssize_t len;

	do {
		len = write(fd, buffer, count);
	} while (len < 0 && errno == EINTR);

	return len;
}

/*
 * Matched pair for safe_write(). If an EINTR occurs, pick up and try
 * again.
 */
static ssize_t safe_read(int fd, void *buffer, size_t count)
{
	ssize_t len;

	do {
		len = read(fd, buffer, count);
	} while (len < 0 && errno == EINTR);

	return len;
}

/*
 * Parse a client HTTP request and then establish connection.
 */
static int process_method(struct conn_s *connptr)
{
	URI *uri = NULL;
	char inbuf[LINE_LENGTH];
	char *buffer = NULL, *request = NULL, *port = NULL;
	char *inbuf_ptr = NULL;

	regex_t preg;
	regmatch_t pmatch[NMATCH];

	size_t request_len;
	size_t len;
	int fd, port_no = HTTP_PORT;

	char peer_ipaddr[PEER_IP_LENGTH];

	getpeer_ip(connptr->client_fd, peer_ipaddr);

	len = readline(connptr->client_fd, inbuf, LINE_LENGTH);
	if (len <= 0) {
		log_message(LOG_ERR, "client closed before read");
		update_stats(STAT_BADCONN);
		return -2;
	}

	/*
	 * Strip the newline and character return from the string.
	 */
	inbuf_ptr = inbuf + len - 1;
	while (*inbuf_ptr == '\r' || *inbuf_ptr == '\n')
		*inbuf_ptr-- = '\0';

	log_message(LOG_CONN, "Request: %s", inbuf);

	if (regcomp(&preg, HTTPPATTERN, REG_EXTENDED | REG_ICASE) != 0) {
		log_message(LOG_ERR, "clientreq: regcomp");
		httperr(connptr, 503, HTTP503ERROR);
		update_stats(STAT_BADCONN);
		goto COMMON_EXIT;
	}
	if (regexec(&preg, inbuf, NMATCH, pmatch, 0) != 0) {
		log_message(LOG_ERR, "clientreq: regexec");
		regfree(&preg);
		httperr(connptr, 503, HTTP503ERROR);
		update_stats(STAT_BADCONN);
		goto COMMON_EXIT;
	}
	regfree(&preg);

	/*
	 * Test for a simple request, or a request from version 0.9
	 *	- rjkaes
	 */
	if (pmatch[VERSION_MARK].rm_so == -1
	    || !strncasecmp("http/0.9", inbuf + pmatch[VERSION_IND].rm_so, 8))
		connptr->simple_req = TRUE;
	

	if (pmatch[METHOD_IND].rm_so == -1 || pmatch[URI_IND].rm_so == -1) {
		log_message(LOG_ERR, "clientreq: Incomplete line from %s (%s)",
		    peer_ipaddr, inbuf);
		httperr(connptr, 400, HTTP400ERROR);
		update_stats(STAT_BADCONN);
		goto COMMON_EXIT;
	}

	len = pmatch[URI_IND].rm_eo - pmatch[URI_IND].rm_so;
	if (!(buffer = malloc(len + 1))) {
		log_message(LOG_ERR,
		    "clientreq: Cannot allocate buffer for request from %s",
		    peer_ipaddr);
		httperr(connptr, 503, HTTP503ERROR);
		update_stats(STAT_BADCONN);
		goto COMMON_EXIT;
	}
	memcpy(buffer, inbuf + pmatch[URI_IND].rm_so, len);
	buffer[len] = '\0';
	if (!(uri = explode_uri(buffer))) {
		safefree(buffer);
		log_message(LOG_ERR, "clientreq: Problem with explode_uri");
		httperr(connptr, 503, HTTP503ERROR);
		update_stats(STAT_BADCONN);
		goto COMMON_EXIT;
	}
	safefree(buffer);

	if (!uri->scheme || strcasecmp(uri->scheme, "http") != 0) {
		char *error_string;
		if (uri->scheme) {
			size_t error_string_len = strlen(uri->scheme) + 64;
			error_string = malloc(error_string_len);
			if (!error_string) {
				log_message(LOG_CRIT, "Out of Memory!");
				goto COMMON_EXIT;
			}
			snprintf(error_string, error_string_len,
				"Invalid scheme (%s). Only HTTP is allowed.",
				uri->scheme);
		} else {
			error_string =
				strdup("Invalid scheme (NULL). Only HTTP is allowed.");
			if (!error_string) {
				log_message(LOG_CRIT, "Out of Memory!");
				goto COMMON_EXIT;
			}
		}

		httperr(connptr, 400, error_string);
		safefree(error_string);
		update_stats(STAT_BADCONN);
		goto COMMON_EXIT;
	}

	if (!uri->authority) {
		httperr(connptr, 400, "Invalid authority.");
		update_stats(STAT_BADCONN);
		goto COMMON_EXIT;
	}

	if ((strlen(config.stathost) > 0) &&
	    strcasecmp(uri->authority, config.stathost) == 0) {
		showstats(connptr);
		goto COMMON_EXIT;
	}

	if ((port = strchr(uri->authority, ':'))) {
		*port++ = '\0';
		if (strlen(port) > 0)
			port_no = atoi(port);
	}

#ifdef FILTER_ENABLE
	/* Filter domains out */
	if (config.filter) {
		if (filter_url(uri->authority)) {
			log_message(LOG_ERR, "clientreq: Filtered connection (%s)",
			    peer_ipaddr);
			httperr(connptr, 404,
				"Unable to connect to filtered host.");
			update_stats(STAT_DENIED);
			goto COMMON_EXIT;
		}
	}
#endif /* FILTER_ENABLE */

	/* Build a new request from the first line of the header */
	request_len = strlen(inbuf) + 1;
	if (!(request = malloc(request_len))) {
		log_message(LOG_ERR,
		    "clientreq: cannot allocate buffer for request from %s",
		    peer_ipaddr);
		httperr(connptr, 503, HTTP503ERROR);
		update_stats(STAT_BADCONN);
		goto COMMON_EXIT;
	}

	memcpy(request, inbuf, pmatch[METHOD_IND].rm_eo);
	request[pmatch[METHOD_IND].rm_eo] = '\0';
	strlcat(request, " ", request_len);
	if (strlen(uri->path) > 0) {
		strlcat(request, uri->path, request_len);
		if (uri->query) {
			strlcat(request, "?", request_len);
			strlcat(request, uri->query, request_len);
		}
	} else {
		strlcat(request, "/", request_len);
	}
	strlcat(request, " HTTP/1.0\r\n", request_len);

	fd = opensock(uri->authority, port_no);
	if (fd < 0) {
		httperr(connptr, 500, HTTP500ERROR);
		update_stats(STAT_DENIED);
		goto COMMON_EXIT;
	}

	connptr->server_fd = fd;

	if (safe_write(connptr->server_fd, request, strlen(request)) < 0)
		goto COMMON_EXIT;
	
	/*
	 * Send the Host: header
	 */
	if (safe_write(connptr->server_fd, "Host: ", 6) < 0)
		goto COMMON_EXIT;
	if (safe_write(connptr->server_fd, uri->authority, strlen(uri->authority)) < 0)
		goto COMMON_EXIT;
	if (safe_write(connptr->server_fd, "\r\n", 2) < 0)
		goto COMMON_EXIT;

	/*
	 * Send the Connection header since we don't support persistant
	 * connections.
	 */
	if (safe_write(connptr->server_fd, "Connection: close\r\n", 19) < 0)
		goto COMMON_EXIT;

	safefree(request);
	free_uri(uri);
	return 0;

      COMMON_EXIT:
	safefree(request);
	free_uri(uri);
	return -1;
}

/*
 * Check to see if the line is allowed or not depending on the anonymous
 * headers which are to be allowed. If the header is found in the
 * anonymous list return 0, otherwise return -1.
 */
static int compare_header(char *line)
{
	char *buffer;
	char *ptr;
	int ret;

	if ((ptr = xstrstr(line, ":", strlen(line), FALSE)) == NULL)
		return -1;

	if ((buffer = malloc(ptr - line + 1)) == NULL)
		return -1;

	memcpy(buffer, line, (size_t)(ptr - line));
	buffer[ptr - line] = '\0';

	ret = anonymous_search(buffer);
	safefree(buffer);

	return ret;
}

/*
 * pull_client_data is used to pull across any client data (like in a
 * POST) which needs to be handled before an error can be reported, or
 * server headers can be processed.
 *	- rjkaes
 */
static int pull_client_data(struct conn_s *connptr, unsigned long int length)
{
	char buffer[MAXBUFFSIZE];
	ssize_t len;

	do {
		len = safe_read(connptr->client_fd, buffer, min(MAXBUFFSIZE, length));

		if (len <= 0) {
			return -1;
		}

		if (!connptr->output_message) {
			if (safe_write(connptr->server_fd, buffer, len) < 0) {
				return -1;
			}
		}

		length -= len;
	} while (length > 0);

	return 0;
}

#ifdef XTINYPROXY_ENABLE
/*
 * Add the X-Tinyproxy header to the collection of headers being sent to
 * the server.
 *	-rjkaes
 */
static int add_xtinyproxy_header(struct conn_s *connptr)
{
	char ipaddr[PEER_IP_LENGTH];
	char xtinyproxy[32];
	int length;

	length = snprintf(xtinyproxy, sizeof(xtinyproxy),
			  "X-Tinyproxy: %s\r\n",
			  getpeer_ip(connptr->client_fd, ipaddr));
	if (safe_write(connptr->server_fd, xtinyproxy, length) < 0)
		return -1;

	return 0;
}
#endif		/* XTINYPROXY */

/*
 * Here we loop through all the headers the client is sending. If we
 * are running in anonymous mode, we will _only_ send the headers listed
 * (plus a few which are required for various methods).
 *	- rjkaes
 */
static int process_client_headers(struct conn_s *connptr)
{
	char header[LINE_LENGTH];
	long content_length = -1;

	char *skipheaders[] = {
		"proxy-connection",
		"host",
		"connection"
	};
	int i;

	for ( ; ; ) {
		if (readline(connptr->client_fd, header, LINE_LENGTH) < 0) {
			return -1;
		}

		if (header[0] == '\n'
		    || (header[0] == '\r' && header[1] == '\n')) {
			break;
		}

		if (connptr->output_message)
			continue;

		if (is_anonymous_enabled() && compare_header(header) < 0)
			continue;

		/*
		 * Don't send certain headers.
		 */
		for (i = 0; i < (sizeof(skipheaders) / sizeof(char *)); i++) {
			if (strncasecmp(header, skipheaders[i], strlen(skipheaders[i])) == 0) {
				break;
			}
		}
		if (i != (sizeof(skipheaders) / sizeof(char *)))
			continue;

		if (content_length == -1
		    && strncasecmp(header, "content-length", 14) == 0) {
			char *content_ptr = strchr(header, ':') + 1;
			content_length = atol(content_ptr);
		}

		if (safe_write(connptr->server_fd, header, strlen(header)) < 0)
			return -1;
	}

	if (!connptr->output_message) {
#ifdef XTINYPROXY_ENABLE
		if (config.my_domain
		    && add_xtinyproxy_header(connptr) < 0) {
			return -1;
		}
#endif	/* XTINYPROXY */

		if (safe_write(connptr->server_fd, header, strlen(header)) < 0) {
			return -1;
		}
	}

	/*
	 * Spin here pulling the data from the client.
	 */
	if (content_length >= 0)
		return pull_client_data(connptr, (unsigned long int)content_length);
	else
		return 0;
}

/*
 * Loop through all the headers (including the response code) from the
 * server.
 */
static int process_server_headers(struct conn_s *connptr)
{
	char header[LINE_LENGTH];

	for ( ; ; ) {
		if (readline(connptr->server_fd, header, LINE_LENGTH) < 0) {
			return -1;
		}

		if (header[0] == '\n'
		    || (header[0] == '\r' && header[1] == '\n')) {
			break;
		}

		if (!connptr->simple_req
		    && safe_write(connptr->client_fd, header, strlen(header)) < 0) {
			return -1;
		}
	}
	
	if (!connptr->simple_req
	    && safe_write(connptr->client_fd, header, strlen(header)) < 0) {
		return -1;
	}
	return 0;
}

/*
 * Switch the sockets into nonblocking mode and begin relaying the bytes
 * between the two connections. We continue to use the buffering code
 * since we want to be able to buffer a certain amount for slower
 * connections (as this was the reason why I originally modified
 * tinyproxy oh so long ago...)
 *	- rjkaes
 */
static void relay_connection(struct conn_s *connptr)
{
	fd_set rset, wset;
	struct timeval tv;
	time_t last_access;
	int ret;
	double tdiff;
	int maxfd = max(connptr->client_fd, connptr->server_fd) + 1;

	socket_nonblocking(connptr->client_fd);
	socket_nonblocking(connptr->server_fd);

	last_access = time(NULL);

	for ( ; ; ) {
		FD_ZERO(&rset);
		FD_ZERO(&wset);

		tv.tv_sec = config.idletimeout - difftime(time(NULL), last_access);
		tv.tv_usec = 0;

		if (buffer_size(connptr->sbuffer) > 0)
			FD_SET(connptr->client_fd, &wset);
		if (buffer_size(connptr->cbuffer) > 0)
			FD_SET(connptr->server_fd, &wset);
		if (buffer_size(connptr->sbuffer) < MAXBUFFSIZE)
			FD_SET(connptr->server_fd, &rset);
		if (buffer_size(connptr->cbuffer) < MAXBUFFSIZE)
			FD_SET(connptr->client_fd, &rset);

		ret = select(maxfd, &rset, &wset, NULL, &tv);

		if (ret == 0) {
			tdiff = difftime(time(NULL), last_access);
			if (tdiff > config.idletimeout) {
				log_message(LOG_INFO, "Idle Timeout (after select) %g > %u", tdiff, config.idletimeout);
				return;
			} else {
				continue;
			}
		} else if (ret < 0) {
			return;
		} else {
			/*
			 * Okay, something was actually selected so mark it.
			 */
			last_access = time(NULL);
		}
		
		if (FD_ISSET(connptr->server_fd, &rset)
		    && readbuff(connptr->server_fd, connptr->sbuffer) < 0) {
			shutdown(connptr->server_fd, SHUT_WR);
			break;
		}
		if (FD_ISSET(connptr->client_fd, &rset)
		    && readbuff(connptr->client_fd, connptr->cbuffer) < 0) {
			return;
		}
		if (FD_ISSET(connptr->server_fd, &wset)
		    && writebuff(connptr->server_fd, connptr->cbuffer) < 0) {
			shutdown(connptr->server_fd, SHUT_WR);
			break;
		}
		if (FD_ISSET(connptr->client_fd, &wset)
		    && writebuff(connptr->client_fd, connptr->sbuffer) < 0) {
			return;
		}
	}

	/*
	 * Here the server has closed the connection... write the
	 * remainder to the client and then exit.
	 */
	socket_blocking(connptr->client_fd);
	while (buffer_size(connptr->sbuffer) > 0) {
		if (writebuff(connptr->client_fd, connptr->sbuffer) < 0)
			return;
	}

	return;
}

static void initialize_conn(struct conn_s *connptr)
{
	connptr->client_fd = connptr->server_fd = -1;
	connptr->cbuffer = new_buffer();
	connptr->sbuffer = new_buffer();

	connptr->output_message = NULL;
	connptr->simple_req = FALSE;

	update_stats(STAT_OPEN);
}

static void destroy_conn(struct conn_s *connptr)
{
	if (connptr->client_fd != -1)
		close(connptr->client_fd);
	if (connptr->server_fd != -1)
		close(connptr->server_fd);

	if (connptr->cbuffer)
		delete_buffer(connptr->cbuffer);
	if (connptr->sbuffer)
		delete_buffer(connptr->sbuffer);

	safefree(connptr->output_message);
	safefree(connptr);

	update_stats(STAT_CLOSE);
}

/*
 * This is the main drive for each connection. As you can tell, for the
 * first few steps we are using a blocking socket. If you remember the
 * older tinyproxy code, this use to be a very confusing state machine.
 * Well, no more! :) The sockets are only switched into nonblocking mode
 * when we start the relay portion. This makes most of the original
 * tinyproxy code, which was confusing, redundant. Hail progress.
 * 	- rjkaes
 */
void handle_connection(int fd)
{
	struct conn_s *connptr;
	char peer_ipaddr[PEER_IP_LENGTH];
	char peer_string[PEER_STRING_LENGTH];

	log_message(LOG_CONN, "Connect: %s [%s]",
		    getpeer_string(fd, peer_string),
		    getpeer_ip(fd, peer_ipaddr));

	connptr = malloc(sizeof(struct conn_s));
	if (!connptr) {
		log_message(LOG_CRIT, "Out of memory!");
		return;
	}

	initialize_conn(connptr);
	connptr->client_fd = fd;

	if (check_acl(fd) <= 0) {
		update_stats(STAT_DENIED);
		httperr(connptr, 403, "You do not have authorization for using this service.");
		goto send_error;
	}

#ifdef TUNNEL_SUPPORT
        /*
	 * If an upstream proxy has been configured then redirect any
	 * connections to it. If we cannot connect to the upstream, see if
	 * we can handle it ourselves. I know I used GOTOs, but it seems to
	 * me to be the best way of handling this situations. So sue me. :)
	 *	- rjkaes
	 */
	if (config.tunnel_name && config.tunnel_port != -1) {
		log_message(LOG_INFO, "Redirecting to %s:%d",
		    config.tunnel_name, config.tunnel_port);

		connptr->server_fd = opensock(config.tunnel_name, config.tunnel_port);
		if (connptr->server_fd < 0) {
			log_message(LOG_ERR, "Could not connect to tunnel's end, see if we can handle it ourselves.");
			goto internal_proxy;
		}

		/*
		 * I know GOTOs are evil, but duplicating the code is even
		 * more evil.
		 *	- rjkaes
		 */
		goto relay_proxy;
	}
#endif /* TUNNEL_SUPPORT */

internal_proxy:
	if (process_method(connptr) < -1) {
		destroy_conn(connptr);
		return;
	}

send_error:
	if (!connptr->simple_req) {
		if (process_client_headers(connptr) < 0) {
			update_stats(STAT_BADCONN);
			destroy_conn(connptr);
			return;
		}
	}

	if (connptr->output_message) {
		safe_write(connptr->client_fd, connptr->output_message,
			   strlen(connptr->output_message));
		
		destroy_conn(connptr);
		return;
	}

	if (process_server_headers(connptr) < 0) {
		update_stats(STAT_BADCONN);
		destroy_conn(connptr);
		return;
	}

relay_proxy:
	relay_connection(connptr);

	/*
	 * All done... close everything and go home... :)
	 */
	destroy_conn(connptr);
	return;
}
