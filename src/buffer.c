/* $Id: buffer.c,v 1.1.1.1 2000-02-16 17:32:22 sdyoung Exp $
 *
 * The buffer used in each connection is a linked list of lines. As the lines
 * are read in and written out the buffer expands and contracts. Basically,
 * by using this method we can increase the buffer size dynamicly. However,
 * we have a hard limit of 64 KB for the size of the buffer. The buffer can be
 * thought of as a queue were we act on both the head and tail. The various
 * functions act on each end (the names are taken from what Perl uses to act on
 * the ends of an array. :)
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

#ifdef HAVE_CONFIG_H
#include <defines.h>
#endif

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>

#include "utils.h"
#include "log.h"
#include "tinyproxy.h"
#include "buffer.h"

/*
 * Take a string of data and a length and make a new line which can be added
 * to the buffer
 */
static struct bufline_s *makenewline(unsigned char *data, unsigned int length)
{
	struct bufline_s *newline;

	assert(data);
	assert(length > 0);

	if (!(newline = xmalloc(sizeof(struct bufline_s))))
		return NULL;

	newline->string = data;
	newline->next = NULL;
	newline->length = length;
	newline->pos = 0;

	return newline;
}

/*
 * Create a new buffer
 */
struct buffer_s *new_buffer(void)
{
	struct buffer_s *buffptr;

	if (!(buffptr = xmalloc(sizeof(struct buffer_s))))
		return NULL;

	buffptr->head = buffptr->tail = NULL;
	buffptr->size = 0;

	return buffptr;
}

/*
 * Delete all the lines in the buffer and the buffer itself
 */
void delete_buffer(struct buffer_s *buffptr)
{
	struct bufline_s *next;

	assert(buffptr);

	while (buffer_head(buffptr)) {
		next = buffer_head(buffptr)->next;
		free_line(buffer_head(buffptr));
		buffer_head(buffptr) = next;
	}
	buffer_head(buffptr) = NULL;
	buffer_tail(buffptr) = NULL;

	safefree(buffptr);
}

/*
 * Free the allocated buffer line
 */
void free_line(struct bufline_s *line)
{
	if (!line)
		return;

	if (line->string) {
		safefree(line->string);
	}

	safefree(line);
}

/*
 * Push a new line on to the end of the buffer
 */
struct bufline_s *push_buffer(struct buffer_s *buffptr, unsigned char *data,
			      unsigned int length)
{
	struct bufline_s *newline;

	assert(buffptr);
	assert(data);
	assert(length > 0);

	if (!(newline = makenewline(data, length)))
		return NULL;

	if (!buffer_head(buffptr) && !buffer_tail(buffptr))
		buffer_head(buffptr) = buffer_tail(buffptr) = newline;
	else
		buffer_tail(buffptr) = (buffer_tail(buffptr)->next = newline);

	buffptr->size += length;

	return newline;
}

/*
 * Pop a buffer line off the end of the buffer
 */
struct bufline_s *pop_buffer(struct buffer_s *buffptr)
{
	struct bufline_s *line, *newend;

	assert(buffptr);

	if (buffer_head(buffptr) == buffer_tail(buffptr)) {
		line = buffer_head(buffptr);
		buffer_head(buffptr) = buffer_tail(buffptr) = NULL;
		buffptr->size = 0;
		return line;
	}

	line = buffer_tail(buffptr);
	newend = buffer_head(buffptr);

	while (newend->next != line && newend->next)
		newend = newend->next;

	buffer_tail(buffptr) = newend;
	buffptr->size -= line->length;

	return line;
}

/*
 * Unshift a buffer line from the top of the buffer (meaning add a new line
 * to the top of the buffer)
 */
struct bufline_s *unshift_buffer(struct buffer_s *buffptr, unsigned char *data,
				 unsigned int length)
{
	struct bufline_s *newline;

	assert(buffptr);
	assert(data);
	assert(length > 0);

	if (!(newline = makenewline(data, length)))
		return NULL;

	if (!buffer_head(buffptr) && buffer_tail(buffptr)) {
		buffer_head(buffptr) = buffer_tail(buffptr) = newline;
	} else {
		newline->next = buffer_head(buffptr);
		buffer_head(buffptr) = newline;
		if (!buffer_tail(buffptr))
			buffer_tail(buffptr) = newline;
	}

	buffptr->size += length;

	return newline;
}

/*
 * Shift a line off the top of the buffer (remove the line from the top of
 * the buffer)
 */
struct bufline_s *shift_buffer(struct buffer_s *buffptr)
{
	struct bufline_s *line;

	assert(buffptr);

	if (!buffer_head(buffptr) && !buffer_tail(buffptr)) {
		line = buffer_head(buffptr);
		buffer_head(buffptr) = buffer_tail(buffptr) = NULL;
		buffptr->size = 0;
		return line;
	}

	line = buffer_head(buffptr);
	buffer_head(buffptr) = line->next;

	if (!buffer_head(buffptr))
		buffer_tail(buffptr) = NULL;

	buffptr->size -= line->length;

	return line;
}

/*
 * Reads the bytes from the socket, and adds them to the buffer.
 * Takes a connection and returns the number of bytes read.
 */
int readbuff(int fd, struct buffer_s *buffptr)
{
	int bytesin;
	unsigned char inbuf[BUFFER];
	unsigned char *buffer;

	assert(fd >= 0);
	assert(buffptr);

	bytesin = recv(fd, inbuf, BUFFER, 0);

	if (bytesin > 0) {
		if (!(buffer = xmalloc(bytesin)))
			return 0;

		memcpy(buffer, inbuf, bytesin);
		push_buffer(buffptr, buffer, bytesin);
		return bytesin;
	} else if (bytesin == 0) {
		/* connection was closed by client */
		return -1;
	} else {
		switch (errno) {
#ifdef EWOULDBLOCK
		case EWOULDBLOCK:
#else
#  ifdef EAGAIN
		case EAGAIN:
#  endif
#endif
		case EINTR:
			return 0;
		case ECONNRESET:
			return -1;
		default:
			log("ERROR readbuff: recv (%s)", strerror(errno));
			return -1;
		}
	}
}

/*
 * Write the bytes in the buffer to the socket.
 * Takes a connection and returns the number of bytes written.
 */
int writebuff(int fd, struct buffer_s *buffptr)
{
	int bytessent;
	struct bufline_s *line = buffer_head(buffptr);

	assert(fd >= 0);
	assert(buffptr);

	bytessent = send(fd, line->string + line->pos,
			 (size_t) (line->length - line->pos), 0);

	if (bytessent >= 0) {
		/* bytes sent, adjust buffer */
		line->pos += bytessent;
		if (line->pos == line->length)
			free_line(shift_buffer(buffptr));
		return bytessent;
	} else {
		switch (errno) {
#ifdef EWOULDBLOCK
		case EWOULDBLOCK:
#else
#  ifdef EAGAIN
		case EAGAIN:
#  endif
#endif
		case ENOBUFS:
		case EINTR:
		case ENOMEM:
			return 0;
		default:
			log("ERROR writebuff: send (%s)", strerror(errno));
			return -1;
		}
	}
}
