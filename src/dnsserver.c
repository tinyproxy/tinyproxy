/* $Id: dnsserver.c,v 1.1 2002-05-23 04:40:42 rjkaes Exp $
 *
 * This is a DNS resolver program.  I've borrowed _liberally_ from
 * Squid's dnsserver program.  Only one argument is passed to this
 * program (the path to the Unix socket.)  Additionally, this program
 * will be started by tinyproxy itself.
 * 
 * As noted, communication is conducted over a Unix socket.  The
 * client opens a connection and writes a '\n' terminated string with
 * either a domain name (if an address is required) or a
 * dotted-decimal IP address (if the domain name is required.)  This
 * program will then respond with a '\n' terminated string starting
 * with either "$addr" if IP addresses are being returned, or "$name"
 * if a host name is returned.  The connection will remain open until
 * the "$shutdown" command is sent by the client.
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

#include "common.h"

#include "daemon.h"
#include "heap.h"
#include "network.h"
#include "text.h"

/*
 * Store a copy of the location of the Unix socket
 */
static char *unix_socket_path;

/*
 * Handle SIGCHLD signals so that zombies are reaped.
 */
static void
child_signal_handler(int signo)
{
	pid_t pid;
	int status;

	while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
		;
	return;
}

/*
 * Handle the SIGTERM signal to remove the Unix socket
 */
static void
term_signal_handler(int signo)
{
	unlink(unix_socket_path);
	return;
}

/*
 * Text error messages for DNS problems.
 */
static char*
dns_error_messages(int err)
{
	if (err == HOST_NOT_FOUND)
		return "Host not found (authoritative)";
	else if (err == TRY_AGAIN)
		return "Host not found (non-authoritative)";
	else if (err == NO_RECOVERY)
		return "Non recoverable errors";
	else if (err == NO_DATA || err == NO_ADDRESS)
		return "Valid name, no data record of requested type";
	else
		return "Unknown DNS problem";
}

/*
 * Lookup and return the information for a host name (or IP address)
 */
static int
lookup_dns(int fd)
{
	const struct hostent *result = NULL;
	int reverse_lookup = 0;
	int retry = 0;
	int i;
	struct in_addr addr;
	char *buf;
	ssize_t len;
	char address_buf[1024];

	len = readline(fd, &buf);
	chomp(buf, len);

	if (strcmp(buf, "$shutdown") == 0)
		return 0;
	if (strcmp(buf, "$hello") == 0) {
		write_message(fd, "$alive\n");
		return 1;
	}
	
	/*
	 * Check to see if the buffer is an IP address in dotted-decimal
	 * form.
	 */
	for (;;) {
		if (inet_aton(buf, &addr)) {
			reverse_lookup = 1;
			result = gethostbyaddr((char *)&addr.s_addr, 4, AF_INET);
		} else {
			result = gethostbyname(buf);
		}

		if (NULL != result)
			break;
		if (h_errno != TRY_AGAIN)
			break;
		if (++retry == 3)
			break;
		sleep(1);
	}

	if (NULL == result) {
		if (h_errno == TRY_AGAIN) {
			write_message(fd, "$fail Name Server for domain '%s' is unavailable.\n", buf);
		} else {
			write_message(fd, "$fail DNS Domain '%s' is invalid: %s.\n",
				      buf, dns_error_messages(h_errno));
		}
		return 1;
	}

	if (reverse_lookup) {
		write_message(fd, "$name %s\n", result->h_name);
		return 1;
	}

	strlcpy(address_buf, "$addr", sizeof(address_buf));
	for (i = 0; i < 32; i++) {
		if (!result->h_addr_list[i])
			break;

		memcpy(&addr, result->h_addr_list[i], sizeof(addr));

		strlcat(address_buf, " ", sizeof(address_buf));
		strlcat(address_buf, inet_ntoa(addr), sizeof(address_buf));
	}
	write_message(fd, "%s\n", address_buf);

	return 1;
}

/*
 * Print out a usage message.
 */
static void
usage(void)
{
	fprintf(stderr, "usage: dnsserver path\n");
}

/*
 * Start up function.  Create the sockets and fork() when needed.
 */
int
main(int argc, char **argv)
{
	int listenfd, connfd;
	pid_t childpid;
	socklen_t clilen;
	struct sockaddr_un cliaddr, servaddr;

	if (--argc != 1) {
		usage();
		exit(1);
	}

	/*
	 * The path for the socket is supplied as the one (and only) command
	 * line argument.
	 */
	unix_socket_path = safestrdup(argv[1]);
	if (!unix_socket_path) {
		fprintf(stderr, "%s: could not allocate memory.\n", argv[0]);
		exit(1);
	}
	
	/* Create the listening socket */
	listenfd = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (listenfd < 0) {
		fprintf(stderr, "%s: could not create listening socket.\n",
			argv[0]);
		exit(1);
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sun_family = AF_LOCAL;
	strcpy(servaddr.sun_path, unix_socket_path);

	if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		fprintf(stderr, "%s: could not bind socket.\n", argv[0]);
		exit(1);
	}
	if (listen(listenfd, MAXLISTEN) < 0) {
		fprintf(stderr, "%s: could not start listening on socket.\n",
			argv[0]);
		exit(1);
	}

	set_signal_handler(SIGCHLD, child_signal_handler);
	set_signal_handler(SIGTERM, term_signal_handler);

	for ( ; ; ) {
		clilen = sizeof(cliaddr);
		if ((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen)) < 0) {
			if (errno == EINTR)
				continue;
			else
				exit(1);
		}

		if ((childpid = fork()) == 0) {
			/* child process */
			close(listenfd);

			while (lookup_dns(connfd))
				;

			close(connfd);
			exit(0);
		}

		close(connfd);		/* parent closes connected socket */
	}
}
