/* tinyproxy - A fast light-weight HTTP proxy
 * Copyright (C) 1999, 2001 Robert James Kaes <rjkaes@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "main.h"

#include "buffer.h"
#include "heap.h"
#include "log.h"
#include <string.h>

#define BUFFER_CAPACITY (16 * 1024)

/*
* The buffer struct is allocated as a single block. The data area
* immediately follows the struct in memory.
*/
struct buffer_s {
        size_t size;
};

static unsigned char *get_data (struct buffer_s *b)
{
        return (unsigned char *)(b + 1);
}

struct buffer_s *new_buffer (void)
{
        struct buffer_s *b = safemalloc (sizeof (*b) + BUFFER_CAPACITY);
        if (b) b->size = 0;
        return b;
}

void delete_buffer (struct buffer_s *b)
{
        safefree (b);
}

size_t buffer_size (struct buffer_s *b)
{
        return b->size;
}

size_t buffer_space (struct buffer_s *b)
{
        return BUFFER_CAPACITY - b->size;
}

ssize_t read_buffer (int fd, struct buffer_s *b)
{
        size_t space = BUFFER_CAPACITY - b->size;
        ssize_t n;

        if (space == 0 || !b)
                return 0;

        n = read (fd, get_data (b) + b->size, space);

        if (n > 0) {
                b->size += n;
        } else if (n == 0) {
                n = -1; /* EOF */
        } else if (errno == EINTR || errno == EAGAIN) {
                n = 0; /* retry later */
        } else {
                log_message (LOG_ERR, "read_buffer: read() failed on fd %d: %s", fd, strerror (errno));
        }

        return n;
}

ssize_t write_buffer (int fd, struct buffer_s *b)
{
        ssize_t n;

        if (!b || b->size == 0)
                return 0;

        n = send (fd, get_data (b), b->size, MSG_NOSIGNAL);

        if (n > 0) {
                b->size -= n;
                /* Compact remaining data to the start of the buffer so the next
                * read() can always write contiguously to the end. */
                if (b->size > 0)
                        memmove (get_data (b), get_data (b) + n, b->size);
                return n;
        } else if (n == 0 || errno == EINTR || errno == EAGAIN) {
                return 0;
        } else {
                log_message (LOG_ERR, "write_buffer: send() error \"%s\" on fd %d", strerror (errno), fd);
                return -1;
        }
}
