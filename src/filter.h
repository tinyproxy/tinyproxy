/* tinyproxy - A fast light-weight HTTP proxy
 * Copyright (C) 1999 George Talusan <gstalusan@uwaterloo.ca>
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

/* See 'filter.c' for detailed information. */

#ifndef _TINYPROXY_FILTER_H_
#define _TINYPROXY_FILTER_H_

enum filter_options {
        FILTER_OPT_CASESENSITIVE	= 1 << 0,
        FILTER_OPT_URL			= 1 << 1,
        FILTER_OPT_DEFAULT_DENY		= 1 << 2,

        FILTER_OPT_TYPE_BRE		= 1 << 8,
        FILTER_OPT_TYPE_ERE		= 1 << 9,
        FILTER_OPT_TYPE_FNMATCH		= 1 << 10,
};

#define FILTER_TYPE_MASK \
    (FILTER_OPT_TYPE_BRE | FILTER_OPT_TYPE_ERE | FILTER_OPT_TYPE_FNMATCH)

extern void filter_init (void);
extern void filter_destroy (void);
extern void filter_reload (void);
extern int filter_run (const char *str);

#endif
