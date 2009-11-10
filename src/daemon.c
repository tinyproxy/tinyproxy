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

/* This file contains functions which are useful when writing a
 * daemon process.  The functions include a "makedaemon" function and
 * a function to portably set a signal handler.
 */

#include "main.h"

#include "daemon.h"
#include "log.h"

/*
 * Fork a child process and then kill the parent so make the calling
 * program a daemon process.
 */
void makedaemon (void)
{
        if (fork () != 0)
                exit (0);

        setsid ();
        set_signal_handler (SIGHUP, SIG_IGN);

        if (fork () != 0)
                exit (0);

	if (chdir ("/") != 0) {
                log_message (LOG_WARNING,
                             "Could not change directory to /");
	}

        umask (0177);

#ifdef NDEBUG
        /*
         * When not in debugging mode, close the standard file
         * descriptors.
         */
        close (0);
        close (1);
        close (2);
#endif
}

/*
 * Pass a signal number and a signal handling function into this function
 * to handle signals sent to the process.
 */
signal_func *set_signal_handler (int signo, signal_func * func)
{
        struct sigaction act, oact;

        act.sa_handler = func;
        sigemptyset (&act.sa_mask);
        act.sa_flags = 0;
        if (signo == SIGALRM) {
#ifdef SA_INTERRUPT
                act.sa_flags |= SA_INTERRUPT;   /* SunOS 4.x */
#endif
        } else {
#ifdef SA_RESTART
                act.sa_flags |= SA_RESTART;     /* SVR4, 4.4BSD */
#endif
        }

        if (sigaction (signo, &act, &oact) < 0)
                return SIG_ERR;

        return oact.sa_handler;
}
