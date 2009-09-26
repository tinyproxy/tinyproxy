/* tinyproxy - A fast light-weight HTTP proxy
 * Copyright (C) 1998 Steven Young <sdyoung@miranda.org>
 * Copyright (C) 1998-2002 Robert James Kaes <rjkaes@users.sourceforge.net>
 * Copyright (C) 2000 Chris Lightfoot <chris@ex-parrot.com>
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
unsigned int received_sighup = FALSE;   /* boolean */
unsigned int processed_config_file = FALSE;     /* boolean */

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
        display_version ();

        printf ("\
  Copyright 1998       Steven Young (sdyoung@well.com)\n\
  Copyright 1998-2002  Robert James Kaes (rjkaes@users.sourceforge.net)\n\
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

        if (0 == features)
                printf ("    None\n");

        printf ("\n"
                "For bug reporting instructions, please see:\n"
                "<https://www.banu.com/tinyproxy/support/>.\n");
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
process_cmdline (int argc, char **argv)
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
                        config.godaemon = FALSE;
                        break;

                case 'c':
                        config.config_file = safestrdup (optarg);
                        if (!config.config_file) {
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

int
main (int argc, char **argv)
{
        FILE *config_file;

        /* Only allow u+rw bits. This may be required for some versions
         * of glibc so that mkstemp() doesn't make us vulnerable.
         */
        umask (0177);

        config.config_file = SYSCONFDIR "/tinyproxy.conf";
        config.godaemon = TRUE;

        process_cmdline (argc, argv);

        log_message (LOG_INFO, "Initializing " PACKAGE " ...");

        /*
         * Make sure the HTML error pages array is NULL to begin with.
         * (FIXME: Should have a better API for all this)
         */
        config.errorpages = NULL;

        /* Read in the settings from the config file */
        config_file = fopen (config.config_file, "r");
        if (!config_file) {
                fprintf (stderr,
                         "%s: Could not open config file \"%s\".\n",
                         argv[0], config.config_file);
                exit (EX_SOFTWARE);
        }

        if (config_compile () || config_parse (&config, config_file)) {
                fprintf (stderr, "Unable to parse config file. "
                         "Not starting.\n");
                exit (EX_SOFTWARE);
        }

        fclose (config_file);

        /* Write to a user supplied log file if it's defined.  This will
         * override using the syslog even if syslog is defined. */
        if (config.logf_name) {
                if (open_log_file (config.logf_name) < 0) {
                        fprintf (stderr,
                                 "%s: Could not create log file.\n", argv[0]);
                        exit (EX_SOFTWARE);
                }
                config.syslog = FALSE;  /* disable syslog */
        } else if (config.syslog) {
                if (config.godaemon == TRUE)
                        openlog ("tinyproxy", LOG_PID, LOG_DAEMON);
                else
                        openlog ("tinyproxy", LOG_PID, LOG_USER);
        } else {
                fprintf (stderr, "%s: Either define a logfile or "
                         "enable syslog logging.\n", argv[0]);
                exit (EX_SOFTWARE);
        }

        processed_config_file = TRUE;
        send_stored_logs ();

        /* Set the default values if they were not set in the config
         * file. */
        if (config.port == 0) {
                fprintf (stderr, "%s: You MUST set a Port in the "
                         "config file.\n", argv[0]);
                exit (EX_SOFTWARE);
        }

        if (!config.stathost) {
                log_message (LOG_INFO, "Setting stathost to \"%s\".",
                             TINYPROXY_STATHOST);
                config.stathost = strdup (TINYPROXY_STATHOST);
        }

        if (!config.user) {
                log_message (LOG_WARNING, "You SHOULD set a UserName in the "
                             "config file. Using current user instead.");
        }

        if (config.idletimeout == 0) {
                log_message (LOG_WARNING, "Invalid idle time setting. "
                             "Only values greater than zero are allowed. "
                             "Therefore setting idle timeout to %u seconds.",
                             MAX_IDLE_TIME);
                config.idletimeout = MAX_IDLE_TIME;
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

        if (config.pidpath) {
                if (pidfile_create (config.pidpath) < 0) {
                        fprintf (stderr, "%s: Could not create PID file.\n",
                                 argv[0]);
                        exit (EX_OSERR);
                }
        }

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
        if (child_listening_sock (config.port) < 0) {
                fprintf (stderr, "%s: Could not create listening socket.\n",
                         argv[0]);
                exit (EX_OSERR);
        }

        /* Switch to a different user if we're running as root */
        if (geteuid () == 0)
                change_user (argv[0]);
        else
                log_message (LOG_WARNING,
                             "Not running as root, so not changing UID/GID.");

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

        child_kill_children ();
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

        if (config.syslog)
                closelog ();
        else
                close_log_file ();

        return EXIT_SUCCESS;
}
