/* $Id: tinyproxy.h,v 1.5 2000-09-14 16:41:20 rjkaes Exp $
 *
 * See 'tinyproxy.c' for a detailed description.
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

#ifndef _TINYPROXY_TINYPROXY_H_
#define _TINYPROXY_TINYPROXY_H_

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

/*
 * Include standard headers which are used through-out tinyproxy
 */
#ifdef HAVE_SYS_SELECT_H
#  include	<sys/select.h>
#endif
#include	<sys/socket.h>
#include	<sys/stat.h>
#include	<sys/time.h>
#include	<sys/types.h>
#include	<sys/uio.h>
#include	<arpa/inet.h>
#include	<netinet/in.h>
#include	<errno.h>
#include	<fcntl.h>
#include	<netdb.h>
#ifdef HAVE_PTHREAD_H
#  include	<pthread.h>
#endif
#ifdef HAVE_STDINT_H
#  include	<stdint.h>
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#ifdef HAVE_STRINGS_H
#  include	<strings.h>
#endif
#include	<time.h>
#include	<unistd.h>

#ifndef SHUT_RD			/* these three Posix.1g names are quite new */
#  define SHUT_RD	0	/* shutdown for reading */
#  define SHUT_WR	1	/* shutdown for writing */
#  define SHUT_RDWR	2	/* shutdown for reading and writing */
#endif

/* Global variables for the main controls of the program */
#define MAXBUFFSIZE	(1024 * 48)	/* Max size of buffer */
#define MAXLISTEN	1024		/* Max number of connections */
#define MAX_IDLE_TIME 	(60 * 10)	/* 10 minutes of no activity */

/* Useful function macros */
#define min(a,b)	((a) < (b) ? (a) : (b))
#define max(a,b)	((a) > (b) ? (a) : (b))

/* Make a new type: bool_t */
typedef enum {
	FALSE = 0,
	TRUE = (!FALSE)
} bool_t;

struct config_s {
	FILE *logf;
	char *logf_name;
	bool_t syslog;
	int port;
	char *stathost;
	bool_t quit;
	char *username;
	char *group;
	bool_t anonymous;
	char *ipAddr;
#ifdef FILTER_ENABLE
	char *filter;
#endif				/* FILTER_ENABLE */
#ifdef XTINYPROXY_ENABLE
	char *my_domain;
#endif
#ifdef TUNNEL_SUPPORT
	char *tunnel_name;
	int tunnel_port;
#endif				/* TUNNEL_SUPPORT */
	char *pidpath;
	unsigned int idletimeout;
};

struct conn_s {
	int client_fd, server_fd;
	struct buffer_s *cbuffer, *sbuffer;
	bool_t simple_req;
	char *output_message;
};

/* Global Structures used in the program */
extern struct config_s config;

#endif
