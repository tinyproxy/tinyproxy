/* $Id: sock.c,v 1.39 2002-10-03 20:50:59 rjkaes Exp $
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

#include "tinyproxy.h"

#include "log.h"
#include "heap.h"
#include "sock.h"
#include "text.h"

/*
 * Take a string host address and return a struct in_addr so we can connect
 * to the remote host.
 *
 * Return a negative if there is a problem.
 */
static int
lookup_domain(struct in_addr *addr, const char *domain)
{	
	struct hostent* result;

	assert(domain != NULL);

	result = gethostbyname(domain);
	if (result) {
		memcpy(addr, result->h_addr_list[0], result->h_length);
		return 0;
	} else
		return -1;
}

/* This routine is so old I can't even remember writing it.  But I do
 * remember that it was an .h file because I didn't know putting code in a
 * header was bad magic yet.  anyway, this routine opens a connection to a
 * system and returns the fd.
 *	- steve
 *
 * Cleaned up some of the code to use memory routines which are now the
 * default. Also, the routine first checks to see if the address is in
 * dotted-decimal form before it does a name lookup.
 *      - rjkaes
 */
int
opensock(char *ip_addr, uint16_t port)
{
	int sock_fd;
	struct sockaddr_in port_info;
	struct sockaddr_in bind_addr;
	int ret;

	assert(ip_addr != NULL);
	assert(port > 0);

	memset((struct sockaddr *) &port_info, 0, sizeof(port_info));

	port_info.sin_family = AF_INET;

	/* Lookup and return the address if possible */
	ret = lookup_domain(&port_info.sin_addr, ip_addr);

	if (ret < 0) {
		log_message(LOG_ERR,
			    "opensock: Could not lookup address \"%s\".",
			    ip_addr);
		return -1;
	}

	port_info.sin_port = htons(port);

	if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		log_message(LOG_ERR, "opensock: socket() error \"%s\".",
			    strerror(errno));
		return -1;
	}

	/* Bind to the specified address */
	if (config.bind_address) {
		memset(&bind_addr, 0, sizeof(bind_addr));
		bind_addr.sin_family = AF_INET;
		bind_addr.sin_addr.s_addr = inet_addr(config.bind_address);

		ret = bind(sock_fd, (struct sockaddr *)&bind_addr, sizeof(bind_addr));
		if (ret < 0) {
			log_message(LOG_ERR, "Could not bind local address \"%\" because of %s",
				    config.bind_address,
				    strerror(errno));
			return -1;
		}
	}

	if (connect(sock_fd, (struct sockaddr *) &port_info, sizeof(port_info)) < 0) {
		log_message(LOG_ERR, "opensock: connect() error \"%s\".",
			    strerror(errno));
		return -1;
	}

	return sock_fd;
}

/*
 * Set the socket to non blocking -rjkaes
 */
int
socket_nonblocking(int sock)
{
	int flags;

	assert(sock >= 0);

	flags = fcntl(sock, F_GETFL, 0);
	return fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

/*
 * Set the socket to blocking -rjkaes
 */
int
socket_blocking(int sock)
{
	int flags;

	assert(sock >= 0);

	flags = fcntl(sock, F_GETFL, 0);
	return fcntl(sock, F_SETFL, flags & ~O_NONBLOCK);
}

/*
 * Start listening to a socket. Create a socket with the selected port.
 * The size of the socket address will be returned to the caller through
 * the pointer, while the socket is returned as a default return.
 *	- rjkaes
 */
int
listen_sock(uint16_t port, socklen_t* addrlen)
{
	int listenfd;
	const int on = 1;
	struct sockaddr_in addr;

	assert(port > 0);
	assert(addrlen != NULL);

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	if (config.ipAddr) {
		addr.sin_addr.s_addr = inet_addr(config.ipAddr);
	} else {
		addr.sin_addr.s_addr = inet_addr("0.0.0.0");
	}

	if (bind(listenfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		log_message(LOG_ERR, "Unable to bind listening socket because of %s",
			    strerror(errno));
		return -1;
	}

	if (listen(listenfd, MAXLISTEN) < 0) {
		log_message(LOG_ERR, "Unable to start listening socket because of %s",
			    strerror(errno));
		return -1;
	}

	*addrlen = sizeof(addr);

	return listenfd;
}

/*
 * Return the peer's socket information.
 */
int
getpeer_information(int fd, char* ipaddr, char* string_addr)
{
	struct sockaddr_in name;
	socklen_t namelen = sizeof(name);
	struct hostent* result;

	assert(fd >= 0);
	assert(ipaddr != NULL);
	assert(string_addr != NULL);

	/*
	 * Clear the memory.
	 */
	memset(ipaddr, '\0', PEER_IP_LENGTH);
	memset(string_addr, '\0', PEER_STRING_LENGTH);

	if (getpeername(fd, (struct sockaddr *)&name, &namelen) != 0) {
		log_message(LOG_ERR, "getpeer_information: getpeername() error: %s",
			    strerror(errno));
		return -1;
	} else {
		strlcpy(ipaddr,
			inet_ntoa(*(struct in_addr *)&name.sin_addr.s_addr),
			PEER_IP_LENGTH);
	}

	result = gethostbyaddr((char *)&name.sin_addr.s_addr, 4, AF_INET);
	if (result) {
		strlcpy(string_addr, result->h_name, PEER_STRING_LENGTH);
		return 0;
	} else {
		return -1;
	}
}
