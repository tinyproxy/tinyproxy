/* $Id: tinyproxy.c,v 1.9 2001-05-27 02:36:22 rjkaes Exp $
 *
 * The initialise routine. Basically sets up all the initial stuff (logfile,
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

#include "tinyproxy.h"

#ifdef HAVE_SYS_RESOURCE_H
#  include <sys/resource.h>
#endif /* HAVE_SYS_RESOUCE_H */
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <sysexits.h>
#include <syslog.h>

#include "anonymous.h"
#include "buffer.h"
#include "dnscache.h"
#include "filter.h"
#include "log.h"
#include "reqs.h"
#include "sock.h"
#include "stats.h"
#include "thread.h"
#include "utils.h"

void takesig(int sig);

extern int yyparse(void);
extern FILE *yyin;

/* 
 * Global Structures
 */
struct config_s config;
float load = 0.00;

/*
 * Handle a signal
 */
void takesig(int sig)
{
	switch (sig) {
	case SIGHUP:
		if (config.logf)
			ftruncate(fileno(config.logf), 0);

		log_message(LOG_NOTICE, "SIGHUP received, cleaning up...");

#ifdef FILTER_ENABLE
		if (config.filter) {
			filter_destroy();
			filter_init();
		}
		log_message(LOG_NOTICE, "Re-reading filter file.");
#endif				/* FILTER_ENABLE */
		log_message(LOG_NOTICE, "Finished cleaning memory/connections.");		       
		break;
	case SIGTERM:
#ifdef FILTER_ENABLE
		if (config.filter)
			filter_destroy();
#endif				/* FILTER_ENABLE */
		config.quit = TRUE;
		log_message(LOG_INFO, "SIGTERM received.");
		break;
	}

	if (sig != SIGTERM)
		signal(sig, takesig);
	signal(SIGPIPE, SIG_IGN);
}

/*
 * Display the version information for the user.
 */
static void versiondisp(void)
{
	printf("tinyproxy " VERSION "\n");
	printf("\
Copyright 1998       Steven Young (sdyoung@well.com)\n\
Copyright 1998-2000  Robert James Kaes (rjkaes@flarenet.com)\n\
Copyright 2000       Chris Lightfoot (chris@ex-parrot.com)\n\
\n\
This program is free software; you can redistribute it and/or modify it\n\
under the terms of the GNU General Public License as published by the Free\n\
Software Foundation; either version 2, or (at your option) any later\n\
version.\n\
\n\
This program is distributed in the hope that it will be useful, but WITHOUT\n\
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or\n\
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for\n\
more details.\n");
}

/*
 * Display usage to the user.
 */
static void usagedisp(void)
{
	printf("\
Usage: tinyproxy [options]\n\
Options:\n\
  -d		Operate in DEBUG mode.\n\
  -c FILE	Use an alternate configuration file.\n\
  -h		Display this usage information.\n\
  -v            Display the version number.\n");


	/* Display the modes compiled into tinyproxy */
	printf("\nFeatures Compiled In:\n");
#ifdef XTINYPROXY_ENABLE
	printf("    XTinyproxy Header\n");
#endif				/* XTINYPROXY */
#ifdef FILTER_ENABLE
	printf("    Filtering\n");
	printf("        * with Regular Expression support\n");
#endif				/* FILTER_ENABLE */
#ifndef NDEBUG
	printf("    Debugging code\n");
#endif				/* NDEBUG */
#ifdef TUNNEL_SUPPORT
	printf("    TCP Tunneling\n");
#endif				/* TUNNEL_SUPPORT */
}

int main(int argc, char **argv)
{
	int optch;
	bool_t godaemon = TRUE;
	struct passwd *thisuser = NULL;
	struct group *thisgroup = NULL;
	char *conf_file = DEFAULT_CONF_FILE;

	/*
	 * Disable the creation of CORE files right up front.
	 */
#ifdef HAVE_SETRLIMIT
	struct rlimit core_limit = {0, 0};
	if (setrlimit(RLIMIT_CORE, &core_limit) < 0) {
		log_message(LOG_EMERG, "tinyproxy: could not set the core limit to zero.");
		exit(EX_SOFTWARE);
	}
#endif /* HAVE_SETRLIMIT */

	/*
	 * Process the various options
	 */
	while ((optch = getopt(argc, argv, "c:vdh")) !=
	       EOF) {
		switch (optch) {
		case 'v':
			versiondisp();
			exit(EX_OK);
		case 'd':
			godaemon = FALSE;
			break;
		case 'c':
			conf_file = strdup(optarg);
			if (!conf_file) {
				log_message(LOG_EMERG, "tinyproxy: could not allocate memory");
				exit(EX_SOFTWARE);
			}
			break;
		case 'h':
		default:
			usagedisp();
			exit(EX_OK);
		}
	}

	/*
	 * Read in the settings from the config file.
	 */
	yyin = fopen(conf_file, "r");
	if (!yyin) {
		log_message(LOG_ERR, "Could not open %s file", conf_file);
		exit(EX_SOFTWARE);
	}
	yyparse();

	/* Open the log file if not using syslog */
	if (config.syslog == FALSE) {
		if (!config.logf_name) {
			log_message(LOG_INFO, "Setting Logfile to \"%s\"", DEFAULT_LOG);
			config.logf_name = DEFAULT_LOG;
		}

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

	log_message(LOG_INFO, PACKAGE " " VERSION " starting...");

	/*
	 * Set the default values if they were not set in the config file.
	 */
	if (config.port == 0) {
		log_message(LOG_INFO, "Setting Port to %u", DEFAULT_PORT);
		config.port = DEFAULT_PORT;
	}
	if (!config.stathost) {
		log_message(LOG_INFO, "Setting stathost to \"%s\"", DEFAULT_STATHOST);
		config.stathost = DEFAULT_STATHOST;
	}
	if (!config.username) {
		log_message(LOG_INFO, "Setting user to \"%s\"", DEFAULT_USER);
		config.username = DEFAULT_USER;
	}
	if (config.idletimeout == 0) {
		log_message(LOG_INFO, "Setting idle timeout to %u seconds", MAX_IDLE_TIME);
		config.idletimeout = MAX_IDLE_TIME;
	}

	init_stats();

	/*
	 * If ANONYMOUS is turned on, make sure that Content-Length is
	 * in the list of allowed headers, since it is required in a
	 * HTTP/1.0 request. Also add the Content-Type header since it goes
	 * hand in hand with Content-Length.
	 *     - rjkaes
	 */
	if (config.anonymous) {
		anon_insert("Content-Length:");
		anon_insert("Content-Type:");
	}

	if (godaemon == TRUE)
		makedaemon();

	if (config.pidpath) {
		pidfile_create(config.pidpath);
	}

	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
		log_message(LOG_CRIT, "Could not set SIGPIPE\n");
		exit(EX_OSERR);
	}

#ifdef FILTER_ENABLE
	if (config.filter)
		filter_init();
#endif				/* FILTER_ENABLE */

	/*
	 * Start listening on the selected port.
	 */
	if (thread_listening_sock(config.port) < 0) {
		log_message(LOG_CRIT, "Problem creating listening socket");
		exit(EX_OSERR);
	}

	/*
	 * Switch to a different user.
	 */
	if (geteuid() == 0) {
		if (config.group && strlen(config.group) > 0) {
			thisgroup = getgrnam(config.group);
			if (!thisgroup) {
				log_message(LOG_ERR, "Unable to find group '%s'!", config.group);
				exit(EX_NOUSER);
			}
			if (setgid(thisgroup->gr_gid) < 0) {
				log_message(LOG_ERR, "Unable to change to group '%s'", config.group);
				exit(EX_CANTCREAT);
			}
			log_message(LOG_INFO, "Now running as group %s", config.group);
		}
		if (config.username && strlen(config.username) > 0) {
			thisuser = getpwnam(config.username);
			if (!thisuser) {
				log_message(LOG_ERR, "Unable to find user '%s'!", config.username);
				exit(EX_NOUSER);
			}
			if (setuid(thisuser->pw_uid) < 0) {
				log_message(LOG_ERR, "Unable to change to user '%s'", config.username);
				exit(EX_CANTCREAT);
			}
			log_message(LOG_INFO, "Now running as user %s", config.username);
		}
	} else {
		log_message(LOG_WARNING, "Not running as root, so not changing UID/GID.");
	}

	if (thread_pool_create() < 0) {
		log_message(LOG_ERR, "Could not create the pool of threads");
		exit(EX_SOFTWARE);
	}

	/*
	 * These signals are only for the main thread.
	 */
	log_message(LOG_INFO, "Setting the various signals.");
	if (signal(SIGTERM, takesig) == SIG_ERR) {
		log_message(LOG_CRIT, "Could not set SIGTERM\n");
		exit(EX_OSERR);
	}
	if (signal(SIGHUP, takesig) == SIG_ERR) {
		log_message(LOG_CRIT, "Could not set SIGHUP\n");
		exit(EX_OSERR);
	}

	/*
	 * Initialize the various subsystems...
	 */
	log_message(LOG_INFO, "Starting the DNS caching subsystem.");
	if (!new_dnscache())
		exit(EX_SOFTWARE);
	log_message(LOG_INFO, "Starting the Anonymous header subsystem.");
	if (!new_anonymous())
		exit(EX_SOFTWARE);

	/*
	 * Start the main loop.
	 */
	log_message(LOG_INFO, "Starting main loop. Accepting connections.");
	do {
		thread_main_loop();
		sleep(1);
	} while (!config.quit);

	log_message(LOG_INFO, "Shutting down.");
	thread_close_sock();

	/*
	 * Remove the PID file.
	 */
	if (unlink(config.pidpath) < 0) {
		log_message(LOG_WARNING, "Could not remove PID file %s: %s",
		    config.pidpath, strerror(errno));
	}

#ifdef FILTER_ENABLE
	if (config.filter)
		filter_destroy();
#endif				/* FILTER_ENABLE */

	if (config.syslog == FALSE)
		fclose(config.logf);
	else
		closelog();

	exit(EX_OK);
}
