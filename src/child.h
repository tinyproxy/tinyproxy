/* $Id: child.h,v 1.2 2004-08-10 21:24:23 rjkaes Exp $
 *
 * See 'child.c' for more information.
 *
 * Copyright (C) 2002 Robert James Kaes (rjkaes@flarenet.com)
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

#ifndef TINYPROXY_CHILD_H
#define TINYPROXY_CHILD_H

typedef enum {
	CHILD_MAXCLIENTS,
	CHILD_MAXSPARESERVERS,
	CHILD_MINSPARESERVERS,
	CHILD_STARTSERVERS,
	CHILD_MAXREQUESTSPERCHILD
} child_config_t;

extern short int child_pool_create(void);
extern int child_listening_sock(uint16_t port);
extern void child_close_sock(void);
extern void child_main_loop(void);
extern void child_kill_children(void);

extern short int child_configure(child_config_t type, int val);

#endif
