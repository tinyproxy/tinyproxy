/* tinyproxy - A fast light-weight HTTP proxy
 * Copyright (C) 2000 Robert James Kaes <rjkaes@users.sourceforge.net>
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

/* See 'stats.c' for detailed information. */

#ifndef _TINYPROXY_STATS_H_
#define _TINYPROXY_STATS_H_

#include "conns.h"

/*
 * Various logable statistics
 */
typedef enum {
        STAT_BADCONN,           /* bad connection, for unknown reason */
        STAT_OPEN,              /* connection opened */
        STAT_CLOSE,             /* connection closed */
        STAT_REFUSE,            /* connection refused (to outside world) */
        STAT_DENIED             /* connection denied to tinyproxy itself */
} status_t;

/*
 * Public API to the statistics for tinyproxy
 */
extern void init_stats (void);
extern int showstats (struct conn_s *connptr);
extern int update_stats (status_t update_level);

#endif
