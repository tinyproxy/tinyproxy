/* $Id: stats.h,v 1.4 2001-11-22 00:31:10 rjkaes Exp $
 *
 * See 'stats.h' for a detailed description.
 *
 * Copyright (C) 2000  Robert James Kaes (rjkaes@flarenet.com)
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

#ifndef _TINYPROXY_STATS_H_
#define _TINYPROXY_STATS_H_

#include "conns.h"

/*
 * Various logable statistics
 */
typedef enum {
	STAT_BADCONN,		/* bad connection, for unknown reason */
	STAT_OPEN,		/* connection opened */
	STAT_CLOSE,		/* connection closed */
	STAT_REFUSE,		/* connection refused (to outside world) */
	STAT_DENIED		/* connection denied to tinyproxy itself */
} status_t;

/*
 * Public API to the statistics for tinyproxy
 */
extern void init_stats(void);
extern int showstats(struct conn_s *connptr);
extern int update_stats(status_t update_level);

#endif
