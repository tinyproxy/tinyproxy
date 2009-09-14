/* tinyproxy - A fast light-weight HTTP proxy
 * Copyright (C) 2003 Robert James Kaes <rjkaes@users.sourceforge.net>
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

/* See 'http-message.h' for detailed information. */

#include "common.h"
#include "heap.h"
#include "http-message.h"
#include "network.h"

/*
 * Package up an HTTP message into a nice little structure.  As you can
 * see this structure doesn't actually store any allocated strings;
 * therefore, the caller must free any memory referenced by this struct.
 * Also, the caller MUST NOT free the memory while the structure is
 * still in use---bad things would happen.
 */
struct http_message_s {
        /* Response string and code supplied on the HTTP status line */
        struct {
                const char *string;
                int code;
        } response;

        /*
         * A group of headers to be sent with this message.  Right now
         * the strings are referenced through pointers in an array.
         * I might change this to a vector in the future.
         */
        struct {
                const char **strings;
                unsigned int total;
                unsigned int used;
        } headers;

        /* Body of the message (most likely an HTML message) */
        struct {
                const char *text;
                size_t length;
        } body;
};

/*
 * Check if the HTTP message is validly formed.  This is the one odd-ball
 * function.  It returns 0 if the message is invalid; otherwise, a positive
 * number is returned.  Useful for if() tests and assert() tests.
 */
static int is_http_message_valid (http_message_t msg)
{
        if (msg == NULL)
                return 0;
        if (msg->headers.strings == NULL)
                return 0;
        if (msg->response.string == NULL)
                return 0;
        if (msg->response.code < 1 || msg->response.code > 999)
                return 0;

        return 1;
}

/* Initially allocate space for 128 headers */
#define NUMBER_OF_HEADERS 128

/*
 * Allocate a new http_message structure on the heap.
 * If memory could not be allocated, return a NULL.
 */
http_message_t
http_message_create (int response_code, const char *response_string)
{
        http_message_t msg;
        int ret;

        msg =
            (struct http_message_s *) safecalloc (1,
                                                  sizeof (struct
                                                          http_message_s));
        if (msg == NULL)
                return NULL;

        msg->headers.strings = (const char **) safecalloc (NUMBER_OF_HEADERS,
                                                           sizeof (char *));
        if (msg->headers.strings == NULL) {
                safefree (msg);
                return NULL;
        }

        msg->headers.total = NUMBER_OF_HEADERS;

        /* Store the HTTP response information in the structure */
        ret = http_message_set_response (msg, response_code, response_string);
        if (IS_HTTP_MSG_ERROR (ret)) {
                safefree (msg->headers.strings);
                safefree (msg);
                return NULL;
        }

        return msg;
}

/*
 * Free up the space associated with this HTTP message structure.
 * This DOES NOT free the pointers stored in this structure.  That memory
 * is the responsibility of the caller.
 */
int http_message_destroy (http_message_t msg)
{
        assert (msg != NULL);
        assert (msg->headers.strings != NULL);

        /* Check for valid arguments */
        if (msg == NULL)
                return -EFAULT;

        if (msg->headers.strings != NULL)
                safefree (msg->headers.strings);
        safefree (msg);
        return 0;
}

/*
 * Set the HTTP response information for this structure.  The response_string
 * must be a NUL ('\0') terminated C string.
 */
int
http_message_set_response (http_message_t msg,
                           int response_code, const char *response_string)
{
        /* Check for valid arguments */
        if (msg == NULL)
                return -EFAULT;
        if (response_code < 1 || response_code > 999)
                return -EINVAL;
        if (response_string == NULL)
                return -EINVAL;
        if (strlen (response_string) == 0)
                return -EINVAL;

        msg->response.code = response_code;
        msg->response.string = response_string;

        return 0;
}

/*
 * Set the HTTP message body.
 */
int http_message_set_body (http_message_t msg, const char *body, size_t len)
{
        /* Check for valid arguments */
        if (msg == NULL)
                return -EFAULT;
        if (body == NULL)
                return -EINVAL;
        if (len == 0)
                return -EINVAL;

        msg->body.text = body;
        msg->body.length = len;

        return 0;
}

/*
 * Add headers to the structure.
 */
int
http_message_add_headers (http_message_t msg, const char **headers,
                          unsigned int num_headers)
{
        const char **new_headers;
        unsigned int i;

        /* Check for valid arguments */
        if (msg == NULL)
                return -EFAULT;
        if (headers == NULL)
                return -EINVAL;

        /*
         * If the number of headers to add is greater than the space
         * available, reallocate the memory.
         */
        if (msg->headers.used + num_headers > msg->headers.total) {
                new_headers =
                    (const char **) safecalloc (msg->headers.total * 2,
                                                sizeof (char *));
                if (new_headers == NULL)
                        return -ENOMEM;

                /* Copy the array */
                for (i = 0; i != msg->headers.used; ++i)
                        new_headers[i] = msg->headers.strings[i];

                /* Remove the old array and replace it with the new array */
                safefree (msg->headers.strings);
                msg->headers.strings = new_headers;
                msg->headers.total *= 2;
        }

        /*
         * Add the new headers to the structure
         */
        for (i = 0; i != num_headers; ++i)
                msg->headers.strings[i + msg->headers.used] = headers[i];
        msg->headers.used += num_headers;

        return 0;
}

/*
 * Send the completed HTTP message via the supplied file descriptor.
 */
int http_message_send (http_message_t msg, int fd)
{
        char timebuf[30];
        time_t global_time;
        unsigned int i;

        assert (is_http_message_valid (msg));

        /* Check for valid arguments */
        if (msg == NULL)
                return -EFAULT;
        if (fd < 1)
                return -EBADF;
        if (!is_http_message_valid (msg))
                return -EINVAL;

        /* Write the response line */
        write_message (fd, "HTTP/1.0 %d %s\r\n",
                       msg->response.code, msg->response.string);

        /* Go through all the headers */
        for (i = 0; i != msg->headers.used; ++i)
                write_message (fd, "%s\r\n", msg->headers.strings[i]);

        /* Output the date */
        global_time = time (NULL);
        strftime (timebuf, sizeof (timebuf), "%a, %d %b %Y %H:%M:%S GMT",
                  gmtime (&global_time));
        write_message (fd, "Date: %s\r\n", timebuf);

        /* Output the content-length */
        write_message (fd, "Content-length: %u\r\n", msg->body.length);

        /* Write the separator between the headers and body */
        safe_write (fd, "\r\n", 2);

        /* If there's a body, send it! */
        if (msg->body.length > 0)
                safe_write (fd, msg->body.text, msg->body.length);

        return 0;
}
