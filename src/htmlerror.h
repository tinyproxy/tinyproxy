/* $Id: htmlerror.h,v 1.1 2003-03-13 21:25:04 rjkaes Exp $
 *
 * Contains header declarations for the HTML error functions in htmlerror.c
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
