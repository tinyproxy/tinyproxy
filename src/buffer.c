/* $Id: buffer.c,v 1.4 2001-05-23 18:01:23 rjkaes Exp $
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
	unsigned int length;	/* length of the string of data */
	unsigned int pos;	/* start sending from this offset */
};

struct buffer_s {
	struct bufline_s *head;	/* top of the buffer */
	struct bufline_s *tail;	/* bottom of the buffer */
	unsigned int size;	/* total size of the buffer */
};

/*
 * Return the size of the supplied buffer.
 */
unsigned int buffer_size(struct buffer_s *buffptr)
{
	assert(buffptr != NULL);

	return buffptr->size;
}

/*
 * Take a string of data and a length and make a new line which can be added
 * to the buffer
 */
static struct bufline_s *makenewline(unsigned char *data, unsigned int length)
{
	struct bufline_s *newline;

	assert(data != NULL);

	if (!(newline = malloc(sizeof(struct bufline_s))))
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

	if (line->string) {
		safefree(line->string);
	}

	safefree(line);
}

/*
 * Create a new buffer
 */
struct buffer_s *new_buffer(void)
{
	struct buffer_s *buffptr;

	if (!(buffptr = malloc(sizeof(struct buffer_s))))
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
			 unsigned int length)
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
int readbuff(int fd, struct buffer_s *buffptr)
{
	int bytesin;
	unsigned char inbuf[MAXBUFFSIZE];
	unsigned char *buffer;

	assert(fd >= 0);
	assert(buffptr != NULL);

	if (buffer_size(buffptr) >= MAXBUFFSIZE)
		return 0;

	bytesin = read(fd, inbuf, MAXBUFFSIZE - buffer_size(buffptr));

	if (bytesin > 0) {
		if (!(buffer = malloc(bytesin))) {
			log(LOG_CRIT, "Could not allocate memory in readbuff() [%s:%d]", __FILE__, __LINE__);
			return 0;
		}

		memcpy(buffer, inbuf, bytesin);
		if (add_to_buffer(buffptr, buffer, bytesin) < 0) {
			return -1;
		}

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
		default:
			log(LOG_ERR, "readbuff: recv (%s)", strerror(errno));
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
			log(LOG_ERR, "writebuff: send [NOBUFS/NOMEM] %s", strerror(errno));
			return 0;
		default:
			log(LOG_ERR, "writebuff: send (%s)", strerror(errno));
			return -1;
		}
	}
}
