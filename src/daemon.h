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

/* See 'daemon.c' for detailed information. */

#ifndef TINYPROXY_DAEMON_H
#define TINYPROXY_DAEMON_H

typedef void signal_func (int);

/*
 * Pass a singal integer and a function to handle the signal.
 */
extern signal_func *set_signal_handler (int signo, signal_func * func);

/*
 * Make a program a daemon process
 */
extern void makedaemon (void);

#endif
