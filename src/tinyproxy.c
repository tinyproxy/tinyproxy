/* $Id: tinyproxy.c,v 1.2 2000-03-11 20:37:44 rjkaes Exp $
 *
 * The initialize routine. Basically sets up all the initial stuff (logfile,
 * listening socket, config options, etc.) and then sits there and loops
 * over the new connections until the daemon is closed. Also has additional
 * functions to handle the "user friendly" aspects of a program (usage,
 * stats, etc.) Like any good program, most of the work is actually done
 * elsewhere.
 *
 * Copyright (C) 1998  Steven Young
 * Copyright (C) 1999  Robert James Kaes (rjkaes@flarenet.com)
 * Copyright (C) 2000  Chris Lightfoot (chris@ex-parrot.com>
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

#ifdef HAVE_CONFIG_H
#include <defines.h>
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <stdlib.h>
#include <sysexits.h>
#include <pwd.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <syslog.h>
#include <unistd.h>
#include <fcntl.h>

/* chris - need this for asynchronous DNS resolution */
#include <adns.h>

adns_state adns;

#include "config.h"
#include "tinyproxy.h"
#include "utils.h"
#include "log.h"
#include "sock.h"
#include "conns.h"
#include "reqs.h"
#include "buffer.h"
#include "filter.h"

void takesig(int sig);

/* 
 * Global Structures
 */
struct config_s config = {
	NULL,			/* Log file handle */
	DEFAULT_LOG,		/* Logfile name */
	FALSE,			/* Use syslog instead? */
	DEFAULT_CUTOFFLOAD,	/* Cut off load */
	DEFAULT_PORT,		/* Listen on this port */
	DEFAULT_STATHOST,	/* URL of stats host */
	FALSE,			/* Quit? */
	DEFAULT_USER,		/* Name of user to change to */
	FALSE,			/* Run anonymous by default? */
	NULL,			/* String containing the subnet allowed */
	NULL,			/* IP address to listen on */
#ifdef FILTER_ENABLE
	NULL,			/* Location of filter file */
#endif				/* FILTER_ENABLE */
	FALSE,			/* Restrict the log to only errors */
#ifdef XTINYPROXY
	NULL,			/* The name of this domain */
#endif
#ifdef UPSTREAM_PROXY
	NULL,			/* name of the upstream proxy */
	0,			/* port of the upstream proxy */
#endif
};

struct stat_s stats;
float load = 0.00;
struct allowedhdr_s *allowedhdrs = NULL;

/*
 * Dump info to the logfile
 */
static void dumpdebug(void)
{
	struct conn_s *connptr = connections;
	long clients = 0, waiting = 0, relaying = 0, closing = 0, finished = 0;

	log("SIGUSR1 received, debug dump follows.");

	while (connptr) {
		switch (connptr->type) {
		case NEWCONN:
			clients++;
			break;
		case WAITCONN:
			waiting++;
			break;
		case RELAYCONN:
			relaying++;
			break;
		case CLOSINGCONN:
			closing++;
			break;
		case FINISHCONN:
			finished++;
			break;
		default:
			break;
		}
		connptr = connptr->next;
	}
	log("clients: %d, waiting: %d, relaying: %d," \
	    "closing: %d, finished: %d",
	    clients, waiting, relaying, closing, finished);
	log("total requests handled: %lu", stats.num_reqs);
	log("total connections handled: %lu", stats.num_cons);
	log("total sockets listened: %lu", stats.num_listens);
	log("total sockets opened: %lu", stats.num_opens);
	log("total bad opens: %lu", stats.num_badcons);
	log("total bytes tx: %lu", stats.num_tx);
	log("total bytes rx: %lu", stats.num_rx);
	log("connections refused due to load: %lu", stats.num_refused);
	log("garbage collections: %lu", stats.num_garbage);
	log("idle connections killed: %lu", stats.num_idles);
	log("end debug dump.");
}

/*
 * Handle a signal
 */
void takesig(int sig)
{
	switch (sig) {
	case SIGUSR1:
		dumpdebug();
		break;
	case SIGHUP:
		if (config.logf)
			ftruncate(fileno(config.logf), 0);

		log("SIGHUP received, cleaning up...");
		conncoll();
		garbcoll();

#ifdef FILTER_ENABLE
		if (config.filter) {
			filter_destroy();
			filter_init();
		}
		log("Re-reading filter file.");
#endif				/* FILTER_ENABLE */
		log("Finished cleaning memory/connections.");		       
		break;
	case SIGTERM:
#ifdef FILTER_ENABLE
		if (config.filter)
			filter_destroy();
#endif				/* FILTER_ENABLE */
		config.quit = TRUE;
		break;
	case SIGALRM:
		calcload();
		alarm(LOAD_RECALCTIMER);
		break;
	}
	if (sig != SIGTERM)
		signal(sig, takesig);
	signal(SIGPIPE, SIG_IGN);
}

/*
 * Display usage to the user on stderr.
 */
static void usagedisp(void)
{
	printf("tinyproxy version " VERSION "\n");
	printf("Copyright 1998       Steven Young (sdyoung@well.com)\n");
	printf
	    ("Copyright 1998-1999  Robert James Kaes (rjkaes@flarenet.com)\n\n");
	printf("Copyright 2000       Chris Lightfoot (chris@ex-parrot.com)\n");

	printf
	    ("This software is licensed under the GNU General Public License (GPL).\n");
	printf("See the file 'COPYING' included with tinyproxy source.\n\n");

	printf("Compiled with Ian Jackson's adns:\n");
	printf("   http://www.chiark.greenend.org.uk/~ian/adns/\n\n");

	printf("Usage: tinyproxy [args]\n");
	printf("Options:\n");
	printf("\t-a header\tallow 'header' through the anon block\n");
	printf("\t-d\t\tdo not daemonize\n");
#ifdef FILTER_ENABLE
	printf("\t-f filterfile\tblock sites specified in filterfile\n");
#endif				/* FILTER_ENABLE */
	printf("\t-h\t\tdisplay usage\n");
	printf("\t-i ip_address\tonly listen on this address\n");
	printf("\t-l logfile\tlog to 'logfile'\n");
	printf
	    ("\t-n ip_address\tallow access from only this subnet. (i.e. 192.168.0.)\n");
	printf("\t-p port\t\tlisten on 'port'\n");
	printf("\t-r\t\trestrict the log to only errors\n");
	printf("\t-s stathost\tset stathost to 'stathost'\n");
#ifdef HAVE_SYSLOG_H
	printf("\t-S\t\tlog using the syslog instead\n");
#endif
#ifdef UPSTREAM_PROXY
	printf("\t-t domain:port\tredirect connections to an upstream proxy\n");
#endif	
	printf("\t-u user\t\tchange to user after startup.  \"\" disables\n");
	printf("\t-v\t\tdisplay version number\n");
	printf
	    ("\t-w load\t\tstop accepting new connections at 'load'.  0 disables\n");
#ifdef XTINYPROXY
	printf
	    ("\t-x domain\tAdd a XTinyproxy header with the peer's IP address\n");
#endif
			/* UPSTREAM_PROXY */

	/* Display the modes compiled into tinyproxy */
	printf("\nFeatures Compiled In:\n");
#ifdef XTINYPROXY
	printf("    XTinyproxy Header\n");
#endif				/* XTINYPROXY */
#ifdef FILTER_ENABLE
	printf("    Filtering\n");
	printf("        * with Regular Expression support\n");
#endif				/* FILTER_ENABLE */
#ifndef NDEBUG
	printf("    Debuggin code\n");
#endif				/* NDEBUG */
#ifdef UPSTREAM_PROXY
	printf("    Upstream proxy\n");
#endif				/* UPSTREAM_PROXY */
}

int main(int argc, char **argv)
{
	int optch;
	flag usage = FALSE, godaemon = TRUE, changeid = FALSE;
	struct passwd *thisuser = NULL;

#ifdef UPSTREAM_PROXY
	char *upstream_ptr;
#endif	/* UPSTREAM_PROXY */

	struct allowedhdr_s **rpallowedptr = &allowedhdrs;
	struct allowedhdr_s *allowedptr = allowedhdrs, *newallowed;

	while ((optch = getopt(argc, argv, "vh?dp:l:Sa:w:s:u:n:i:rx:f:t:")) !=
	       EOF) {
		switch (optch) {
		case 'v':
			fprintf(stderr, "tinyproxy version " VERSION "\n");
			exit(EX_OK);
			break;
		case 'p':
			if (!(config.port = atoi(optarg))) {
				log
				    ("bad port on commandline, defaulting to %d",
				    DEFAULT_PORT);
				config.port = DEFAULT_PORT;
			}
			break;
		case 'l':
			if (!(config.logf_name = xstrdup(optarg))) {
				log("bad log file, defaulting to %s",
				    DEFAULT_LOG);
				config.logf_name = DEFAULT_LOG;
			}
			break;
#ifdef HAVE_SYSLOG_H
		case 'S':	/* Use the syslog function to handle logging */
			config.syslog = TRUE;
			break;
#endif
		case 'd':
			godaemon = FALSE;
			break;
		case 'w':
			sscanf(optarg, "%f", &config.cutoffload);
			break;
		case 's':
			if (!(config.stathost = xstrdup(optarg))) {
				log("bad stathost, defaulting to %s",
				    DEFAULT_STATHOST);
				config.stathost = DEFAULT_STATHOST;
			}
			break;
		case 'u':
			if (!(config.changeuser = xstrdup(optarg))) {
				log("bad user name, defaulting to %s",
				    DEFAULT_USER);
				config.changeuser = DEFAULT_USER;
			}
			break;
		case 'a':
			config.anonymous = TRUE;

			while (allowedptr) {
				rpallowedptr = &allowedptr->next;
				allowedptr = allowedptr->next;
			}

			if (!
			    (newallowed =
			     xmalloc(sizeof(struct allowedhdr_s)))) {
				log("tinyproxy: cannot allocate headers");
				exit(EX_SOFTWARE);
			}

			if (!(newallowed->hdrname = xstrdup(optarg))) {
				log("tinyproxy: cannot duplicate string");
				exit(EX_SOFTWARE);
			}

			*rpallowedptr = newallowed;
			newallowed->next = allowedptr;

			break;
		case 'n':
			if (!(config.subnet = xstrdup(optarg))) {
				log("tinyproxy: could not allocate memory");
				exit(EX_SOFTWARE);
			}
			break;
		case 'i':
			if (!(config.ipAddr = xstrdup(optarg))) {
				log("tinyproxy: could not allocate memory");
				exit(EX_SOFTWARE);
			}
			break;
#ifdef FILTER_ENABLE
		case 'f':
			if (!(config.filter = xstrdup(optarg))) {
				log("tinyproxy: could not allocate memory");
			}
			break;
#endif				/* FILTER_ENABLE */
		case 'r':
			config.restricted = TRUE;
			break;
#ifdef XTINYPROXY
		case 'x':
			if (!(config.my_domain = xstrdup(optarg))) {
				log("tinyproxy: could not allocate memory");
				exit(EX_SOFTWARE);
			}
			break;
#endif

#ifdef UPSTREAM_PROXY
		case 't':
			if (!(upstream_ptr = strchr(optarg, ':'))) {
				log("tinyproxy: invalid UPSTREAM declaration");
				break;
			}

			*upstream_ptr++ = '\0';
			if (!(config.upstream_name = xstrdup(optarg))) {
				log("tinyproxy: could not allocate memory");
				exit(EX_SOFTWARE);
			}
			config.upstream_port = atoi(upstream_ptr);
#ifndef NDEBUG
			log("upstream name: %s", config.upstream_name);
			log("upstream port: %d", config.upstream_port);
#endif	/* NDEBUG */

			break;
#endif				/* UPSTREAM_PROXY */

		case '?':
		case 'h':
		default:
			usage = TRUE;
			break;
		}
	}

	if (usage == TRUE) {
		usagedisp();
		exit(EX_OK);
	}

	/* chris - Initialise asynchronous DNS */
	if (adns_init(&adns, 0, 0)) {
		log("tinyproxy: could not initialise ADNS");
		exit(EX_SOFTWARE);
	}

	/* Open the log file if not using syslog */
	if (config.syslog == FALSE) {
		if (!(config.logf = fopen(config.logf_name, "a"))) {
			fprintf(stderr,
				"Unable to open logfile %s for appending!\n",
				config.logf_name);
			exit(EX_CANTCREAT);
		}
	} else {
		if (godaemon == TRUE)
			openlog("tinyproxy", LOG_PID, LOG_DAEMON);
		else
			openlog("tinyproxy", LOG_PID, LOG_USER);
	}

	log(PACKAGE " " VERSION " starting...");

	if (strlen(config.changeuser)) {
		if ((getuid() != 0) && (geteuid() != 0)) {
			log
			    ("not running as root, therefore not changing uid/gid.");
		} else {
			changeid = TRUE;
			if (!(thisuser = getpwnam(config.changeuser))) {
				log("unable to find user \"%s\"!",
				    config.changeuser);
				exit(EX_NOUSER);
			}
			log("changing to user \"%s\" (%d/%d).",
			    config.changeuser, thisuser->pw_uid,
			    thisuser->pw_gid);
		}
	}
#ifdef NDEBUG
	if (godaemon == TRUE)
		makedaemon();
#else
	printf("Debugging is enabled, so you can not go daemon.\n");
#endif

	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
		fprintf(stderr, "Could not set SIGPIPE\n");
		exit(EX_OSERR);
	}
	if (signal(SIGUSR1, takesig) == SIG_ERR) {
		fprintf(stderr, "Could not set SIGUSR1\n");
		exit(EX_OSERR);
	}
	if (signal(SIGTERM, takesig) == SIG_ERR) {
		fprintf(stderr, "Could not set SIGTERM\n");
		exit(EX_OSERR);
	}
	if (signal(SIGHUP, takesig) == SIG_ERR) {
		fprintf(stderr, "Could not set SIGHUP\n");
		exit(EX_OSERR);
	}
	if (signal(SIGALRM, takesig) == SIG_ERR) {
		fprintf(stderr, "Could not set SIGALRM\n");
		exit(EX_OSERR);
	}
	alarm(LOAD_RECALCTIMER);
	calcload();

	if (init_listen_sock(config.port) < 0) {
		log("unable to bind port %d!", config.port);
		exit(EX_UNAVAILABLE);
	}
	if (changeid == TRUE) {
		setuid(thisuser->pw_uid);
		setgid(thisuser->pw_gid);
	}
	log("now accepting connections.");

#ifdef FILTER_ENABLE
	if (config.filter)
		filter_init();
#endif				/* FILTER_ENABLE */

	while (config.quit == FALSE) {
		if (getreqs() < 0)
			break;
	}

#ifdef FILTER_ENABLE
	if (config.filter)
		filter_destroy();
#endif				/* FILTER_ENABLE */

	log("shutting down.");
	de_init_listen_sock();

	if (config.syslog == FALSE)
		fclose(config.logf);
	else
		closelog();

	allowedptr = allowedhdrs;
	while (allowedptr) {
		struct allowedhdr_s *delptr = NULL;
		delptr = allowedptr;
		safefree(delptr->hdrname);
		allowedptr = delptr->next;
		safefree(delptr);
	}

	/* finsih up ADNS */
	adns_finish(adns);

	exit(EX_OK);
}
