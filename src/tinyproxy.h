/* $Id: tinyproxy.h,v 1.26 2001-12-28 22:31:12 rjkaes Exp $
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
#ifdef HAVE_SYS_TYPES_H
#  include      <sys/types.h>
#endif
#ifdef HAVE_INTTYPES_H
#  include	<inttypes.h>
#endif
#ifdef HAVE_STDDEF_H
#  include	<stddef.h>
#endif
#ifdef HAVE_STDINT_H
#  include	<stdint.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#  include	<sys/ioctl.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#  include	<sys/select.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#  include	<sys/socket.h>
#endif
#ifdef HAVE_SYS_STAT_H
#  include	<sys/stat.h>
#endif

#ifdef HAVE_SYS_TIME_H
#  include	<sys/time.h>
#  ifdef HAVE_TIME_H
#    include	<time.h>
#  endif
#else
#  ifdef HAVE_SYS_TIME_H
#    include 	<sys/time.h>
#  else
#    ifdef HAVE_TIME_H
#      include	<time.h>
#    endif
#  endif
#endif

#ifdef HAVE_SYS_RESOURCE_H
#  include      <sys/resource.h>
#endif
#ifdef HAVE_SYS_UIO_H
#  include	<sys/uio.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#  include	<sys/wait.h>
#endif

#ifdef HAVE_NETINET_IN_H
#  include	<netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#  include	<arpa/inet.h>
#endif
#ifdef HAVE_ALLOCA_H
#  include	<alloca.h>
#endif
#ifdef HAVE_ASSERT_H
#  include      <assert.h>
#endif
#ifdef HAVE_CTYPE_H
#  include      <ctype.h>
#endif
#ifdef HAVE_ERRNO_H
#  include	<errno.h>
#endif
#ifdef HAVE_FCNTL_H
#  include	<fcntl.h>
#endif
#ifdef HAVE_GRP_H
#  include    	<grp.h>
#endif
#ifdef HAVE_MALLOC_H
#  include	<malloc.h>
#endif
#ifdef HAVE_MEMORY_H
#  include	<memory.h>
#endif
#ifdef HAVE_NETDB_H
#  include	<netdb.h>
#endif
#ifdef HAVE_PTHREAD_H
#  include	<pthread.h>
#else
#  ifdef HAVE_PTHREADS_H
#    include 	<pthreads.h>
#  endif
#endif
#ifdef HAVE_PWD_H
#  include     	<pwd.h>
#endif
#ifdef HAVE_SIGNAL_H
#  include      <signal.h>
#endif
#ifdef HAVE_STDARG_H
#  include      <stdarg.h>
#endif
#ifdef HAVE_STDIO_H
#  include	<stdio.h>
#endif
#ifdef HAVE_STDLIB_H
#  include	<stdlib.h>
#endif
#ifdef HAVE_STRING_H
#  include	<string.h>
#endif
#ifdef HAVE_STRINGS_H
#  include	<strings.h>
#endif
#ifdef HAVE_SYSEXITS_H
#  include     	<sysexits.h>
#endif
#ifdef HAVE_SYSLOG_H
#  include      <syslog.h>
#endif
#ifdef HAVE_UNISTD_H
#  include      <unistd.h>
#endif
#ifdef HAVE_VFORK_H
#  include	<vfork.h>
#endif
#ifdef HAVE_WCHAR_H
#  include	<wchar.h>
#endif
#ifdef HAVE_WCTYPE_H
#  include	<wctype.h>
#endif

/*
 * If MSG_NOSIGNAL is not defined, define it to be zero so that it doesn't
 * cause any problems.
 */
#ifndef MSG_NOSIGNAL
#  define MSG_NOSIGNAL (0)
#endif

#ifndef SHUT_RD			/* these three Posix.1g names are quite new */
#  define SHUT_RD	0	/* shutdown for reading */
#  define SHUT_WR	1	/* shutdown for writing */
#  define SHUT_RDWR	2	/* shutdown for reading and writing */
#endif

/* Global variables for the main controls of the program */
#define MAXBUFFSIZE	((size_t)(1024 * 96))	/* Max size of buffer */
#define MAXLISTEN	1024	/* Max number of connections */
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
