/* $Id: utils.h,v 1.23 2003-03-13 21:34:37 rjkaes Exp $
 *
 * See 'utils.h' for a detailed description.
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
 */

#ifndef TINYPROXY_UTILS_H
#define TINYPROXY_UTILS_H

/*
 * Forward declaration.
 */
struct conn_s;

extern int send_http_message(struct conn_s *connptr, int http_code,
			     const char *error_title, const char *message);

extern int pidfile_create(const char *path);
extern int create_file_safely(const char *filename, unsigned int truncate_file);

#endif
