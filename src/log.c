/* $Id: log.c,v 1.1.1.1 2000-02-16 17:32:22 sdyoung Exp $
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
 *
 * log.c - For the manipulation of log files.
 */

#ifdef HAVE_CONFIG_H
#include <defines.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <syslog.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include "tinyproxy.h"
#include "log.h"

#define LENGTH 16

/*
 * This routine logs messages to either the log file or the syslog function.
 */
void log(char *fmt, ...)
{
	va_list args;
	time_t nowtime;
	FILE *cf;

#if defined(HAVE_SYSLOG_H) && !defined(HAVE_VSYSLOG_H)
	static char str[800];
#endif
	static char time_string[LENGTH];

	assert(fmt);

	va_start(args, fmt);

#ifdef HAVE_SYSLOG_H
	if (config.syslog == FALSE) {
#endif
		nowtime = time(NULL);
		/* Format is month day hour:minute:second (24 time) */
		strftime(time_string, LENGTH, "%b %d %H:%M:%S",
			 localtime(&nowtime));

		if (!(cf = config.logf))
			cf = stderr;

		fprintf(cf, "%s [%d]: ", time_string, getpid());
		vfprintf(cf, fmt, args);
		fprintf(cf, "\n");
		fflush(cf);
#ifdef HAVE_SYSLOG_H
	} else {
#  ifdef HAVE_VSYSLOG_H
		vsyslog(LOG_INFO, fmt, args);
#  else
#    ifdef HAVE_VSNPRINTF
		vsnprintf(str, 800, fmt, args);
#    else
#      ifdef HAVE_VPRINTF
		vsprintf(str, fmt, args);
#      endif
#    endif
		syslog(LOG_INFO, str);
#  endif
	}
#endif

	va_end(args);
}
