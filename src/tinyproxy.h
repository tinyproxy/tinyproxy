/* $Id: tinyproxy.h,v 1.34 2002-11-03 17:10:32 rjkaes Exp $
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

struct config_s {
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
	bool_t filter_url;
	bool_t filter_extended;
#endif				/* FILTER_ENABLE */
#ifdef XTINYPROXY_ENABLE
	char *my_domain;
#endif
#ifdef UPSTREAM_SUPPORT
	char *upstream_name;
	int upstream_port;
#endif				/* UPSTREAM_SUPPORT */
	char *pidpath;
	unsigned int idletimeout;
	char* bind_address;

	char* dnsserver_location;
	char* dnsserver_socket;
};

/* Global Structures used in the program */
extern struct config_s config;
extern bool_t received_sighup;
extern bool_t processed_config_file;

#endif
