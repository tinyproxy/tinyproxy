/* $Id: daemon.h,v 1.1 2002-05-23 04:39:32 rjkaes Exp $
 *
 * See 'daemon.c' for a detailed description.
 *
 * Copyright (C) 2002  Robert James Kaes (rjkaes@flarenet.com)
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

#ifndef TINYPROXY_DAEMON_H
#define TINYPROXY_DAEMON_H

typedef void signal_func(int);

/*
 * Pass a singal integer and a function to handle the signal.
 */
extern signal_func *set_signal_handler(int signo, signal_func *func);

/*
 * Make a program a daemon process
 */
extern void makedaemon(void);

#endif
