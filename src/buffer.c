/* $Id: buffer.c,v 1.11 2001-09-15 21:24:18 rjkaes Exp $
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

#include "tinyproxy.h"

#include "buffer.h"
#include "log.h"
#include "utils.h"

#define BUFFER_HEAD(x) (((struct buffer_s *)(x))->head)
#define BUFFER_TAIL(x) (((struct buffer_s *)(x))->tail)

struct bufline_s {
	unsigned char *string;	/* the actual string of data */
	struct bufline_s *next;	/* pointer to next in linked list */
	size_t length;	/* length of the string of data */
	size_t pos;	/* start sending from this offset */
};

struct buffer_s {
	struct bufline_s *head;	/* top of the buffer */
	struct bufline_s *tail;	/* bottom of the buffer */
	size_t size;	/* total size of the buffer */
};

/*
 * Return the size of the supplied buffer.
 */
size_t buffer_size(struct buffer_s *buffptr)
{
	assert(buffptr != NULL);

	return buffptr->size;
}

/*
 * Take a string of data and a length and make a new line which can be added
 * to the buffer
 */
static struct bufline_s *makenewline(unsigned char *data, size_t length)
{
	struct bufline_s *newline;

	assert(data != NULL);

	if (!(newline = safemalloc(sizeof(struct bufline_s))))
		return NULL;

	newline->string = data;
	newline->next = NULL;
	newline->length = length;
	newline->pos = 0;

	return newline;
}

/*
 * Free the allocated buffer line
 */
static void free_line(struct bufline_s *line)
{
	assert(line != NULL);

	if (!line)
		return;

	if (line->string)
		safefree(line->string);

	safefree(line);
}

/*
 * Create a new buffer
 */
struct buffer_s *new_buffer(void)
{
	struct buffer_s *buffptr;

	if (!(buffptr = safemalloc(sizeof(struct buffer_s))))
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

	assert(buffptr != NULL);

	while (BUFFER_HEAD(buffptr)) {
		next = BUFFER_HEAD(buffptr)->next;
		free_line(BUFFER_HEAD(buffptr));
		BUFFER_HEAD(buffptr) = next;
	}
	BUFFER_HEAD(buffptr) = NULL;
	BUFFER_TAIL(buffptr) = NULL;

	safefree(buffptr);
}

/*
 * Push a new line on to the end of the buffer
 */
static int add_to_buffer(struct buffer_s *buffptr, unsigned char *data,
			 size_t length)
{
	struct bufline_s *newline;

	assert(buffptr != NULL);
	assert(data != NULL);

	if (!(newline = makenewline(data, length)))
		return -1;

	if (!BUFFER_HEAD(buffptr) && !BUFFER_TAIL(buffptr))
		BUFFER_HEAD(buffptr) = BUFFER_TAIL(buffptr) = newline;
	else
		BUFFER_TAIL(buffptr) = (BUFFER_TAIL(buffptr)->next = newline);

	buffptr->size += length;

	return 0;
}

/*
 * Shift a line off the top of the buffer (remove the line from the top of
 * the buffer)
 */
static struct bufline_s *remove_from_buffer(struct buffer_s *buffptr)
{
	struct bufline_s *line;

	assert(buffptr != NULL);

	if (!BUFFER_HEAD(buffptr) && !BUFFER_TAIL(buffptr)) {
		line = BUFFER_HEAD(buffptr);
		BUFFER_HEAD(buffptr) = BUFFER_TAIL(buffptr) = NULL;
		buffptr->size = 0;
		return line;
	}

	line = BUFFER_HEAD(buffptr);
	BUFFER_HEAD(buffptr) = line->next;

	if (!BUFFER_HEAD(buffptr))
		BUFFER_TAIL(buffptr) = NULL;

	buffptr->size -= line->length;

	return line;
}

/*
 * Reads the bytes from the socket, and adds them to the buffer.
 * Takes a connection and returns the number of bytes read.
 */
ssize_t readbuff(int fd, struct buffer_s *buffptr)
{
	ssize_t bytesin;
	unsigned char *buffer;
	unsigned char *newbuffer;

	assert(fd >= 0);
	assert(buffptr != NULL);

	if (buffer_size(buffptr) >= MAXBUFFSIZE)
		return 0;

	buffer = safemalloc(MAXBUFFSIZE);
	if (!buffer)
		return 0;

	bytesin = read(fd, buffer, MAXBUFFSIZE - buffer_size(buffptr) - 1);

	if (bytesin > 0) {
		newbuffer = saferealloc(buffer, bytesin);
		if (!newbuffer) {
			safefree(buffer);
			return 0;
		}

		if (add_to_buffer(buffptr, newbuffer, bytesin) < 0) {
			return -1;
		}

		return bytesin;
	} else if (bytesin == 0) {
		/* connection was closed by client */
		safefree(buffer);
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
			safefree(buffer);
			return 0;
		default:
			log_message(LOG_ERR, "recv error (fd %d: %s:%d) in 'readbuff'.",
				    fd, strerror(errno), errno);
			safefree(buffer);
			return -1;
		}
	}
}

/*
 * Write the bytes in the buffer to the socket.
 * Takes a connection and returns the number of bytes written.
 */
ssize_t writebuff(int fd, struct buffer_s *buffptr)
{
	ssize_t bytessent;
	struct bufline_s *line;

	assert(fd >= 0);
	assert(buffptr != NULL);

	line = BUFFER_HEAD(buffptr);

	if (buffer_size(buffptr) <= 0)
		return 0;

	bytessent = write(fd, line->string + line->pos, line->length - line->pos);

	if (bytessent >= 0) {
		/* bytes sent, adjust buffer */
		line->pos += bytessent;
		if (line->pos == line->length)
			free_line(remove_from_buffer(buffptr));
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
		case EINTR:
			return 0;
		case ENOBUFS:
		case ENOMEM:
			log_message(LOG_ERR,
				    "write error [NOBUFS/NOMEM] (%s) in 'writebuff'.",
				    strerror(errno));
			return 0;
		default:
			log_message(LOG_ERR,
				    "write error (fd %d: %s:%d) in 'writebuff'.",
				    fd, strerror(errno), errno);
			return -1;
		}
	}
}
