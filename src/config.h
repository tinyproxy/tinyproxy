/* $Id: config.h,v 1.1.1.1 2000-02-16 17:32:22 sdyoung Exp $
 *
 * Contains all the tune-able variables which are used by tinyproxy.
 * Modifications made to these variables WILL change the default behaviour
 * of tinyproxy. Please read the comments to better understand what each
 * variable does.
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

#ifndef _CONFIG_H_
#define _CONFIG_H_	1

/* Change these if you want */

/* Default log file */
#ifndef DEFAULT_LOG
#define DEFAULT_LOG  "/usr/local/var/log/tinyproxy.log"
#endif

/* Default port tinyproxy listens on */
#ifndef DEFAULT_PORT
#define DEFAULT_PORT 8080
#endif

/* Default user to change to after */
#ifndef DEFAULT_USER
#define DEFAULT_USER "nobody"
#endif

/*
 * Define this if you want to have all requests sent to a particular machine.
 * This is useful if you have another proxy further upstream which you must
 * send your HTTP requests through.
 */
#undef UPSTREAM

#ifdef UPSTREAM
#define UPSTREAM_PROXY_NAME "name.of.upstream.machine"
#define UPSTREAM_PROXY_PORT 80
#endif

/*
 * Define this if you want tinyproxy to use /proc/loadavg to determine
 * system load (Linux only, I think)
 */
#define USE_PROC

#ifndef USE_PROC
/*
 * Path to uptime to determin system load.  This path doesn't have to be
 * valid if DEFAULT_CUTOFFLOAD is 0
 */
#define UPTIME_PATH "/usr/bin/uptime"
#endif				/* !USE_PROC */

/*
 * The default load at which tinyproxy will start refusing connections.
 * 0 == disabled by default
 */
#define DEFAULT_CUTOFFLOAD 0

/*
 * NOTE: for DEFAULT_STATHOST:  this controls remote proxy stats display.
 * for example, the default DEFAULT_STATHOST of "tinyproxy.stats" will
 * mean that when you use the proxy to access http://tinyproxy.stats/",
 * you will be shown the proxy stats.  Set this to something obscure
 * if you don't want random people to be able to see them, or set it to
 * "" to disable.  In the future, I figure maybe some sort of auth
 * might be desirable, but that would involve a major simplicity
 * sacrifice.
 */

/* The "hostname" for getting tinyproxy stats. "" = disabled by default */
#define DEFAULT_STATHOST "tinyproxy.stats"

/*
 * NOTE: change these if you know what you're doing
 */
#define SOCK_TIMEOUT 5
/* Recalculate load avery 30 seconds */
#define LOAD_RECALCTIMER 30
/* Default HTTP port */
#define HTTP_PORT 80
/* Every time conncoll is called, nuke connections idle for > 60 seconds */
#define STALECONN_TIME (60 * 15)
/* Every 100 times through run the garbage collection */
#define GARBCOLL_INTERVAL 10

#endif
