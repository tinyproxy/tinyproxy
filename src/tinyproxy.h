/* $Id: tinyproxy.h,v 1.1.1.1 2000-02-16 17:32:23 sdyoung Exp $
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

#ifndef _TINYPROXY_H_
#define _TINYPROXY_H_	1

#ifdef HAVE_CONFIG_H
#include <defines.h>
#endif

#include <stdio.h>
#include <time.h>

#include "config.h"

/* Global variables for the main controls of the program */
#define BUFFER (1024 * 2)	/* Size of buffer for reading */
#define MAXLISTEN 128		/* Max number of connections to listen for */

/* Make a new type: flag */
typedef char flag;

/* Other stuff */
#define FALSE (0)
#define TRUE  (!FALSE)

struct config_s {
	FILE *logf;
	char *logf_name;
	flag syslog;
	float cutoffload;
	int port;
	char *stathost;
	flag quit;
	char *changeuser;
	flag anonymous;
	char *subnet;
	char *ipAddr;
#ifdef FILTER_ENABLE
	char *filter;
#endif				/* FILTER_ENABLE */
	flag restricted;
#ifdef XTINYPROXY
	char *my_domain;
#endif
};

struct stat_s {
	unsigned long int num_reqs;
	unsigned long int num_cons;
	unsigned long int num_badcons;
	unsigned long int num_opens;
	unsigned long int num_listens;
	unsigned long int num_tx;
	unsigned long int num_rx;
	unsigned long int num_garbage;
	unsigned long int num_idles;
	unsigned long int num_refused;
};

struct allowedhdr_s {
	char *hdrname;
	struct allowedhdr_s *next;
};

/* Global Structures used in the program */
extern struct config_s config;
extern struct stat_s stats;
extern struct allowedhdr_s *allowedhdrs;
extern float load;

#endif
