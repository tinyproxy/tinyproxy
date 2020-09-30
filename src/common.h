/* tinyproxy - A fast light-weight HTTP proxy
 * Copyright (C) 2002 Robert James Kaes <rjkaes@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/* This file groups all the headers required throughout the tinyproxy
 * system.  All this information use to be in the "main.h" header,
 * but various other "libraries" in the program need the same information,
 * without the tinyproxy specific defines.
 */

#ifndef COMMON_HEADER_H
#define COMMON_HEADER_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

/*
 * Include standard headers which are used through-out tinyproxy
 */

/* standard C headers - we can safely assume they exist. */
#include	<stddef.h>
#include	<stdint.h>
#include	<ctype.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>
/* standard POSIX headers - they need to be there as well. */
#  include	<errno.h>
#  include	<fcntl.h>
#  include	<netdb.h>
#  include      <signal.h>
#  include      <stdarg.h>
#  include	<strings.h>
#  include      <syslog.h>
#  include	<wchar.h>
#  include	<wctype.h>
#  include      <sys/mman.h>
#  include	<sys/select.h>
#  include	<sys/socket.h>
#  include	<sys/stat.h>
#  include      <sys/types.h>
#  include	<sys/wait.h>
#  include	<sys/uio.h>
#  include	<sys/un.h>
#  include	<sys/time.h>
#  include	<time.h>
#  include	<inttypes.h>
#  include      <sys/resource.h>
#  include	<netinet/in.h>
#  include      <assert.h>
#  include	<arpa/inet.h>
#  include	<grp.h>
#  include	<pwd.h>

/* rest - some oddball headers */
#ifdef HAVE_VALUES_H
#  include      <values.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#  include	<sys/ioctl.h>
#endif

#ifdef HAVE_ALLOCA_H
#  include	<alloca.h>
#endif

#ifdef HAVE_MEMORY_H
#  include	<memory.h>
#endif

#ifdef HAVE_MALLOC_H
#  include	<malloc.h>
#endif

#ifdef HAVE_SYSEXITS_H
#  include	<sysexits.h>
#endif

/*
 * If MSG_NOSIGNAL is not defined, define it to be zero so that it doesn't
 * cause any problems.
 */
#ifndef MSG_NOSIGNAL
#  define MSG_NOSIGNAL (0)
#endif

#ifndef SHUT_RD                 /* these three Posix.1g names are quite new */
#  define SHUT_RD	0       /* shutdown for reading */
#  define SHUT_WR	1       /* shutdown for writing */
#  define SHUT_RDWR	2       /* shutdown for reading and writing */
#endif

#define MAXLISTEN	1024    /* Max number of connections */

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
