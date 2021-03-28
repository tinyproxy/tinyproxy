/* tinyproxy - A fast light-weight HTTP proxy
 * Copyright (C) 2003 Steven Young <sdyoung@miranda.org>
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

/* See 'html-error.c' for detailed information. */

#ifndef TINYPROXY_HTML_ERROR_H
#define TINYPROXY_HTML_ERROR_H

/* Forward declaration */
struct conn_s;
struct config_s;

extern int add_new_errorpage (struct config_s *, char *filepath, unsigned int errornum);
extern int send_http_error_message (struct conn_s *connptr);
extern int indicate_http_error (struct conn_s *connptr, int number,
                                const char *message, ...);
extern int add_error_variable (struct conn_s *connptr, const char *key,
                               const char *val);
extern int send_html_file (FILE * infile, struct conn_s *connptr);
extern int send_http_headers (struct conn_s *connptr, int code,
                              const char *message, const char *extra);
extern int add_standard_vars (struct conn_s *connptr);

#endif /* !TINYPROXY_HTML_ERROR_H */
