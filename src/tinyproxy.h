/* $Id: tinyproxy.h,v 1.41.2.1 2004-08-06 16:56:55 rjkaes Exp $
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

#include "common.h"

/* Global variables for the main controls of the program */
#define MAXBUFFSIZE	((size_t)(1024 * 96))	/* Max size of buffer */
#define MAX_IDLE_TIME 	(60 * 10)	/* 10 minutes of no activity */

/*
 * Even if upstream support is not compiled into tinyproxy, this
 * structure still needs to be defined.
 */
struct upstream {
	struct upstream *next;
	char *domain; /* optional */
	char *host;
	int port;
	in_addr_t ip, mask;
};

struct config_s {
	char *logf_name;
	char *config_file;
	unsigned int syslog; /* boolean */
	int port;
	char *stathost;
	unsigned int quit; /* boolean */
	char *username;
	char *group;
	char *ipAddr;
#ifdef FILTER_ENABLE
	char *filter;
	unsigned int filter_url; /* boolean */
	unsigned int filter_extended; /* boolean */
        unsigned int filter_casesensitive; /* boolean */
#endif				/* FILTER_ENABLE */
#ifdef XTINYPROXY_ENABLE
	char *my_domain;
#endif
#ifdef UPSTREAM_SUPPORT
	struct upstream *upstream_list;
#endif				/* UPSTREAM_SUPPORT */
	char *pidpath;
	unsigned int idletimeout;
	char* bind_address;

	/*
	 * The configured name to use in the HTTP "Via" header field.
	 */
	char* via_proxy_name;

	/* 
	 * Error page support.  This is an array of pointers to structures
	 * which describe the error page path, and what HTTP error it handles.
	 * an example would be { "/usr/local/etc/tinyproxy/404.html", 404 }
	 * Ending of array is noted with NULL, 0.
	 */
	struct error_pages_s {
		char *errorpage_path;
		unsigned int errorpage_errnum;
	} **errorpages;
	/* 
	 * Error page to be displayed if appropriate page cannot be located
	 * in the errorpages structure.
	 */
	char *errorpage_undef;

	/* 
	 * The HTML statistics page. 
	 */
	char *statpage;
};

/* Global Structures used in the program */
extern struct config_s config;
extern unsigned int received_sighup; /* boolean */
extern unsigned int processed_config_file; /* boolean */

#endif
