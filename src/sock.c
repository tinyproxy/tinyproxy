/* $Id: sock.c,v 1.3 2000-09-11 23:56:32 rjkaes Exp $
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

#include "dnscache.h"
#include "log.h"
#include "sock.h"
#include "utils.h"

#define SA struct sockaddr

/*
 * The mutex is used for locking around the calls to the dnscache since I
 * don't want multiple threads accessing the linked list at the same time.
 * This should be more fine grained, but it will do for now.
 *	- rjkaes
 */
pthread_mutex_t sock_mutex = PTHREAD_MUTEX_INITIALIZER;

#define SOCK_LOCK()   pthread_mutex_lock(&sock_mutex);
#define SOCK_UNLOCK() pthread_mutex_unlock(&sock_mutex);


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
int opensock(char *ip_addr, int port)
{
	int sock_fd;
	struct sockaddr_in port_info;
	int ret;

	memset((SA *) &port_info, 0, sizeof(port_info));

	port_info.sin_family = AF_INET;

	/* chris - Could block; neet to ensure that this is never called
	 * before a non-blocking DNS query happens for this address. Not
	 * relevant in the code as it stands.
	 */
	SOCK_LOCK();
	ret = dnscache(&port_info.sin_addr, ip_addr);
	SOCK_UNLOCK();

	if (ret < 0) {
		log(LOG_ERR, "opensock: Could not lookup address: %s", ip_addr);
		return -1;
	}

	port_info.sin_port = htons(port);

	if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		log(LOG_ERR, "opensock: socket (%s)", strerror(errno));
		return -1;
	}

	if (connect(sock_fd, (SA *) &port_info, sizeof(port_info)) < 0) {
		log(LOG_ERR, "connecting socket");
		return -1;
	}

	return sock_fd;
}

/*
 * Set the socket to non blocking -rjkaes
 */
int socket_nonblocking(int sock)
{
	int flags;

	flags = fcntl(sock, F_GETFL, 0);
	return fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

/*
 * Set the socket to blocking -rjkaes
 */
int socket_blocking(int sock)
{
	int flags;

	flags = fcntl(sock, F_GETFL, 0);
	return fcntl(sock, F_SETFL, flags & ~O_NONBLOCK);
}

/*
 * Start listening to a socket. Create a socket with the selected port.
 * The size of the socket address will be returned to the caller through
 * the pointer, while the socket is returned as a default return.
 *	- rjkaes
 */
int listen_sock(unsigned int port, socklen_t *addrlen)
{
	int listenfd;
	const int on = 1;
	struct sockaddr_in addr;

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
	
	bind(listenfd, (struct sockaddr *) &addr, sizeof(addr));

	listen(listenfd, MAXLISTEN);

	*addrlen = sizeof(addr);

	return listenfd;
}

/*
 * Takes a socket descriptor and returns the string contain the peer's
 * IP address.
 */
char *getpeer_ip(int fd, char *ipaddr)
{
	struct sockaddr_in name;
	int namelen = sizeof(name);

	if (getpeername(fd, (struct sockaddr *) &name, &namelen) != 0) {
		log(LOG_ERR, "Connect: 'could not get peer name'");
	} else {
		strlcpy(ipaddr,
			inet_ntoa(*(struct in_addr *) &name.sin_addr.s_addr),
			PEER_IP_LENGTH);
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

	if (getpeername(fd, (struct sockaddr *) &name, &namelen) != 0) {
		log(LOG_ERR, "Connect: 'could not get peer name'");
	} else {
		SOCK_LOCK();
		peername = gethostbyaddr((char *)&name.sin_addr.s_addr,
					 sizeof(name.sin_addr.s_addr),
					 AF_INET);
		if (peername) {
			strlcpy(string, peername->h_name, PEER_STRING_LENGTH);
		}
		SOCK_UNLOCK();
	}

	return string;
}

ssize_t readline(int fd, void *vptr, size_t maxlen)
{
	ssize_t n, rc;
	char c, *ptr;

	ptr = vptr;
	for (n = 1; n < maxlen; n++) {
	again:
		if ((rc = read(fd, &c, 1)) == 1) {
			*ptr++ = c;
			if (c == '\n')
				break;
		} else if (rc == 0) {
			if (n == 1)
				return 0;
			else
				break;
		} else {
			if (errno == EINTR)
				goto again;
			return -1;
		}
	}

	*ptr = 0;
	return n;
}
