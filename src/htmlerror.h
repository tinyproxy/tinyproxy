/* $Id: htmlerror.h,v 1.2 2003-03-14 22:45:59 rjkaes Exp $
 *
 * Contains header declarations for the HTML error functions in
 * htmlerror.c
 *
 * Copyright (C) 2003  Steven Young <sdyoung@well.com>
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

#ifndef TINYPROXY_HTMLERROR_H
#define TINYPROXY_HTMLERROR_H

/* Forward declaration */
struct conn_s;

extern int add_new_errorpage(char *filepath, unsigned int errornum);
extern int send_http_error_message(struct conn_s *connptr);
extern int indicate_http_error(struct conn_s *connptr, int number, char *message, ...);
extern int add_error_variable(struct conn_s *connptr, char *key, char *val);
extern int send_html_file(FILE *infile, struct conn_s *connptr);
extern int send_http_headers(struct conn_s *connptr, int code, char *message);
extern int add_standard_vars(struct conn_s *connptr);

#endif /* !TINYPROXY_HTMLERROR_H */
