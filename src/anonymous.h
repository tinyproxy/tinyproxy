/* $Id: anonymous.h,v 1.5 2001-08-26 21:07:27 rjkaes Exp $
 *
 * See 'anonymous.c' for a detailed description.
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

#ifndef _TINYPROXY_ANONYMOUS_H_
#define _TINYPROXY_ANONYMOUS_H_

#include "ternary.h"

extern short int is_anonymous_enabled(void);
extern int anonymous_search(char *s);
extern int anonymous_insert(char *s);

#endif
