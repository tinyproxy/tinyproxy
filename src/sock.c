/* $Id: sock.c,v 1.2 2000-03-31 20:10:13 rjkaes Exp $
 *
 * Sockets are created and destroyed here. When a new connection comes in from
 * a client, we need to copy the socket and the create a second socket to the
 * remote server the client is trying to connect to. Also, the listening
 * socket is created and destroyed here. Sounds more impressive than it
 * actually is.
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
#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <assert.h>

#include "tinyproxy.h"
#include "sock.h"
#include "log.h"
#include "utils.h"
#include "dnscache.h"

/* This routine is so old I can't even remember writing it.  But I do
 * remember that it was an .h file because I didn't know putting code in a
 * header was bad magic yet.  anyway, this routine opens a connection to a
 * system and returns the fd.
 */

/*
 * Cleaned up some of the code to use memory routines which are now the
 * default. Also, the routine first checks to see if the address is in
 * dotted-decimal form before it does a name lookup. Finally, the newly
 * created socket is made non-blocking.
 *      - rjkaes
 */
int opensock(char *ip_addr, int port)
{
	int sock_fd, flags;
	struct sockaddr_in port_info;

	assert(ip_addr);
	assert(port > 0);

	memset((struct sockaddr *) &port_info, 0, sizeof(port_info));

	port_info.sin_family = AF_INET;

	/* chris - Could block; neet to ensure that this is never called
	 * before a non-blocking DNS query happens for this address. Not
	 * relevant in the code as it stands.
	 */
	if (dnscache(&port_info.sin_addr, ip_addr) < 0) {
		log("ERROR opensock: Could not lookup address: %s", ip_addr);
		return -1;
	}

	port_info.sin_port = htons(port);

	if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		log("ERROR opensock: socket (%s)", strerror(errno));
		return -1;
	}

	flags = fcntl(sock_fd, F_GETFL, 0);
	fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);

	if (connect
	    (sock_fd, (struct sockaddr *) &port_info, sizeof(port_info)) < 0) {
		if (errno != EINPROGRESS) {
			log("ERROR opensock: connect (%s)", strerror(errno));
			return -1;
		}
	}

	return sock_fd;
}

/* chris - added this to open a socket given a struct in_addr */
int opensock_inaddr(struct in_addr *inaddr, int port)
{
	int sock_fd, flags;
	struct sockaddr_in port_info;

	assert(inaddr);
	assert(port > 0);

	memset((struct sockaddr *) &port_info, 0, sizeof(port_info));

	port_info.sin_family = AF_INET;

	memcpy(&port_info.sin_addr, inaddr, sizeof(struct in_addr));

	port_info.sin_port = htons(port);

	if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		log("ERROR opensock_inaddr: socket (%s)", strerror(errno));
		return -1;
	}

	flags = fcntl(sock_fd, F_GETFL, 0);
	fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);

	if (connect
	    (sock_fd, (struct sockaddr *) &port_info, sizeof(port_info)) < 0) {
		if (errno != EINPROGRESS) {
			log("ERROR opensock: connect (%s)", strerror(errno));
			return -1;
		}
	}

	return sock_fd;
}

int setup_fd;
static struct sockaddr listen_sock_addr;

/*
 * Start listening to a socket.
 */
int init_listen_sock(int port)
{
	struct sockaddr_in laddr;
	int i = 1;

	assert(port > 0);

	if ((setup_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		log("ERROR init_listen_sock: socket (%s)", strerror(errno));
		return -1;
	}

	if (setsockopt(setup_fd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i)) < 0) {
		log("ERROR init_listen_sock: setsockopt (%s)",
		    strerror(errno));
		return -1;
	}

	memset(&listen_sock_addr, 0, sizeof(listen_sock_addr));
	memset(&laddr, 0, sizeof(laddr));
	laddr.sin_family = AF_INET;
	laddr.sin_port = htons(port);

	if (config.ipAddr) {
		laddr.sin_addr.s_addr = inet_addr(config.ipAddr);
	} else {
		laddr.sin_addr.s_addr = inet_addr("0.0.0.0");
	}
	if (bind(setup_fd, (struct sockaddr *) &laddr, sizeof(laddr)) < 0) {
		log("ERROR init_listen_sock: bind (%s)", strerror(errno));
		return -1;
	}
	if ((listen(setup_fd, MAXLISTEN)) != 0) {
		log("ERROR init_listen_sock: listen (%s)", strerror(errno));
		return -1;
	}

	return 0;
}

/*
 * Grab a pending connection
 */
int listen_sock(void)
{
	static int sock;
	int sz = sizeof(listen_sock_addr);

	if ((sock = accept(setup_fd, &listen_sock_addr, &sz)) < 0) {
		if (errno != ECONNABORTED
#ifdef EPROTO
		    && errno != EPROTO
#endif
#ifdef EWOULDBLOCK
		    && errno != EWOULDBLOCK
#endif
		    && errno != EINTR)
			log("ERROR listen_sock: accept (%s)", strerror(errno));
		return -1;
	}
	stats.num_listens++;

	return sock;
}

/*
 * Stop listening on a socket.
 */
void de_init_listen_sock(void)
{
	close(setup_fd);
}

/*
 * Takes a socket descriptor and returns the string contain the peer's
 * IP address.
 */
char *getpeer_ip(int fd, char *ipaddr)
{
	struct sockaddr_in name;
	int namelen = sizeof(name);

	assert(fd >= 0);
	assert(ipaddr);

	memset(ipaddr, '\0', PEER_IP_LENGTH);

	if (getpeername(fd, (struct sockaddr *) &name, &namelen) != 0) {
		log("ERROR Connect: 'could not get peer name'");
	} else {
		strncpy(ipaddr,
			inet_ntoa(*(struct in_addr *) &name.sin_addr.s_addr),
			PEER_IP_LENGTH - 1);
	}

	return ipaddr;
}

/*
 * Takes a socket descriptor and returns the string containing the peer's
 * address.
 */
char *getpeer_string(int fd, char *string)
{
	struct sockaddr_in name;
	int namelen = sizeof(name);
	struct hostent *peername;

	assert(fd >= 0);
	assert(string);

	memset(string, '\0', PEER_STRING_LENGTH);

	if (getpeername(fd, (struct sockaddr *) &name, &namelen) != 0) {
		log("ERROR Connect: 'could not get peer name'");
	} else {
		if (
		    (peername =
		     gethostbyaddr((char *) &name.sin_addr.s_addr,
				   sizeof(name.sin_addr.s_addr),
				   AF_INET)) != NULL) {
			strncpy(string, peername->h_name,
				PEER_STRING_LENGTH - 1);
		}
	}

	return string;
}


/*
 * Okay, this is a wacked out function. The basic gist is that we read in one
 * line from the socket and return it in "line". However, if we can't pull in
 * one complete line (up to an including the '\n') then we need to store it in
 * the buffer's "working_string". Fun. :)
 *	-- rjkaes
 */
int readline(int fd, struct buffer_s *buffer, char **line)
{
	char inbuf[BUFFER];
	int bytesin;
	char *endline = NULL;
	char *newline;
	unsigned long length;

	assert(fd >= 0);
	assert(buffer);
	assert(line);

	*line = NULL;

	/* Inspect the queue. */
	if ((bytesin = recv(fd, inbuf, BUFFER - 1, MSG_PEEK)) <= 0) {
		goto CONN_ERROR;
	}

	if (buffer->working_length == 0) {
		/* There is no working line, so read in a line of text. */

		/* Okay, check to see if there is a '\n' in this. */
		endline = xstrstr(inbuf, "\n", bytesin, FALSE);
		
		if (endline) {
			/* Yes, we have a complete line. */
			*(++endline) = '\0';
			length = strlen(inbuf);
			
			/* Actually pull the data off the queue */
			if ((bytesin = recv(fd, inbuf, length, 0) <= 0)) {
				goto CONN_ERROR;
			}

			*line = xstrdup(inbuf);
			return strlen(*line);
		}

		/*
		 * Well, we don't have a complete line, so add it to the
		 * working_string.
		 */
		if (!(buffer->working_string = xmalloc(bytesin))) {
			return -1;
		}

		if ((bytesin = recv(fd, inbuf, bytesin, 0)) <= 0) {
			safefree(buffer->working_string);
			goto CONN_ERROR;
		}

		memcpy(buffer->working_string, inbuf, bytesin);
		buffer->working_length = bytesin;

		return 0;
	}

	/* 
	 * Alright, we do have a working line, so read in more data and see
	 * if there is a '\n' in it.
	 */
	endline = xstrstr(inbuf, "\n", bytesin, FALSE);

	if (endline) {
		/* 
		 * Great, there was a "\n" found, so combine with
		 * working_string.
		 */
		*(++endline) = '\0';
		length = strlen(inbuf);

		if (!(*line = xmalloc(bytesin + buffer->working_length + 1))) {
			return -1;
		}

		/* Pull the data off */
		if ((bytesin = recv(fd, inbuf, bytesin, 0)) <= 0) {
			goto CONN_ERROR;
		}

		/* Copy all the data into a new line */
		memcpy(*line, buffer->working_string, buffer->working_length);
		memcpy(*line + buffer->working_length, inbuf, bytesin);

		*(*line + buffer->working_length + bytesin + 1) = '\0';
		
		safefree(buffer->working_string);
		buffer->working_length = 0;

		return strlen(*line);
	}

	/*
	 * Well, we have a working line and still don't have a complete line.
	 * Add the new data to the working line and return.
	 */
	if (!(newline = xmalloc(buffer->working_length + bytesin))) {
		return -1;
	}

	if ((bytesin = recv(fd, inbuf, bytesin, 0)) <= 0) {
		goto CONN_ERROR;
	}
	
	memcpy(newline, buffer->working_string, buffer->working_length);
	memcpy(newline + buffer->working_length, inbuf, bytesin);

	safefree(buffer->working_string);
	buffer->working_string = newline;
	buffer->working_length += bytesin;

	return 0;

	/* Handle all the errors a socket could produce. */
      CONN_ERROR:
	if (bytesin == 0 || (errno != EAGAIN && errno != EINTR)) {
		return -1;
	}
	return 0;
}
