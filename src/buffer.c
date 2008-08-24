/* $Id: buffer.c,v 1.22 2002-05-24 04:45:32 rjkaes Exp $
 *
 * The buffer used in each connection is a linked list of lines. As the lines
 * are read in and written out the buffer expands and contracts. Basically,
 * by using this method we can increase the buffer size dynamically. However,
 * we have a hard limit of 64 KB for the size of the buffer. The buffer can be
 * thought of as a queue were we act on both the head and tail. The various
 * functions act on each end (the names are taken from what Perl uses to act on
 * the ends of an array. :)
 *
 * Copyright (C) 1999,2001  Robert James Kaes (rjkaes@users.sourceforge.net)
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
#include "heap.h"
#include "log.h"

#define BUFFER_HEAD(x) (x)->head
#define BUFFER_TAIL(x) (x)->tail

struct bufline_s {
	unsigned char *string;	/* the actual string of data */
	struct bufline_s *next;	/* pointer to next in linked list */
	size_t length;		/* length of the string of data */
	size_t pos;		/* start sending from this offset */
};

/*
 * The buffer structure points to the beginning and end of the buffer list
 * (and includes the total size)
 */
struct buffer_s {
	struct bufline_s *head;	/* top of the buffer */
	struct bufline_s *tail;	/* bottom of the buffer */
	size_t size;		/* total size of the buffer */
};

/*
 * Take a string of data and a length and make a new line which can be added
 * to the buffer. The data IS copied, so make sure if you allocated your
 * data buffer on the heap, delete it because you now have TWO copies.
 */
static struct bufline_s *
makenewline(unsigned char *data, size_t length)
{
	struct bufline_s *newline;

	assert(data != NULL);
	assert(length > 0);

	if (!(newline = safemalloc(sizeof(struct bufline_s))))
		return NULL;

	if (!(newline->string = safemalloc(length))) {
		safefree(newline);
		return NULL;
	}

	memcpy(newline->string, data, length);

	newline->next = NULL;
	newline->length = length;

	/* Position our "read" pointer at the beginning of the data */
	newline->pos = 0;

	return newline;
}

/*
 * Free the allocated buffer line
 */
static void
free_line(struct bufline_s *line)
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
struct buffer_s *
new_buffer(void)
{
	struct buffer_s *buffptr;

	if (!(buffptr = safemalloc(sizeof(struct buffer_s))))
		return NULL;

	/*
	 * Since the buffer is initially empty, set the HEAD and TAIL
	 * pointers to NULL since they can't possibly point anywhere at the
	 * moment.
	 */
	BUFFER_HEAD(buffptr) = BUFFER_TAIL(buffptr) = NULL;
	buffptr->size = 0;

	return buffptr;
}

/*
 * Delete all the lines in the buffer and the buffer itself
 */
void
delete_buffer(struct buffer_s *buffptr)
{
	struct bufline_s *next;

	assert(buffptr != NULL);

	while (BUFFER_HEAD(buffptr)) {
		next = BUFFER_HEAD(buffptr)->next;
		free_line(BUFFER_HEAD(buffptr));
		BUFFER_HEAD(buffptr) = next;
	}

	safefree(buffptr);
}

/*
 * Return the current size of the buffer.
 */
size_t buffer_size(struct buffer_s *buffptr)
{
	return buffptr->size;
}

/*
 * Push a new line on to the end of the buffer.
 */
int
add_to_buffer(struct buffer_s *buffptr, unsigned char *data, size_t length)
{
	struct bufline_s *newline;

	assert(buffptr != NULL);
	assert(data != NULL);
	assert(length > 0);

	/*
	 * Sanity check here. A buffer with a non-NULL head pointer must
	 * have a size greater than zero, and vice-versa.
	 */
	if (BUFFER_HEAD(buffptr) == NULL)
		assert(buffptr->size == 0);
	else
		assert(buffptr->size > 0);

	/*
	 * Make a new line so we can add it to the buffer.
	 */
	if (!(newline = makenewline(data, length)))
		return -1;

	if (buffptr->size == 0)
		BUFFER_HEAD(buffptr) = BUFFER_TAIL(buffptr) = newline;
	else {
		BUFFER_TAIL(buffptr)->next = newline;
		BUFFER_TAIL(buffptr) = newline;
	}

	buffptr->size += length;

	return 0;
}

/*
 * Remove the first line from the top of the buffer
 */
static struct bufline_s *
remove_from_buffer(struct buffer_s *buffptr)
{
	struct bufline_s *line;

	assert(buffptr != NULL);
	assert(BUFFER_HEAD(buffptr) != NULL);

	line = BUFFER_HEAD(buffptr);
	BUFFER_HEAD(buffptr) = line->next;

	buffptr->size -= line->length;

	return line;
}

/*
 * Reads the bytes from the socket, and adds them to the buffer.
 * Takes a connection and returns the number of bytes read.
 */
#define READ_BUFFER_SIZE (1024 * 2)
ssize_t
read_buffer(int fd, struct buffer_s * buffptr)
{
	ssize_t bytesin;
	unsigned char *buffer;

	assert(fd >= 0);
	assert(buffptr != NULL);

	/*
	 * Don't allow the buffer to grow larger than MAXBUFFSIZE
	 */
	if (buffptr->size >= MAXBUFFSIZE)
		return 0;

	buffer = safemalloc(READ_BUFFER_SIZE);
	if (!buffer) {
		return -ENOMEM;
	}

	bytesin = read(fd, buffer, READ_BUFFER_SIZE);

	if (bytesin > 0) {
		if (add_to_buffer(buffptr, buffer, bytesin) < 0) {
			log_message(LOG_ERR,
				    "readbuff: add_to_buffer() error.");
			bytesin = -1;
		}
	} else {
		if (bytesin == 0) {
			/* connection was closed by client */
			bytesin = -1;
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
				bytesin = 0;
				break;
			default:
				log_message(LOG_ERR,
					    "readbuff: recv() error \"%s\" on file descriptor %d",
					    strerror(errno), fd);
				bytesin = -1;
				break;
			}
		}
	}

	safefree(buffer);
	return bytesin;
}

/*
 * Write the bytes in the buffer to the socket.
 * Takes a connection and returns the number of bytes written.
 */
ssize_t
write_buffer(int fd, struct buffer_s * buffptr)
{
	ssize_t bytessent;
	struct bufline_s *line;

	assert(fd >= 0);
	assert(buffptr != NULL);

	if (buffptr->size == 0)
		return 0;

	/* Sanity check. It would be bad to be using a NULL pointer! */
	assert(BUFFER_HEAD(buffptr) != NULL);
	line = BUFFER_HEAD(buffptr);

	bytessent =
	    send(fd, line->string + line->pos, line->length - line->pos, MSG_NOSIGNAL);

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
				    "writebuff: write() error [NOBUFS/NOMEM] \"%s\" on file descriptor %d",
				    strerror(errno), fd);
			return 0;
		default:
			log_message(LOG_ERR,
				    "writebuff: write() error \"%s\" on file descriptor %d",
				    strerror(errno), fd);
			return -1;
		}
	}
}
