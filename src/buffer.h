/* $Id: buffer.h,v 1.1.1.1 2000-02-16 17:32:22 sdyoung Exp $
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

#ifndef __BUFFER_H_
#define __BUFFER_H_	1

#define MAXBUFFSIZE 20480

struct bufline_s {
	unsigned char *string;	/* the actual string of data */
	struct bufline_s *next;	/* pointer to next in linked list */
	unsigned int length;	/* length of the string of data */
	unsigned int pos;	/* start sending from this offset */
};

struct buffer_s {
	struct bufline_s *head;	/* top of the buffer */
	struct bufline_s *tail;	/* bottom of the buffer */
	unsigned long int size;	/* total size of the buffer */
};

#define buffer_head(x) (((struct buffer_s *)(x))->head)
#define buffer_tail(x) (((struct buffer_s *)(x))->tail)
#define buffer_size(x) (((struct buffer_s *)(x))->size)

/* Create and delete buffers */
extern struct buffer_s *new_buffer(void);
extern void delete_buffer(struct buffer_s *buffptr);

/* Operate on the end of the list */
extern struct bufline_s *push_buffer(struct buffer_s *buffptr,
				     unsigned char *data, unsigned int length);
extern struct bufline_s *pop_buffer(struct buffer_s *buffptr);

/* Operate on the head of the list */
extern struct bufline_s *unshift_buffer(struct buffer_s *buffptr,
					unsigned char *data,
					unsigned int length);
extern struct bufline_s *shift_buffer(struct buffer_s *buffptr);

/* Free a buffer line */
extern void free_line(struct bufline_s *line);

/* Read/Write the buffer to a socket */
extern int readbuff(int fd, struct buffer_s *buffptr);
extern int writebuff(int fd, struct buffer_s *buffptr);

#endif				/* __BUFFER_H_ */
