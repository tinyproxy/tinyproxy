/* $Id: buffer.h,v 1.5 2001-11-05 15:23:05 rjkaes Exp $
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

/*
 * This structure contains the total size of a buffer, plus pointers to the
 * head and tail of the buffer.
 */
struct buffer_s {
	struct bufline_s *head;	/* top of the buffer */
	struct bufline_s *tail;	/* bottom of the buffer */
	size_t size;	/* total size of the buffer */
};

/*
 * Return the size of a buffer (pass a pointer to a buffer_s structure.)
 */
#define BUFFER_SIZE(x) (x)->size

extern struct buffer_s *new_buffer(void);
extern void delete_buffer(struct buffer_s *buffptr);

extern ssize_t readbuff(int fd, struct buffer_s *buffptr);
extern ssize_t writebuff(int fd, struct buffer_s *buffptr);

#endif				/* __BUFFER_H_ */
