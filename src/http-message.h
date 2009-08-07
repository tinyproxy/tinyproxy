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

/* HTTP Message API
 * ----------------
 * The idea behind this application programming interface (API) is to
 * represent an HTTP response message as a concrete entity.  The API
 * functions allow the message to be built up systematically before
 * transmission to a connected socket.
 *
 * The order of the functions in your program would look something like
 * this:
 *   http_message_create()
 *   http_message_set_response()
 *   http_message_set_body() [optional if no body is required]
 *   http_message_add_headers() [optional if no additional headers are used]
 *   http_message_send()
 *   http_message_destroy()
 *
 * NOTE: No user data is stored in the http_message_t type; therefore,
 * do not delete strings referenced by the http_message_t object
 * before you call http_message_destroy().  By not copying data, the
 * API functions are faster, but you must take greater care.
 *
 * (Side note: be _very_ careful when using stack allocated memory with
 * this API.  Bad things will happen if you try to pass the
 * http_message_t out of the calling function since the stack
 * allocated memory referenced by the http_message_t will no long
 * exist.)
 */

#ifndef _TINYPROXY_HTTP_MESSAGE_H_
#define _TINYPROXY_HTTP_MESSAGE_H_

/* Use the "http_message_t" as a cookie or handle to the structure. */
typedef struct http_message_s *http_message_t;

/*
 * Macro to test if an error occurred with the API.  All the HTTP message
 * functions will return 0 if no error occurred, or a negative number if
 * there was a problem.
 */
#define IS_HTTP_MSG_ERROR(x) (x < 0)

/* Initialize the internal structure of the HTTP message */
extern http_message_t http_message_create (int response_code,
                                           const char *response_string);

/* Free up an _internal_ resources */
extern int http_message_destroy (http_message_t msg);

/*
 * Send an HTTP message via the supplied file descriptor.  This function
 * will add the "Date" header before it's sent.
 */
extern int http_message_send (http_message_t msg, int fd);

/*
 * Change the internal state of the HTTP message.  Either set the
 * body of the message, update the response information, or
 * add a new set of headers.
 */
extern int http_message_set_body (http_message_t msg,
                                  const char *body, size_t len);
extern int http_message_set_response (http_message_t msg,
                                      int response_code,
                                      const char *response_string);

/*
 * Set the headers for this HTTP message.  Each string must be NUL ('\0')
 * terminated, but DO NOT include any carriage returns (CR) or
 * line-feeds (LF) since they will be included when the http_message is
 * sent.
 */
extern int http_message_add_headers (http_message_t msg,
                                     const char **headers,
                                     unsigned int num_headers);

#endif /* _TINYPROXY_HTTP_MESSAGE_H_ */
