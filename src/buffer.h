/* $Id: buffer.h,v 1.8 2002-05-14 00:43:38 rjkaes Exp $
 *
 * See 'buffer.c' for a detailed description.
 *
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

#ifndef _TINYPROXY_BUFFER_H_
#define _TINYPROXY_BUFFER_H_

/* Forward declaration */
struct buffer_s;

extern struct buffer_s *new_buffer(void);
extern void delete_buffer(struct buffer_s *buffptr);
extern size_t buffer_size(struct buffer_s *buffptr);

/*
 * Add a new line to the given buffer. The data IS copied into the structure.
 */
extern int add_to_buffer(struct buffer_s *buffptr, unsigned char *data,
			 size_t length);

extern ssize_t read_buffer(int fd, struct buffer_s *buffptr);
extern ssize_t write_buffer(int fd, struct buffer_s *buffptr);

#endif				/* __BUFFER_H_ */
