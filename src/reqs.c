/* $Id: reqs.c,v 1.36 2001-11-02 21:19:46 rjkaes Exp $
 *
 * This is where all the work in tinyproxy is actually done. Incoming
 * connections have a new thread created for them. The thread then
 * processes the headers from the client, the response from the server,
 * and then relays the bytes between the two.
 * If TUNNEL_SUPPORT is enabled, then tinyproxy will actually work
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
#include "conns.h"
#include "filter.h"
#include "log.h"
#include "regexp.h"
#include "reqs.h"
#include "sock.h"
#include "stats.h"
#include "utils.h"

#define HTTP400ERROR "Unrecognizable request. Only HTTP is allowed."
#define HTTP500ERROR "Unable to connect to remote server."
#define HTTP503ERROR "Internal server error."

#define LINE_LENGTH (MAXBUFFSIZE / 3)

/*
 * Remove any new lines or carriage returns from the end of a string.
 */
static inline void trim(char *string, unsigned int len)
{
	char *ptr;

	assert(string != NULL);
	assert(len > 0);

	ptr = string + len - 1;
	while (*ptr == '\r' || *ptr == '\n') {
		*ptr-- = '\0';

		/*
		 * Don't let the ptr back past the beginning of the
		 * string.
		 */
		if (ptr < string)
			return;
	}
}

/*
 * Read in the first line from the client (the request line for HTTP
 * connections. The request line is allocated from the heap, but it must
 * be freed in another function.
 */
static char *read_request_line(struct conn_s *connptr)
{
	char *request_buffer;
	size_t len;

	request_buffer = safemalloc(LINE_LENGTH);
	if (!request_buffer)
		return NULL;

	len = readline(connptr->client_fd, request_buffer, LINE_LENGTH);
	if (len <= 0) {
		log_message(LOG_ERR, "read_request_line: Client (file descriptor: %d) closed socket before read.", connptr->client_fd);
		safefree(request_buffer);
		return NULL;
	}

	/*
	 * Strip the new line and character return from the string.
	 */
	trim(request_buffer, len);

	log_message(LOG_CONN, "Request (file descriptor %d): %s",
		    connptr->client_fd, request_buffer);

	return request_buffer;
}

/*
 * This structure holds the information pulled from a URL request.
 */
struct request_s {
	char *method;
	char *protocol;

	char *host;
	char *path;
	int port;
};

static void free_request_struct(struct request_s *request)
{
	if (!request)
		return;

	safefree(request->method);
	safefree(request->protocol);

	safefree(request->host);
	safefree(request->path);

	safefree(request);
}

/*
 * Pull the information out of the URL line.
 */
static int extract_http_url(const char *url, struct request_s *request)
{
	request->host = safemalloc(strlen(url) + 1);
	request->path = safemalloc(strlen(url) + 1);

	if (!request->host || !request->path) {
		safefree(request->host);
		safefree(request->path);

		return -1;
	}

	if (sscanf(url, "http://%[^:/]:%d%s", request->host, &request->port, request->path) == 3)
		;
	else if (sscanf(url, "http://%[^/]%s", request->host, request->path) == 2)
		request->port = 80;
	else if (sscanf(url, "http://%[^:/]:%d", request->host, &request->port) == 2)
		strcpy(request->path, "/");
	else if (sscanf(url, "http://%[^/]", request->host) == 1) {
		request->port = 80;
		strcpy(request->path, "/");
	} else {
		log_message(LOG_ERR, "extract_http_url: Can't parse URL.");

		safefree(request->host);
		safefree(request->path);
		
		return -1;
	}

	return 0;
}

/*
 * Extract the URL from a SSL connection.
 */
static int extract_ssl_url(const char *url, struct request_s *request)
{
	request->host = safemalloc(strlen(url) + 1);
	if (!request->host)
		return -1;

	if (sscanf(url, "%[^:]:%d", request->host, &request->port) == 2)
		;
	else if (sscanf(url, "%s", request->host) == 1)
		request->port = 443;
	else {
		log_message(LOG_ERR, "extract_ssl_url: Can't parse URL.");

		safefree(request->host);
		return -1;
	}

	return 0;
}

/*
 * Create a connection for HTTP connections.
 */
static int establish_http_connection(struct conn_s *connptr,
				     struct request_s *request)
{
	/*
	 * Send the request line
	 */
	if (safe_write(connptr->server_fd, request->method, strlen(request->method)) < 0)
		return -1;
	if (safe_write(connptr->server_fd, " ", 1) < 0)
		return -1;
	if (safe_write(connptr->server_fd, request->path, strlen(request->path)) < 0)
		return -1;
	if (safe_write(connptr->server_fd, " ", 1) < 0)
		return -1;
	if (safe_write(connptr->server_fd, "HTTP/1.0\r\n", 10) < 0)
		return -1;

	/*
	 * Send headers
	 */
	if (safe_write(connptr->server_fd, "Host: ", 6) < 0)
		return -1;
	if (safe_write(connptr->server_fd, request->host, strlen(request->host)) < 0)
		return -1;

	if (safe_write(connptr->server_fd, "\r\n", 2) < 0)
		return -1;

	/*
	 * Send the Connection header since we don't support persistant
	 * connections.
	 */
	if (safe_write(connptr->server_fd, "Connection: close\r\n", 19) < 0)
		return -1;

	return 0;
}

/*
 * These two defines are for the SSL tunnelling.
 */
#define SSL_CONNECTION_RESPONSE "HTTP/1.0 200 Connection established\r\n"
#define PROXY_AGENT "Proxy-agent: " PACKAGE "/" VERSION "\r\n"

/*
 * Send the appropriate response to the client to establish a SSL
 * connection.
 */
static inline int send_ssl_response(struct conn_s *connptr)
{
	if (safe_write(connptr->client_fd, SSL_CONNECTION_RESPONSE, strlen(SSL_CONNECTION_RESPONSE)) < 0)
		return -1;

	if (safe_write(connptr->client_fd, PROXY_AGENT, strlen(PROXY_AGENT)) < 0)
		return -1;

	if (safe_write(connptr->client_fd, "\r\n", 2) < 0)
		return -1;

	return 0;
}

/*
 * Break the request line apart and figure out where to connect and
 * build a new request line. Finally connect to the remote server.
 */
static struct request_s *process_request(struct conn_s *connptr,
					 char *request_line)
{
	char *url;
	struct request_s *request;

	int ret;

	size_t request_len;

	/* NULL out all the fields so free's don't cause segfaults. */
	request = safecalloc(1, sizeof(struct request_s));
	if (!request)
		return NULL;

	request_len = strlen(request_line) + 1;

	request->method = safemalloc(request_len);
	url = safemalloc(request_len);
	request->protocol = safemalloc(request_len);

	if (!request->method || !url || !request->protocol) {
		safefree(url);
		free_request_struct(request);

		return NULL;
	}

	ret = sscanf(request_line, "%[^ ] %[^ ] %[^ ]", request->method, url, request->protocol);
	if (ret < 2) {
		log_message(LOG_ERR, "process_request: Bad Request on file descriptor %d", connptr->client_fd);
		httperr(connptr, 400, "Bad Request. No request found.");

		safefree(url);
		free_request_struct(request);

		return NULL;
	} else if (ret == 2) {
		connptr->simple_req = TRUE;
	}

	if (!url) {
		log_message(LOG_ERR, "process_request: Null URL on file descriptor %d", connptr->client_fd);
		httperr(connptr, 400, "Bad Request. Null URL.");

		safefree(url);
		free_request_struct(request);

		return NULL;
	}

	if (strncasecmp(url, "http://", 7) == 0) {
		/* Make sure the first four characters are lowercase */
		memcpy(url, "http", 4);

		if (extract_http_url(url, request) < 0) {
			httperr(connptr, 400, "Bad Request. Could not parse URL.");
			
			safefree(url);
			free_request_struct(request);

			return NULL;
		}
		connptr->ssl = FALSE;
	} else if (strcmp(request->method, "CONNECT") == 0) {
		if (extract_ssl_url(url, request) < 0) {
			httperr(connptr, 400, "Bad Request. Could not parse URL.");

			safefree(url);
			free_request_struct(request);

			return NULL;
		}
		connptr->ssl = TRUE;
	} else {
		log_message(LOG_ERR, "process_request: Unknown URL type on file descriptor %d", connptr->client_fd);
		httperr(connptr, 400, "Bad Request. Unknown URL type.");

		safefree(url);
		free_request_struct(request);

		return NULL;
	}

	safefree(url);

#ifdef FILTER_ENABLE
	/*
	 * Filter restricted domains
	 */
	if (config.filter) {
		if (filter_url(request->host)) {
			update_stats(STAT_DENIED);

			log_message(LOG_NOTICE, "Proxying refused on filtered domain \"%s\"", request->host);
			httperr(connptr, 404, "Connection to filtered domain is now allowed.");

			free_request_struct(request);

			return NULL;
		}
	}
#endif

	/*
	 * Check to see if they're requesting the stat host
	 */
	if (config.stathost && strcmp(config.stathost, request->host) == 0) {
		log_message(LOG_NOTICE, "Request for the stathost.");

		free_request_struct(request);

		showstats(connptr);
		return NULL;
	}

	return request;
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

	if ((ptr = strstr(line, ":")) == NULL)
		return -1;

	if ((buffer = safemalloc(ptr - line + 1)) == NULL)
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
	char *buffer;
	ssize_t len;

	buffer = safemalloc(MAXBUFFSIZE);
	if (!buffer)
		return -1;

	do {
		len = safe_read(connptr->client_fd, buffer, min(MAXBUFFSIZE, length));

		if (len <= 0) {
			safefree(buffer);
			return -1;
		}

		if (!connptr->send_message) {
			if (safe_write(connptr->server_fd, buffer, len) < 0) {
				safefree(buffer);
				return -1;
			}
		}

		length -= len;
	} while (length > 0);

	safefree(buffer);
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
	char *header;
	long content_length = -1;

	static char *skipheaders[] = {
		"proxy-connection",
		"host",
		"connection"
	};
	int i;

	header = safemalloc(LINE_LENGTH);
	if (!header)
		return -1;

	for ( ; ; ) {
		if (readline(connptr->client_fd, header, LINE_LENGTH) <= 0) {
			safefree(header);
			return -1;
		}

		if (header[0] == '\n'
		    || (header[0] == '\r' && header[1] == '\n')) {
			break;
		}

		if (connptr->send_message)
			continue;

		/*
		 * Don't send any of the headers if we're in SSL mode and
		 * NOT using an upstream proxy.
		 */
		if (connptr->ssl && !connptr->upstream)
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

		if (is_anonymous_enabled() && compare_header(header) < 0)
			continue;

		if (content_length == -1
		    && strncasecmp(header, "content-length", 14) == 0) {
			char *content_ptr = strchr(header, ':') + 1;
			content_length = atol(content_ptr);
		}

		if (safe_write(connptr->server_fd, header, strlen(header)) < 0) {
			safefree(header);
			return -1;
		}
	}

	if (!connptr->send_message && (connptr->upstream || !connptr->ssl)) {
#ifdef XTINYPROXY_ENABLE
		if (config.my_domain
		    && add_xtinyproxy_header(connptr) < 0) {
			safefree(header);
			return -1;
		}
#endif	/* XTINYPROXY */

		if (safe_write(connptr->server_fd, header, strlen(header)) < 0) {
			safefree(header);
			return -1;
		}
	}

	safefree(header);

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
	char *header;

	header = safemalloc(LINE_LENGTH);
	if (!header)
		return -1;

	for ( ; ; ) {
		if (readline(connptr->server_fd, header, LINE_LENGTH) <= 0) {
			safefree(header);
			return -1;
		}

		if (header[0] == '\n'
		    || (header[0] == '\r' && header[1] == '\n')) {
			break;
		}

		if (!connptr->simple_req
		    && safe_write(connptr->client_fd, header, strlen(header)) < 0) {
			safefree(header);
			return -1;
		}
	}
	
	if (!connptr->simple_req
	    && safe_write(connptr->client_fd, header, strlen(header)) < 0) {
		safefree(header);
		return -1;
	}

	safefree(header);
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
				log_message(LOG_INFO, "Idle Timeout (after select) as %g > %u.", tdiff, config.idletimeout);
				return;
			} else {
				continue;
			}
		} else if (ret < 0) {
			log_message(LOG_ERR, "relay_connection: select() error \"%s\". Closing connection (client_fd:%d, server_fd:%d)", strerror(errno), connptr->client_fd, connptr->server_fd);
			return;
		} else {
			/*
			 * Okay, something was actually selected so mark it.
			 */
			last_access = time(NULL);
		}
		
		if (FD_ISSET(connptr->server_fd, &rset)
		    && readbuff(connptr->server_fd, connptr->sbuffer) < 0) {
			break;
		}
		if (FD_ISSET(connptr->client_fd, &rset)
		    && readbuff(connptr->client_fd, connptr->cbuffer) < 0) {
			break;
		}
		if (FD_ISSET(connptr->server_fd, &wset)
		    && writebuff(connptr->server_fd, connptr->cbuffer) < 0) {
			break;
		}
		if (FD_ISSET(connptr->client_fd, &wset)
		    && writebuff(connptr->client_fd, connptr->sbuffer) < 0) {
			break;
		}
	}

	/*
	 * Here the server has closed the connection... write the
	 * remainder to the client and then exit.
	 */
	socket_blocking(connptr->client_fd);
	while (buffer_size(connptr->sbuffer) > 0) {
		if (writebuff(connptr->client_fd, connptr->sbuffer) < 0)
			break;
	}

	/*
	 * Try to send any remaining data to the server if we can.
	 */
	socket_blocking(connptr->server_fd);
	while (buffer_size(connptr->cbuffer) > 0) {
		if (writebuff(connptr->client_fd, connptr->cbuffer) < 0)
			break;
	}

	return;
}

#ifdef UPSTREAM_SUPPORT
/*
 * Establish a connection to the upstream proxy server.
 */
static int connect_to_upstream(struct conn_s *connptr,
			       struct request_s *request)
{
	char *combined_string;
	int len;

	connptr->server_fd = opensock(config.upstream_name, config.upstream_port);

	if (connptr->server_fd < 0) {
		log_message(LOG_WARNING, "Could not connect to upstream proxy.");
		httperr(connptr, 404, "Unable to connect to upstream proxy.");
		return -1;
	}

	log_message(LOG_CONN, "Established connection to upstream proxy \"%s\" using file descriptor %d.", config.upstream_name, connptr->server_fd);

	/*
	 * We need to re-write the "path" part of the request so that we
	 * can reuse the establish_http_connection() function. It expects a
	 * method and path.
	 */
	if (connptr->ssl) {
		len = strlen(request->host) + 6;

		combined_string = safemalloc(len + 1);
		if (!combined_string) {
			return -1;
		}

		snprintf(combined_string, len, "%s:%d", request->host, request->port);
	} else {
		len = strlen(request->host) + strlen(request->path) + 14;
		combined_string = safemalloc(len + 1);
		if (!combined_string) {
			return -1;
		}

		snprintf(combined_string, len, "http://%s:%d%s", request->host, request->port, request->path);
	}

	safefree(request->path);
	request->path = combined_string;
	
	return establish_http_connection(connptr, request);
}
#endif

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
	struct request_s *request = NULL;

	char peer_ipaddr[PEER_IP_LENGTH];
	char peer_string[PEER_STRING_LENGTH];

	char *request_line = NULL;

	log_message(LOG_CONN, "Connect (file descriptor %d): %s [%s]",
		    fd,
		    getpeer_string(fd, peer_string),
		    getpeer_ip(fd, peer_ipaddr));

	connptr = safemalloc(sizeof(struct conn_s));
	if (!connptr)
		return;

	initialize_conn(connptr);
	connptr->client_fd = fd;

	if (check_acl(fd) <= 0) {
		update_stats(STAT_DENIED);
		httperr(connptr, 403, "You do not have authorization for using this service.");
		goto send_error;
	}

#ifdef TUNNEL_SUPPORT
        /*
	 * If tunnel has been configured then redirect any connections to
	 * it. I know I used GOTOs, but it seems to me to be the best way
	 * of handling this situations. So sue me. :)
	 *	- rjkaes
	 */
	if (config.tunnel_name && config.tunnel_port != -1) {
		log_message(LOG_INFO, "Redirecting to %s:%d",
			    config.tunnel_name, config.tunnel_port);

		connptr->server_fd = opensock(config.tunnel_name, config.tunnel_port);
		
		if (connptr->server_fd < 0) {
			log_message(LOG_WARNING, "Could not connect to tunnel.");
			httperr(connptr, 404, "Unable to connect to tunnel.");

			goto internal_proxy;
		}

		log_message(LOG_INFO, "Established a connection to the tunnel \"%s\" using file descriptor %d.", config.tunnel_name, connptr->server_fd);

		/*
		 * I know GOTOs are evil, but duplicating the code is even
		 * more evil.
		 *	- rjkaes
		 */
		goto relay_proxy;
	}
#endif /* TUNNEL_SUPPORT */

internal_proxy:
	request_line = read_request_line(connptr);
	if (!request_line) {
		update_stats(STAT_BADCONN);
		destroy_conn(connptr);
		return;
	}

	request = process_request(connptr, request_line);
	safefree(request_line);

	if (!request) {
		if (!connptr->send_message) {
			update_stats(STAT_BADCONN);
			destroy_conn(connptr);
			return;
		}
		goto send_error;
	}

#ifdef UPSTREAM_SUPPORT
	if (config.upstream_name && config.upstream_port != -1) {
		connptr->upstream = TRUE;

		if (connect_to_upstream(connptr, request) < 0)
			goto send_error;
	} else {
#endif
		connptr->server_fd = opensock(request->host, request->port);
		if (connptr->server_fd < 0) {
			httperr(connptr, 500, HTTP500ERROR);
			goto send_error;
		}

		log_message(LOG_CONN, "Established connection to host \"%s\" using file descriptor %d.", request->host, connptr->server_fd);

		if (!connptr->ssl)
			establish_http_connection(connptr, request);
#ifdef UPSTREAM_SUPPORT
	}
#endif

send_error:
	free_request_struct(request);

	if (!connptr->simple_req) {
		if (process_client_headers(connptr) < 0) {
			update_stats(STAT_BADCONN);
			if (!connptr->send_message) {
				destroy_conn(connptr);
				return;
			}
		}
	}

	if (connptr->send_message) {
		destroy_conn(connptr);
		return;
	}

	if (!connptr->ssl || connptr->upstream) {
		if (process_server_headers(connptr) < 0) {
			update_stats(STAT_BADCONN);
			destroy_conn(connptr);
			return;
		}
	} else {
		if (send_ssl_response(connptr) < 0) {
			log_message(LOG_ERR, "handle_connection: Could not send SSL greeting to client.");
			update_stats(STAT_BADCONN);
			destroy_conn(connptr);
			return;
		}
	}

relay_proxy:
	relay_connection(connptr);

	/*
	 * All done... close everything and go home... :)
	 */
	destroy_conn(connptr);
	return;
}
