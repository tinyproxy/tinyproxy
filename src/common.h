/* $Id: common.h,v 1.6 2003-06-25 18:20:22 rjkaes Exp $
 *
 * This file groups all the headers required throughout the tinyproxy
 * system.  All this information use to be in the "tinyproxy.h" header,
 * but various other "libraries" in the program need the same information,
 * without the tinyproxy specific defines.
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

#ifndef COMMON_HEADER_H
#define COMMON_HEADER_H

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

#ifdef TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
#else
#  ifdef HAVE_SYS_TIME_H
#    include <sys/time.h>
#  else
#    include <time.h>
#  endif
#endif

#ifdef HAVE_SYS_RESOURCE_H
#  include      <sys/resource.h>
#endif
#ifdef HAVE_SYS_UIO_H
#  include	<sys/uio.h>
#endif
#ifdef HAVE_SYS_UN_H
#  include	<sys/un.h>
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
#ifdef HAVE_MEMORY_H
#  include	<memory.h>
#endif
#ifdef HAVE_NETDB_H
#  include	<netdb.h>
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
#else
#  ifdef HAVE_MALLOC_H
#    include	<malloc.h>
#  endif
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
#ifdef HAVE_SYS_MMAN_H
#  include      <sys/mman.h>
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

#define MAXLISTEN	1024	/* Max number of connections */

/*
 * SunOS doesn't have INADDR_NONE defined.
 */
#ifndef INADDR_NONE
#  define INADDR_NONE -1
#endif

/* Define boolean values */
#ifndef FALSE
# define FALSE 0
# define TRUE (!FALSE)
#endif

/* Useful function macros */
#if !defined(min) || !defined(max)
#  define min(a,b)	((a) < (b) ? (a) : (b))
#  define max(a,b)	((a) > (b) ? (a) : (b))
#endif

#endif
