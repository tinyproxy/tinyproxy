/* $Id: acl.h,v 1.1 2000-09-12 00:08:48 rjkaes Exp $
 *
 * See 'acl.c' for detailed information.
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

#ifndef _TINYPROXY_ACL_H_
#define _TINYPROXY_ACL_H_

typedef enum { ACL_ALLOW, ACL_DENY } acl_access_t;

extern int insert_acl(char *location, acl_access_t access_type);
extern int check_acl(int fd);

#endif
