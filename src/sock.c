/* $Id: sock.c,v 1.25 2002-04-13 19:03:18 rjkaes Exp $
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
#include "sock.h"
#include "utils.h"

/*
 * The mutex is used for locking around any calls which access global
 * variables.
 *	- rjkaes
 */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

#define LOCK()   pthread_mutex_lock(&mutex);
#define UNLOCK() pthread_mutex_unlock(&mutex);

/*
 * The mutex is used for locking around accesses to gethostbyname()
 * function.
 */
static pthread_mutex_t gethostbyname_mutex = PTHREAD_MUTEX_INITIALIZER;
#define LOOKUP_LOCK()   pthread_mutex_lock(&gethostbyname_mutex);
#define LOOKUP_UNLOCK() pthread_mutex_unlock(&gethostbyname_mutex);

/*
 * Take a string host address and return a struct in_addr so we can connect
 * to the remote host.
 *
 * Return a negative if there is a problem.
 */
static int
lookup_domain(struct in_addr *addr, char *domain)
{
	struct hostent *resolv;

	if (!addr || !domain)
		return -1;

	/*
	 * First check to see if the domain is in dotted-decimal format.
	 */
	if (inet_aton(domain, (struct in_addr *)addr) != 0)
		return 0;

	/*
	 * Okay, it's an alpha-numeric domain, so look it up.
	 */
	LOOKUP_LOCK();
	if (!(resolv = gethostbyname(domain))) {
		LOOKUP_UNLOCK();
		return -1;
	}

	memcpy(addr, resolv->h_addr_list[0], resolv->h_length);

	LOOKUP_UNLOCK();

	return 0;
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

	/* Bind to our listening address*/
	if (config.ipAddr) {
		memset(&bind_addr, 0, sizeof(bind_addr));
		bind_addr.sin_family = AF_INET;
		bind_addr.sin_addr.s_addr = inet_addr(config.ipAddr);

		ret = bind(sock_fd, (struct sockaddr *)&bind_addr, sizeof(bind_addr));
		if (ret < 0) {
			log_message(LOG_ERR, "Could not bind local address \"%\" because of %s",
				    config.ipAddr,
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
listen_sock(uint16_t port, socklen_t * addrlen)
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
 * Takes a socket descriptor and returns the string contain the peer's
 * IP address.
 */
char *
getpeer_ip(int fd, char *ipaddr)
{
	struct sockaddr_in name;
	size_t namelen = sizeof(name);

	assert(fd >= 0);
	assert(ipaddr != NULL);

	/*
	 * Make sure the user's buffer is initialized to an empty string.
	 */
	*ipaddr = '\0';

	if (getpeername(fd, (struct sockaddr *) &name, &namelen) != 0) {
		log_message(LOG_ERR, "getpeer_ip: getpeername() error \"%s\".",
			    strerror(errno));
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
char *
getpeer_string(int fd, char *string)
{
	struct sockaddr_in name;
	size_t namelen = sizeof(name);
	struct hostent *peername;

	assert(fd >= 0);
	assert(string != NULL);

	/*
	 * Make sure the user's buffer is initialized to an empty string.
	 */
	*string = '\0';

	if (getpeername(fd, (struct sockaddr *) &name, &namelen) != 0) {
		log_message(LOG_ERR,
			    "getpeer_string: getpeername() error \"%s\".",
			    strerror(errno));
	} else {
		LOCK();
		peername = gethostbyaddr((char *) &name.sin_addr.s_addr,
					 sizeof(name.sin_addr.s_addr), AF_INET);
		if (peername)
			strlcpy(string, peername->h_name, PEER_STRING_LENGTH);
		else
			log_message(LOG_ERR,
				    "getpeer_string: gethostbyaddr() error \"%s\".",
				    hstrerror(h_errno));

		UNLOCK();
	}

	return string;
}

/*
 * Write the buffer to the socket. If an EINTR occurs, pick up and try
 * again. Keep sending until the buffer has been sent.
 */
ssize_t
safe_write(int fd, const char *buffer, size_t count)
{
	ssize_t len;
	size_t bytestosend;

	assert(fd >= 0);
	assert(buffer != NULL);
	assert(count > 0);

	bytestosend = count;

	while (1) {
		len = send(fd, buffer, bytestosend, MSG_NOSIGNAL);

		if (len < 0) {
			if (errno == EINTR)
				continue;
			else
				return -errno;
		}

		if (len == bytestosend)
			break;

		buffer += len;
		bytestosend -= len;
	}

	return count;
}

/*
 * Matched pair for safe_write(). If an EINTR occurs, pick up and try
 * again.
 */
ssize_t
safe_read(int fd, char *buffer, size_t count)
{
	ssize_t len;

	do {
		len = read(fd, buffer, count);
	} while (len < 0 && errno == EINTR);

	return len;
}

/*
 * Send a "message" to the file descriptor provided. This handles the
 * differences between the various implementations of vsnprintf. This code
 * was basically stolen from the snprintf() man page of Debian Linux
 * (although I did fix a memory leak. :)
 */
int
write_message(int fd, const char *fmt, ...)
{
	ssize_t n;
	size_t size = (1024 * 8);	/* start with 8 KB and go from there */
	char *buf, *tmpbuf;
	va_list ap;

	if ((buf = safemalloc(size)) == NULL)
		return -1;

	while (1) {
		va_start(ap, fmt);
		n = vsnprintf(buf, size, fmt, ap);
		va_end(ap);

		/* If that worked, break out so we can send the buffer */
		if (n > -1 && n < size)
			break;

		/* Else, try again with more space */
		if (n > -1)
			/* precisely what is needed (glibc2.1) */
			size = n + 1;
		else
			/* twice the old size (glibc2.0) */
			size *= 2;

		if ((tmpbuf = saferealloc(buf, size)) == NULL) {
			safefree(buf);
			return -1;
		} else
			buf = tmpbuf;
	}

	if (safe_write(fd, buf, n) < 0) {
		DEBUG2("Error in write_message(): %d", fd);

		safefree(buf);
		return -1;
	}

	safefree(buf);
	return 0;
}

/*
 * Read in a "line" from the socket. It might take a few loops through
 * the read sequence. The full string is allocate off the heap and stored
 * at the whole_buffer pointer. The caller needs to free the memory when
 * it is no longer in use. The returned line is NULL terminated.
 *
 * Returns the length of the buffer on success (not including the NULL
 * termination), 0 if the socket was closed, and -1 on all other errors.
 */
#define SEGMENT_LEN (512)
#define MAXIMUM_BUFFER_LENGTH (128 * 1024)
ssize_t
readline(int fd, char **whole_buffer)
{
	ssize_t whole_buffer_len;
	char buffer[SEGMENT_LEN];
	char *ptr;

	ssize_t ret;
	ssize_t diff;

	struct read_lines_s {
		char *data;
		size_t len;
		struct read_lines_s *next;
	};
	struct read_lines_s *first_line, *line_ptr;

	first_line = safecalloc(sizeof(struct read_lines_s), 1);
	if (!first_line)
		return -ENOMEMORY;

	line_ptr = first_line;

	whole_buffer_len = 0;
	for (;;) {
		ret = recv(fd, buffer, SEGMENT_LEN, MSG_PEEK);
		if (ret <= 0)
			goto CLEANUP;

		ptr = memchr(buffer, '\n', ret);
		if (ptr)
			diff = ptr - buffer + 1;
		else
			diff = ret;

		whole_buffer_len += diff;

		/*
		 * Don't allow the buffer to grow without bound. If we
		 * get to more than MAXIMUM_BUFFER_LENGTH close.
		 */
		if (whole_buffer_len > MAXIMUM_BUFFER_LENGTH) {
			ret = -EOUTRANGE;
			goto CLEANUP;
		}

		line_ptr->data = safemalloc(diff);
		if (!line_ptr->data) {
			ret = -ENOMEMORY;
			goto CLEANUP;
		}

		recv(fd, line_ptr->data, diff, 0);
		line_ptr->len = diff;

		if (ptr) {
			line_ptr->next = NULL;
			break;
		}

		line_ptr->next = safecalloc(sizeof(struct read_lines_s), 1);
		if (!line_ptr->next) {
			ret = -ENOMEMORY;
			goto CLEANUP;
		}
		line_ptr = line_ptr->next;
	}

	*whole_buffer = safemalloc(whole_buffer_len + 1);
	if (!*whole_buffer)
		return -ENOMEMORY;

	*(*whole_buffer + whole_buffer_len) = '\0';

	whole_buffer_len = 0;
	line_ptr = first_line;
	while (line_ptr) {
		memcpy(*whole_buffer + whole_buffer_len, line_ptr->data,
		       line_ptr->len);
		whole_buffer_len += line_ptr->len;

		line_ptr = line_ptr->next;
	}

	ret = whole_buffer_len;

      CLEANUP:
	do {
		line_ptr = first_line->next;
		safefree(first_line->data);
		safefree(first_line);
		first_line = line_ptr;
	} while (first_line);

	return ret;
}
