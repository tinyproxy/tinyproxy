/* $Id: tinyproxy.c,v 1.17 2001-09-16 20:11:54 rjkaes Exp $
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
		log_message(LOG_NOTICE, "SIGHUP received, cleaning up.");

		if (config.logf) {
			char *rename_file;
			int log_file_des;
			FILE *old_fd;

			rename_file = safemalloc(strlen(config.logf_name) + 5);
			if (!rename_file) {
				fprintf(stderr, "Could not allocate memory in signal handler!\n");
				exit(EX_OSERR);
			}

			strcpy(rename_file, config.logf_name);
			strcat(rename_file, ".rot");

			rename(config.logf_name, rename_file);

			log_file_des = create_file_safely(config.logf_name);
			if (log_file_des < 0) {
				fprintf(stderr, "Could not safely create new log file.\n");
				exit(EX_OSERR);
			}

			old_fd = config.logf;
			
			if (!(config.logf = fdopen(log_file_des, "w"))) {
				fprintf(stderr, "Could not create new log file.\n");
				exit(EX_CANTCREAT);
			}

			fclose(old_fd);

			log_message(LOG_NOTICE, "Log file rotated.");

			safefree(rename_file);
		}

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
static void display_version(void)
{
	printf("%s %s (%s)\n", PACKAGE, VERSION, TARGET_SYSTEM);
}

/*
 * Display the copyright and license for this program.
 */
static void display_license(void)
{
	display_version();

	printf("\
  Copyright 1998       Steven Young (sdyoung@well.com)\n\
  Copyright 1998-2001  Robert James Kaes (rjkaes@users.sourceforge.net)\n\
  Copyright 1999       George Talusan (gstalusan@uwaterloo.ca)\n\
  Copyright 2000       Chris Lightfoot (chris@ex-parrot.com)\n\
\n\
  This program is free software; you can redistribute it and/or modify\n\
  it under the terms of the GNU General Public License as published by\n\
  the Free Software Foundation; either version 2, or (at your option)\n\
  any later version.\n\
\n\
  This program is distributed in the hope that it will be useful,\n\
  but WITHOUT ANY WARRANTY; without even the implied warranty of\n\
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n\
  GNU General Public License for more details.\n\
\n\
  You should have received a copy of the GNU General Public License\n\
  along with this program; if not, write to the Free Software\n\
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.\n");
}

/*
 * Display usage to the user.
 */
static void display_usage(void)
{
	printf("Usage: %s [options]\n", PACKAGE);
	printf("\
Options:\n\
  -d		Operate in DEBUG mode.\n\
  -c FILE	Use an alternate configuration file.\n\
  -h		Display this usage information.\n\
  -l            Display the license.\n\
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
		fprintf(stderr, "%s: Could not set the core limit to zero.\n", argv[0]);
		exit(EX_SOFTWARE);
	}
#endif /* HAVE_SETRLIMIT */

	/*
	 * Process the various options
	 */
	while ((optch = getopt(argc, argv, "c:vldh")) !=
	       EOF) {
		switch (optch) {
		case 'v':
			display_version();
			exit(EX_OK);
		case 'l':
			display_license();
			exit(EX_OK);
		case 'd':
			godaemon = FALSE;
			break;
		case 'c':
			conf_file = strdup(optarg);
			if (!conf_file) {
				fprintf(stderr, "%s: Could not allocate memory.\n", argv[0]);
				exit(EX_SOFTWARE);
			}
			break;
		case 'h':
		default:
		        display_usage();
			exit(EX_OK);
		}
	}

	/*
	 * Read in the settings from the config file.
	 */
	yyin = fopen(conf_file, "r");
	if (!yyin) {
		fprintf(stderr, "%s: Could not open configuration file \"%s\".\n", argv[0], conf_file);
		exit(EX_SOFTWARE);
	}
	yyparse();

#if defined(TUNNEL_SUPPORT) && defined(UPSTREAM_SUPPORT)
	if (config.tunnel_name && config.upstream_name) {
		fprintf(stderr, "%s: \"Tunnel\" and \"Upstream\" directives can not be both set.\n", argv[0]);
		exit(EX_SOFTWARE);
	}
#endif

	/* Open the log file if not using syslog */
	if (config.syslog == FALSE) {
		int log_file_fd;

		if (!config.logf_name) {
			fprintf(stderr, "%s: You MUST set a LogFile in the configuration file.\n", argv[0]);
			exit(EX_SOFTWARE);
		}

		log_file_fd = create_file_safely(config.logf_name);
		if (log_file_fd < 0) {
			fprintf(stderr, "Could not safely create logfile \"%s\".\n", config.logf_name);
			exit(EX_CANTCREAT);
		}

		config.logf = fdopen(log_file_fd, "w");
		if (!config.logf) {
			fprintf(stderr, "Could not write to log file \"%s\".\n", config.logf_name);
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
		fprintf(stderr, "%s: You MUST set a Port in the configuration file.\n", argv[0]);
		exit(EX_SOFTWARE);
	}
	if (!config.stathost) {
		log_message(LOG_INFO, "Setting stathost to \"%s\".", DEFAULT_STATHOST);
		config.stathost = DEFAULT_STATHOST;
	}
	if (!config.username) {
		log_message(LOG_WARNING, "You SHOULD set a UserName in the configuration file. Using current user instead.");
	}
	if (config.idletimeout == 0) {
		log_message(LOG_INFO, "Setting idle timeout to %u seconds.", MAX_IDLE_TIME);
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
	if (is_anonymous_enabled()) {
		anonymous_insert("Content-Length");
		anonymous_insert("Content-Type");
	}

	if (godaemon == TRUE)
		makedaemon();

	if (config.pidpath) {
		pidfile_create(config.pidpath);
	}

	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
		fprintf(stderr, "%s: Could not set the \"SIGPIPE\" signal.\n", argv[0]);
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
		fprintf(stderr, "%s: Could not create listening socket.\n", argv[0]);
		exit(EX_OSERR);
	}

	/*
	 * Switch to a different user.
	 */
	if (geteuid() == 0) {
		if (config.group && strlen(config.group) > 0) {
			thisgroup = getgrnam(config.group);
			if (!thisgroup) {
				fprintf(stderr, "%s: Unable to find group \"%s\".\n", argv[0], config.group);
				exit(EX_NOUSER);
			}
			if (setgid(thisgroup->gr_gid) < 0) {
				fprintf(stderr, "%s: Unable to change to group \"%s\".\n", argv[0], config.group);
				exit(EX_CANTCREAT);
			}
			log_message(LOG_INFO, "Now running as group \"%s\".", config.group);
		}
		if (config.username && strlen(config.username) > 0) {
			thisuser = getpwnam(config.username);
			if (!thisuser) {
				fprintf(stderr, "%s: Unable to find user \"%s\".", argv[0], config.username);
				exit(EX_NOUSER);
			}
			if (setuid(thisuser->pw_uid) < 0) {
				fprintf(stderr, "%s: Unable to change to user \"%s\".", argv[0], config.username);
				exit(EX_CANTCREAT);
			}
			log_message(LOG_INFO, "Now running as user \"%s\".", config.username);
		}
	} else {
		log_message(LOG_WARNING, "Not running as root, so not changing UID/GID.");
	}

	if (thread_pool_create() < 0) {
		fprintf(stderr, "%s: Could not create the pool of threads.", argv[0]);
		exit(EX_SOFTWARE);
	}

	/*
	 * These signals are only for the main thread.
	 */
	log_message(LOG_INFO, "Setting the various signals.");
	if (signal(SIGTERM, takesig) == SIG_ERR) {
		fprintf(stderr, "%s: Could not set the \"SIGTERM\" signal.\n", argv[0]);
		exit(EX_OSERR);
	}
	if (signal(SIGHUP, takesig) == SIG_ERR) {
		fprintf(stderr, "%s: Could not set the \"SIGHUP\" signal.\n", argv[0]);
		exit(EX_OSERR);
	}

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
		log_message(LOG_WARNING, "Could not remove PID file \"%s\": %s.",
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
