/* $Id: dnsclient.c,v 1.1 2002-05-23 04:40:06 rjkaes Exp $
 *
 * Create the dnsserver child process, and then include functions to
 * retrieve IP addresses or host names.  These functions are required
 * since the traditional gethostbyaddr() and getaddrbyname() functions
 * are not thread reentrant.  By spawning a child process the blocking
 * has been moved outside the threads.
 *
 * Copyright (C) 2002  Robert James Kaes (rjkaes@flarenet.com)
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

#include "dnsclient.h"
#include "heap.h"
#include "log.h"
#include "network.h"
#include "text.h"

/*
 * Static variable which holds the "dnsserver" process.
 */
static pid_t dnsserver_process;

/*
 * Hold the location of the Unix socket for client communication.
 */
static char* unix_socket_loc;

/*
 * Fork a copy of the "dnsserver" program.  Two arguments are used
 * by this function: 'dnsserver_location' is the complete path to the
 * dnsserver execuatable; 'path' is the absolute path used by the
 * Unix socket.
 */
int
start_dnsserver(const char* dnsserver_location, const char* path)
{
	int ret;

	assert(dnsserver_location != NULL);
	assert(path != NULL);

	unix_socket_loc = safestrdup(path);

	if ((dnsserver_process = fork()) == 0) {
		/* Child process */
		chdir("/");
		umask(077);

		close(0);
		close(1);
		close(2);

		/* Start the "dnsserver" program */
		ret = execl(dnsserver_location,
			    dnsserver_location,
			    path,
			    NULL);
		
		if (ret < 0)
			return -ENOEXEC;
	}

	/* Parent process */
	return 0;
}

/*
 * Stop the dnsserver process.
 */
int
stop_dnsserver(void)
{
	kill(dnsserver_process, SIGTERM);
	kill(dnsserver_process, SIGKILL);
	unlink(unix_socket_loc);

	return 0;
}

/*
 * Returns a socket connected to the "dnsserver"
 */
int
dns_connect(void)
{
	int dns;
	struct sockaddr_un addr;

	dns = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (dns < 0)
		return -EIO;

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_LOCAL;
	strcpy(addr.sun_path, unix_socket_loc);

	if (connect(dns, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		return -EIO;
	else
		return dns;
}

/*
 * Closes the connection to the "dnsserver"
 */
void
dns_disconnect(int dns)
{
	assert(dns >= 0);

	safe_write(dns, "$shutdown\n", 10);
	close(dns);
}

/*
 * Takes a dotted-decimal IP address and returns the host name in the
 * buffer supplied by the caller.
 *
 * Returns:  0 if the buffer is filled correctly, or
 *          -1 if there was an error
 */
int
dns_gethostbyaddr(int dns, const char* ipaddr, char** hostname,
		  size_t hostlen)
{
	char* buffer;
	ssize_t len;
	char *ptr;

	assert(dns >= 0);
	assert(ipaddr != NULL);
	assert(hostname != NULL);
	assert(hostlen > 0);

	write_message(dns, "%s\n", ipaddr);
	len = readline(dns, &buffer);
	if (len < 0)
		return len;
	chomp(buffer, len);

	if (strncmp(buffer, "$fail", 5) == 0) {
		/* There was a problem. */
		safefree(buffer);
		return -ENOMSG;
	}

	ptr = strchr(buffer, ' ');
	ptr++;

	strlcpy(*hostname, ptr, hostlen);
	
	safefree(buffer);
	return 0;
}

/*
 * Takes a host name and returns an array of IP addresses.  The caller
 * is responsible for freeing the 'addresses' memory block.
 */
int
dns_getaddrbyname(int dns, const char* name, struct in_addr** addresses)
{
	char* buffer;
	ssize_t len;
	char *ptr;

	assert(dns >= 0);
	assert(name != NULL);
	assert(addresses != NULL);

	write_message(dns, "%s\n", name);
	len = readline(dns, &buffer);
	if (len < 0)
		return len;
	chomp(buffer, len);

	if (strncmp(buffer, "$fail", 5) == 0) {
		safefree(buffer);
		return -ENOMSG;
	}

	ptr = strchr(buffer + 6, ' ');
	if (ptr)
		*ptr = '\0';
	
	*addresses = safemalloc(sizeof(struct in_addr));
	if (!*addresses) {
		safefree(buffer);
		return -ENOMEM;
	}

	inet_aton(buffer + 6, *addresses);

	safefree(buffer);
	return 0;
}
