/* $Id: htmlerror.c,v 1.6 2003-07-14 17:42:43 rjkaes Exp $
 * 
 * This file contains source code for the handling and display of
 * HTML error pages with variable substitution.
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

#include "tinyproxy.h"

#include "common.h"
#include "buffer.h"
#include "conns.h"
#include "heap.h"
#include "htmlerror.h"
#include "network.h"
#include "utils.h"

/*
 * Add an error number -> filename mapping to the errorpages list.
 */
int
add_new_errorpage(char *filepath, unsigned int errornum) {
	static int errorpage_count = 1;

	/* First, add space for another pointer to the errorpages array. */
	config.errorpages = saferealloc(config.errorpages, sizeof(struct error_pages_s *) * (errorpage_count + 1));
	if(!config.errorpages)
		return(-1);

	/* Allocate space for an actual structure */
	config.errorpages[errorpage_count - 1] = safemalloc(sizeof(struct error_pages_s));
	if(!config.errorpages[errorpage_count - 1])
		return(-1);

	/* Set values for errorpage structure. */
	config.errorpages[errorpage_count - 1]->errorpage_path = safestrdup(filepath);
	if(!config.errorpages[errorpage_count - 1]->errorpage_path)
		return(-1);

	config.errorpages[errorpage_count - 1]->errorpage_errnum = errornum;

	/* Set NULL to denote end of array */
	config.errorpages[errorpage_count] = NULL;

	errorpage_count++;
	return(0);
}

/*
 * Get the file appropriate for a given error.
 */
static char*
get_html_file(int errornum) {
	int i;

	if (!config.errorpages) return(config.errorpage_undef);

	for (i = 0; config.errorpages[i]; i++) {
		if (config.errorpages[i]->errorpage_errnum == errornum)
			return config.errorpages[i]->errorpage_path;
	}

	return(config.errorpage_undef);
}

/*
 * Look up the value for a variable.
 */
static char*
lookup_variable(struct conn_s *connptr, char *varname) {
	int i;

	for (i = 0; i != connptr->error_variable_count; i++) {
		if (!strcasecmp(connptr->error_variables[i]->error_key, varname))
			return connptr->error_variables[i]->error_val;
	}

	return(NULL);
}

#define HTML_BUFSIZE 4096

/*
 * Send an already-opened file to the client with variable substitution.
 */
int
send_html_file(FILE *infile, struct conn_s *connptr) {
	char inbuf[HTML_BUFSIZE], *varstart = NULL, *p;
	char *varval;
	int in_variable = 0, writeret;

	while(fgets(inbuf, HTML_BUFSIZE, infile) != NULL) {
		for (p = inbuf; *p; p++) {
			switch(*p) {
				case '}':
					if(in_variable) {
						*p = '\0';
						if(!(varval = lookup_variable(connptr, varstart)))
							varval = "(unknown)";
						writeret = write_message(connptr->client_fd, "%s",
												 varval);
						if(writeret) return(writeret);
						in_variable = 0;
					} else {
						writeret = write_message(connptr->client_fd, "%c", *p);
						if (writeret) return(writeret);
					}
					break;
				case '{':
					/* a {{ will print a single {.  If we are NOT
					 * already in a { variable, then proceed with
					 * setup.  If we ARE already in a { variable,
					 * this code will fallthrough to the code that
					 * just dumps a character to the client fd.
					 */
					 if(!in_variable) {
					 	varstart = p+1;
						in_variable++;
					} else
						in_variable = 0;
				default:
					if(!in_variable) {
						writeret = write_message(connptr->client_fd, "%c", 
												 *p);
						if(writeret) return(writeret);
					}
		
			}
		}
		in_variable = 0;
	}
	return(0);
}

int
send_http_headers(struct conn_s *connptr, int code, char *message) {
	char *headers = \
		"HTTP/1.0 %d %s\r\n" \
		"Server: %s/%s\r\n" \
		"Content-Type: text/html\r\n" \
		"Connection: close\r\n" \
		"\r\n";

	return(write_message(connptr->client_fd, headers,
		   				 code, message,
		   				 PACKAGE, VERSION));
}

/*
 * Display an error to the client.
 */
int
send_http_error_message(struct conn_s *connptr)
{
	char *error_file;
	FILE *infile;
	int ret;
	char *fallback_error = \
		"<html><head><title>%s</title></head>" \
		"<body><blockquote><i>%s %s</i><br>" \
		"The page you requested was unavailable. The error code is listed " \
		"below. In addition, the HTML file which has been configured as the " \
		"page to be displayed when an error of this type was unavailable, " \
		"with the error code %d (%s).  Please contact your administrator." \
		"<center>%s</center>" \
		"</body></html>" \
        "\r\n";

	send_http_headers(connptr, connptr->error_number, connptr->error_string);

	error_file = get_html_file(connptr->error_number);
	if(!(infile = fopen(error_file, "r"))) 
		return(write_message(connptr->client_fd, fallback_error,
							 connptr->error_string,
							 PACKAGE, VERSION,
							 errno, strerror(errno),
							 connptr->error_string));

	ret = send_html_file(infile, connptr);
	fclose(infile);
	return(ret);
}

/* 
 * Add a key -> value mapping for HTML file substitution.
 */
int 
add_error_variable(struct conn_s *connptr, char *key, char *val) 
{
	connptr->error_variable_count++;

	/* Add space for a new pointer to the error_variables structure. */
	if (!connptr->error_variables)
		connptr->error_variables = safemalloc(sizeof(struct error_variables_s *) * connptr->error_variable_count);
	else
		connptr->error_variables = saferealloc(connptr->error_variables, sizeof(struct error_variable_s *) * connptr->error_variable_count);

	if(!connptr->error_variables)
		return(-1);
	
	/* Allocate a new variable mapping structure. */
	connptr->error_variables[connptr->error_variable_count - 1] = safemalloc(sizeof(struct error_variable_s));
	if(!connptr->error_variables[connptr->error_variable_count - 1])
		return(-1);

	/* Set values for variable mapping. */
	connptr->error_variables[connptr->error_variable_count - 1]->error_key = safestrdup(key);
	connptr->error_variables[connptr->error_variable_count - 1]->error_val = safestrdup(val);
	if((!connptr->error_variables[connptr->error_variable_count - 1]->error_key)
	|| (!connptr->error_variables[connptr->error_variable_count - 1]->error_val))
	   	return(-1);

	return(0);
}

#define ADD_VAR_RET(x, y) if(y) { if(add_error_variable(connptr, x, y) == -1) return(-1); }

/*
 * Set some standard variables used by all HTML pages
 */
int
add_standard_vars(struct conn_s *connptr) {
	char timebuf[30];
	time_t global_time = time(NULL);

	strftime(timebuf, sizeof(timebuf), "%a, %d %b %Y %H:%M:%S GMT",
			 gmtime(&global_time));

	ADD_VAR_RET("request", connptr->request_line);
	ADD_VAR_RET("cause", connptr->error_string);
	ADD_VAR_RET("clientip", connptr->client_ip_addr);
	ADD_VAR_RET("clienthost", connptr->client_string_addr);
	ADD_VAR_RET("version", VERSION);
	ADD_VAR_RET("package", PACKAGE);
	ADD_VAR_RET("date", timebuf);
	return(0);
}

/*
 * Add the error information to the conn structure.
 */
int
indicate_http_error(struct conn_s* connptr, int number, char *message, ...)
{
	va_list ap;
	char *key, *val;

	va_start(ap, message);

	while((key = va_arg(ap, char *))) {
		val = va_arg(ap, char *);
		if(add_error_variable(connptr, key, val) == -1) {
			va_end(ap);
			return(-1);
		}
	}

	connptr->error_number = number;
	connptr->error_string = safestrdup(message);

	va_end(ap);

	return(add_standard_vars(connptr));
}
