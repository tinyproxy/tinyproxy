/* tinyproxy - A fast light-weight HTTP proxy
 * Copyright (C) 1998 Steven Young <sdyoung@miranda.org>
 * Copyright (C) 1999 Robert James Kaes <rjkaes@users.sourceforge.net>
 * Copyright (C) 2009 Michael Adam <obnox@samba.org>
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

/* See 'log.c' for detailed information. */

#ifndef TINYPROXY_LOG_H
#define TINYPROXY_LOG_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

/*
 * Okay, I have modelled the levels for logging off the syslog() interface.
 * However, I would really prefer if only five of the levels are used. You
 * can see them below and I'll describe what each level should be for.
 * Hopefully tinyproxy will remain consistent with these levels.
 *	-- rjkaes
 * Sorry but I had to destroy the hope ;-) There was a need to log
 * connections without the INFO stuff and not to have them as NOTICE.
 *	-- hgb
 *
 * Level	Description
 * -----	-----------
 * LOG_CRIT	This is catastrophic. Basically, tinyproxy can not recover
 *		from this and will either close the child (if we're lucky),
 *		or the entire daemon. I would relegate this to conditions
 *		like unable to create the listening socket, or unable to
 *		create a child. If you're going to log at this level provide
 *		as much information as possible.
 *
 * LOG_ERR	Okay, something bad happened. We can recover from this, but
 *		the connection will be terminated. This should be for things
 *		like when we cannot create a socket, or out of memory.
 *		Basically, the connection will not work, but it's not enough
 *		to bring the whole daemon down.
 *
 * LOG_WARNING	There is condition which will change the behaviour of
 *		tinyproxy from what is expected. For example, somebody did
 *		not specify a port. tinyproxy will handle this (by using
 *		it's default port), but it's a _higher_ level situation
 *		which the admin should be aware of.
 *
 * LOG_NOTICE	This is for a special condition. Nothing has gone wrong, but
 *		it is more important than the common LOG_INFO level. Right
 *		now it is used for actions like creating/destroying children,
 *		unauthorized access, signal handling, etc.
 *
 * LOG_CONN	This additional level is for logging connections only, so
 *		it is easy to control only the requests in the logfile.
 *		If we log through syslog, this is set to LOG_INFO.
 *			-- hgb
 *
 * LOG_INFO	Everything else ends up here. Logging for incoming
 *		connections, denying due to filtering rules, unable to
 *		connect to remote server, etc.
 *
 * LOG_DEBUG	Don't use this level. :) Use the two DEBUG?() macros
 *		instead since they can remain in the source if needed. (I
 *		don't advocate this, but it could be useful at times.)
 */

#ifdef HAVE_SYSLOG_H
#  include <syslog.h>
#else
#  define LOG_CRIT    2
#  define LOG_ERR     3
#  define LOG_WARNING 4
#  define LOG_NOTICE  5
#  define LOG_INFO    6
#  define LOG_DEBUG   7
#endif

#define LOG_CONN      8         /* extra to log connections without the INFO stuff */

/* Suppress warnings when GCC is in -pedantic mode and not -std=c99 */
#if (__GNUC__ >= 3 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96))
#pragma GCC system_header
#endif

/*
 * Use this for debugging. The format is specific:
 *    DEBUG1("There was a major problem");
 *    DEBUG2("There was a big problem: %s in connptr %p", "hello", connptr);
 */
#ifndef NDEBUG
# define DEBUG1(x) \
  log_message(LOG_DEBUG, "[%s:%d] " x, __FILE__, __LINE__)
# define DEBUG2(x, y...) \
  log_message(LOG_DEBUG, "[%s:%d] " x, __FILE__, __LINE__, ## y)
#else
# define DEBUG1(x)       do { } while(0)
# define DEBUG2(x, y...) do { } while(0)
#endif

extern int open_log_file (const char *file);
extern void close_log_file (void);

extern void log_message (int level, const char *fmt, ...);
extern void set_log_level (int level);
extern void send_stored_logs (void);

extern int setup_logging (void);
extern void shutdown_logging (void);

#endif
