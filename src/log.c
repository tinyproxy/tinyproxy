/* $Id: log.c,v 1.15 2001-10-25 17:27:39 rjkaes Exp $
 *
 * Logs the various messages which tinyproxy produces to either a log file or
 * the syslog daemon. Not much to it...
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

#include "tinyproxy.h"

#include "log.h"

static char *syslog_level[] = {
	NULL,
	NULL,
	"CRITICAL",
	"ERROR",
	"WARNING",
	"NOTICE",
	"INFO",
	"DEBUG",
	"CONNECT"
};

#define TIME_LENGTH 16
#define STRING_LENGTH 800

/*
 * Store the log level setting.
 */
static short int log_level = LOG_ERR;

/*
 * Set the log level for writing to the log file.
 */
void set_log_level(short int level)
{
	log_level = level;
}

/*
 * This routine logs messages to either the log file or the syslog function.
 */
void log_message(short int level, char *fmt, ...)
{
	va_list args;
	time_t nowtime;
	FILE *cf;

	char time_string[TIME_LENGTH];
#if defined(HAVE_SYSLOG_H) && !defined(HAVE_VSYSLOG_H)
	char str[STRING_LENGTH];
#endif

#ifdef NDEBUG
	/*
	 * Figure out if we should write the message or not.
	 */
	if (log_level == LOG_CONN) {
		if (level == LOG_INFO)
			return;
	} else if (log_level == LOG_INFO) {
		if (level > LOG_INFO && level != LOG_CONN)
			return;
	} else if (level > log_level)
			return;
#endif
	

#ifdef HAVE_SYSLOG_H
	if (config.syslog && level == LOG_CONN) 
		level = LOG_INFO;
#endif

	va_start(args, fmt);

#ifdef HAVE_SYSLOG_H
	if (config.syslog) {
#  ifdef HAVE_VSYSLOG_H
		vsyslog(level, fmt, args);
#  else
		vsnprintf(str, STRING_LENGTH, fmt, args);
		syslog(level, "%s", str);
#  endif
	} else {
#endif
		nowtime = time(NULL);
		/* Format is month day hour:minute:second (24 time) */
		strftime(time_string, TIME_LENGTH, "%b %d %H:%M:%S",
			 localtime(&nowtime));

		if (!(cf = config.logf))
			cf = stderr;

		fprintf(cf, "%-9s %s [%ld]: ", syslog_level[level],
			time_string, (long int)getpid());
		vfprintf(cf, fmt, args);
		fprintf(cf, "\n");
		fflush(cf);
#ifdef HAVE_SYSLOG_H
	}
#endif

	va_end(args);
}
