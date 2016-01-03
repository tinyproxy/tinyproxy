/* tinyproxy - A fast light-weight HTTP proxy
 *
 * Copyright (C) 1998 Steven Young <sdyoung@miranda.org>
 * Copyright (C) 1998-2002 Robert James Kaes <rjkaes@users.sourceforge.net>
 * Copyright (C) 2000 Chris Lightfoot <chris@ex-parrot.com>
 * Copyright (C) 2009-2010 Mukund Sivaraman <muks@banu.com>
 * Copyright (C) 2009-2010 Michael Adam <obnox@samba.org>
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

/* The initialize routine. Basically sets up all the initial stuff (logfile,
 * listening socket, config options, etc.) and then sits there and loops
 * over the new connections until the daemon is closed. Also has additional
 * functions to handle the "user friendly" aspects of a program (usage,
 * stats, etc.) Like any good program, most of the work is actually done
 * elsewhere.
 */

#include "main.h"

#include "anonymous.h"
#include "authors.h"
#include "buffer.h"
#include "conf.h"
#include "daemon.h"
#include "heap.h"
#include "filter.h"
#include "child.h"
#include "log.h"
#include "reqs.h"
#include "sock.h"
#include "stats.h"
#include "utils.h"

/*
 * Global Structures
 */
struct config_s config;
struct config_s config_defaults;
unsigned int received_sighup = FALSE;   /* boolean */

/*
 * Handle a signal
 */
static void
takesig (int sig)
{
        pid_t pid;
        int status;

        switch (sig) {
        case SIGHUP:
                received_sighup = TRUE;
                break;

        case SIGTERM:
                config.quit = TRUE;
                break;

        case SIGCHLD:
                while ((pid = waitpid (-1, &status, WNOHANG)) > 0) ;
                break;
        }

        return;
}

/*
 * Display the version information for the user.
 */
static void
display_version (void)
{
        printf ("%s %s\n", PACKAGE, VERSION);
}

/*
 * Display the copyright and license for this program.
 */
static void
display_license (void)
{
        const char * const *authors;
        const char * const *documenters;

        display_version ();

        printf ("\
\n\
  Copyright 1998       Steven Young (sdyoung@well.com)\n\
  Copyright 1998-2002  Robert James Kaes (rjkaes@users.sourceforge.net)\n\
  Copyright 1999       George Talusan (gstalusan@uwaterloo.ca)\n\
  Copyright 2000       Chris Lightfoot (chris@ex-parrot.com)\n\
  Copyright 2009-2010  Mukund Sivaraman (muks@banu.com)\n\
  Copyright 2009-2010  Michael Adam (obnox@samba.org)\n\
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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.\n\
\n");

        printf ("\nAUTHORS:\n");
        for (authors = authors_get_authors (); *authors; authors++) {
                printf ("  %s\n", *authors);
        }

        printf ("\nDOCUMENTERS:\n");
        for (documenters = authors_get_documenters ();
             *documenters; documenters++) {
                printf ("  %s\n", *documenters);
        }

        printf ("\n");
}

/*
 * Display usage to the user.
 */
static void
display_usage (void)
{
        int features = 0;

        printf ("Usage: %s [options]\n", PACKAGE);
        printf ("\n"
                "Options are:\n"
                "  -d        Do not daemonize (run in foreground).\n"
                "  -c FILE   Use an alternate configuration file.\n"
                "  -h        Display this usage information.\n"
                "  -l        Display the license.\n"
                "  -v        Display version information.\n");

        /* Display the modes compiled into tinyproxy */
        printf ("\nFeatures compiled in:\n");

#ifdef XTINYPROXY_ENABLE
        printf ("    XTinyproxy header\n");
        features++;
#endif /* XTINYPROXY */

#ifdef FILTER_ENABLE
        printf ("    Filtering\n");
        features++;
#endif /* FILTER_ENABLE */

#ifndef NDEBUG
        printf ("    Debugging code\n");
        features++;
#endif /* NDEBUG */

#ifdef TRANSPARENT_PROXY
        printf ("    Transparent proxy support\n");
        features++;
#endif /* TRANSPARENT_PROXY */

#ifdef REVERSE_SUPPORT
        printf ("    Reverse proxy support\n");
        features++;
#endif /* REVERSE_SUPPORT */

#ifdef UPSTREAM_SUPPORT
        printf ("    Upstream proxy support\n");
        features++;
#endif /* UPSTREAM_SUPPORT */

        if (0 == features)
                printf ("    None\n");

        printf ("\n"
                "For support and bug reporting instructions, please visit\n"
                "<https://tinyproxy.github.io/>.\n");
}

static int
get_id (char *str)
{
        char *tstr;

        if (str == NULL)
                return -1;

        tstr = str;
        while (*tstr != 0) {
                if (!isdigit (*tstr))
                        return -1;
                tstr++;
        }

        return atoi (str);
}

/**
 * process_cmdline:
 * @argc: argc as passed to main()
 * @argv: argv as passed to main()
 *
 * This function parses command line arguments.
 **/
static void
process_cmdline (int argc, char **argv, struct config_s *conf)
{
        int opt;

        while ((opt = getopt (argc, argv, "c:vldh")) != EOF) {
                switch (opt) {
                case 'v':
                        display_version ();
                        exit (EX_OK);

                case 'l':
                        display_license ();
                        exit (EX_OK);

                case 'd':
                        conf->godaemon = FALSE;
                        break;

                case 'c':
                        if (conf->config_file != NULL) {
                                safefree (conf->config_file);
                        }
                        conf->config_file = safestrdup (optarg);
                        if (!conf->config_file) {
                                fprintf (stderr,
                                         "%s: Could not allocate memory.\n",
                                         argv[0]);
                                exit (EX_SOFTWARE);
                        }
                        break;

                case 'h':
                        display_usage ();
                        exit (EX_OK);

                default:
                        display_usage ();
                        exit (EX_USAGE);
                }
        }
}

/**
 * change_user:
 * @program: The name of the program. Pass argv[0] here.
 *
 * This function tries to change UID and GID to the ones specified in
 * the config file. This function is typically called during
 * initialization when the effective user is root.
 **/
static void
change_user (const char *program)
{
        if (config.group && strlen (config.group) > 0) {
                int gid = get_id (config.group);

                if (gid < 0) {
                        struct group *thisgroup = getgrnam (config.group);

                        if (!thisgroup) {
                                fprintf (stderr,
                                         "%s: Unable to find group \"%s\".\n",
                                         program, config.group);
                                exit (EX_NOUSER);
                        }

                        gid = thisgroup->gr_gid;
                }

                if (setgid (gid) < 0) {
                        fprintf (stderr,
                                 "%s: Unable to change to group \"%s\".\n",
                                 program, config.group);
                        exit (EX_NOPERM);
                }

#ifdef HAVE_SETGROUPS
                /* Drop all supplementary groups, otherwise these are inherited from the calling process */
                if (setgroups (0, NULL) < 0) {
                        fprintf (stderr,
                                 "%s: Unable to drop supplementary groups.\n",
                                 program);
                        exit (EX_NOPERM);
                }
#endif

                log_message (LOG_INFO, "Now running as group \"%s\".",
                             config.group);
        }

        if (config.user && strlen (config.user) > 0) {
                int uid = get_id (config.user);

                if (uid < 0) {
                        struct passwd *thisuser = getpwnam (config.user);

                        if (!thisuser) {
                                fprintf (stderr,
                                         "%s: Unable to find user \"%s\".\n",
                                         program, config.user);
                                exit (EX_NOUSER);
                        }

                        uid = thisuser->pw_uid;
                }

                if (setuid (uid) < 0) {
                        fprintf (stderr,
                                 "%s: Unable to change to user \"%s\".\n",
                                 program, config.user);
                        exit (EX_NOPERM);
                }

                log_message (LOG_INFO, "Now running as user \"%s\".",
                             config.user);
        }
}

static void initialize_config_defaults (struct config_s *conf)
{
        memset (conf, 0, sizeof(*conf));

        conf->config_file = safestrdup (SYSCONFDIR "/tinyproxy.conf");
        if (!conf->config_file) {
                fprintf (stderr, PACKAGE ": Could not allocate memory.\n");
                exit (EX_SOFTWARE);
        }
        conf->godaemon = TRUE;
        /*
         * Make sure the HTML error pages array is NULL to begin with.
         * (FIXME: Should have a better API for all this)
         */
        conf->errorpages = NULL;
        conf->stathost = safestrdup (TINYPROXY_STATHOST);
        conf->idletimeout = MAX_IDLE_TIME;
        conf->logf_name = safestrdup (LOCALSTATEDIR "/log/tinyproxy/tinyproxy.log");
        conf->pidpath = safestrdup (LOCALSTATEDIR "/run/tinyproxy/tinyproxy.pid");
}

/**
 * convenience wrapper around reload_config_file
 * that also re-initializes logging.
 */
int reload_config (void)
{
        int ret;

        shutdown_logging ();

        ret = reload_config_file (config_defaults.config_file, &config,
                                  &config_defaults);
        if (ret != 0) {
                goto done;
        }

        ret = setup_logging ();

done:
        return ret;
}

int
main (int argc, char **argv)
{
        /* Only allow u+rw bits. This may be required for some versions
         * of glibc so that mkstemp() doesn't make us vulnerable.
         */
        umask (0177);

        log_message (LOG_INFO, "Initializing " PACKAGE " ...");

        if (config_compile_regex()) {
                exit (EX_SOFTWARE);
        }

        initialize_config_defaults (&config_defaults);
        process_cmdline (argc, argv, &config_defaults);

        if (reload_config_file (config_defaults.config_file,
                                &config,
                                &config_defaults)) {
                exit (EX_SOFTWARE);
        }

        init_stats ();

        /* If ANONYMOUS is turned on, make sure that Content-Length is
         * in the list of allowed headers, since it is required in a
         * HTTP/1.0 request. Also add the Content-Type header since it
         * goes hand in hand with Content-Length. */
        if (is_anonymous_enabled ()) {
                anonymous_insert ("Content-Length");
                anonymous_insert ("Content-Type");
        }

        if (config.godaemon == TRUE)
                makedaemon ();

        if (set_signal_handler (SIGPIPE, SIG_IGN) == SIG_ERR) {
                fprintf (stderr, "%s: Could not set the \"SIGPIPE\" signal.\n",
                         argv[0]);
                exit (EX_OSERR);
        }

#ifdef FILTER_ENABLE
        if (config.filter)
                filter_init ();
#endif /* FILTER_ENABLE */

        /* Start listening on the selected port. */
        if (child_listening_sockets(config.listen_addrs, config.port) < 0) {
                fprintf (stderr, "%s: Could not create listening sockets.\n",
                         argv[0]);
                exit (EX_OSERR);
        }

        /* Switch to a different user if we're running as root */
        if (geteuid () == 0)
                change_user (argv[0]);
        else
                log_message (LOG_WARNING,
                             "Not running as root, so not changing UID/GID.");

        /* Create log file after we drop privileges */
        if (setup_logging ()) {
                exit (EX_SOFTWARE);
        }

        /* Create pid file after we drop privileges */
        if (config.pidpath) {
                if (pidfile_create (config.pidpath) < 0) {
                        fprintf (stderr, "%s: Could not create PID file.\n",
                                 argv[0]);
                        exit (EX_OSERR);
                }
        }

        if (child_pool_create () < 0) {
                fprintf (stderr,
                         "%s: Could not create the pool of children.\n",
                         argv[0]);
                exit (EX_SOFTWARE);
        }

        /* These signals are only for the parent process. */
        log_message (LOG_INFO, "Setting the various signals.");

        if (set_signal_handler (SIGCHLD, takesig) == SIG_ERR) {
                fprintf (stderr, "%s: Could not set the \"SIGCHLD\" signal.\n",
                         argv[0]);
                exit (EX_OSERR);
        }

        if (set_signal_handler (SIGTERM, takesig) == SIG_ERR) {
                fprintf (stderr, "%s: Could not set the \"SIGTERM\" signal.\n",
                         argv[0]);
                exit (EX_OSERR);
        }

        if (set_signal_handler (SIGHUP, takesig) == SIG_ERR) {
                fprintf (stderr, "%s: Could not set the \"SIGHUP\" signal.\n",
                         argv[0]);
                exit (EX_OSERR);
        }

        /* Start the main loop */
        log_message (LOG_INFO, "Starting main loop. Accepting connections.");

        child_main_loop ();

        log_message (LOG_INFO, "Shutting down.");

        child_kill_children (SIGTERM);
        child_close_sock ();

        /* Remove the PID file */
        if (unlink (config.pidpath) < 0) {
                log_message (LOG_WARNING,
                             "Could not remove PID file \"%s\": %s.",
                             config.pidpath, strerror (errno));
        }

#ifdef FILTER_ENABLE
        if (config.filter)
                filter_destroy ();
#endif /* FILTER_ENABLE */

        shutdown_logging ();

        return EXIT_SUCCESS;
}
