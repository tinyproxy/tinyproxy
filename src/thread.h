/* $Id: thread.h,v 1.4 2002-01-25 00:01:45 rjkaes Exp $
 *
 * See 'thread.c' for more information.
 *
 * Copyright (C) 2000 Robert James Kaes (rjkaes@flarenet.com)
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

#ifndef _TINYPROXY_THREAD_H_
#define _TINYPROXY_THREAD_H_

typedef enum {
	THREAD_MAXCLIENTS,
	THREAD_MAXSPARESERVERS,
	THREAD_MINSPARESERVERS,
	THREAD_STARTSERVERS,
	THREAD_MAXREQUESTSPERCHILD
} thread_config_t;

extern short int thread_pool_create(void);
extern int thread_listening_sock(uint16_t port);
extern void thread_close_sock(void);
extern void thread_main_loop(void);
extern void thread_kill_threads(void);

extern short int thread_configure(thread_config_t type, unsigned int val);

#endif
