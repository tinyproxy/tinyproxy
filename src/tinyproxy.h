/* $Id: tinyproxy.h,v 1.18 2001-10-25 16:58:50 rjkaes Exp $
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

#ifndef TINYPROXY_TINYPROXY_H
#define TINYPROXY_TINYPROXY_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

/*
 * Include standard headers which are used through-out tinyproxy
 */
#include        <sys/types.h>
#ifdef HAVE_SYS_SELECT_H
#  include	<sys/select.h>
#endif
#include	<sys/socket.h>
#include	<sys/stat.h>
#ifdef TIME_WITH_SYS_TIME
#  include	<sys/time.h>
#  include	<time.h>
#else
#  ifdef HAVE_SYS_TIME_H
#    include 	<sys/time.h>
#  else
#    include	<time.h>
#  endif
#endif
#include	<sys/uio.h>
#include	<netinet/in.h>
#include	<arpa/inet.h>
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
#include        <unistd.h>
#include        <assert.h>

#ifndef SHUT_RD			/* these three Posix.1g names are quite new */
#  define SHUT_RD	0	/* shutdown for reading */
#  define SHUT_WR	1	/* shutdown for writing */
#  define SHUT_RDWR	2	/* shutdown for reading and writing */
#endif

/* Global variables for the main controls of the program */
#define MAXBUFFSIZE	((size_t)(1024 * 48))	/* Max size of buffer */
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
#ifdef UPSTREAM_SUPPORT
	char *upstream_name;
	int upstream_port;
#endif				/* UPSTREAM_SUPPORT */
	char *pidpath;
	unsigned int idletimeout;

};

/* Global Structures used in the program */
extern struct config_s config;

#endif
