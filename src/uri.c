/* $Id: uri.c,v 1.5 2001-09-07 04:20:45 rjkaes Exp $
 *
 * This borrows the REGEX from RFC2396 to split a URI string into the five
 * primary components. The components are:
 *	scheme		the uri method (like "http", "ftp", "gopher")
 *	authority	the domain and optional ":" port
 *	path		path to the document/resource
 *	query		an optional query (separated with a "?")
 *	fragment	an optional fragement (separated with a "#")
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

#include "log.h"
#include "regexp.h"
#include "uri.h"
#include "utils.h"

#define NMATCH 10

#define URIPATTERN "^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?"

#define SCHEME        2
#define AUTHORITY     4
#define PATH          5
#define QUERY_MARK    6
#define QUERY         7
#define FRAGMENT_MARK 8
#define FRAGMENT      9

static int extract_uri(regmatch_t pmatch[], const char *buffer, char **section,
		       int substring)
{
	size_t len = pmatch[substring].rm_eo - pmatch[substring].rm_so;
	if ((*section = malloc(len + 1)) == NULL) {
		log_message(LOG_ERR, "Could not allocate memory for extracting URI.");
		return -1;
	}

	memset(*section, '\0', len + 1);
	memcpy(*section, buffer + pmatch[substring].rm_so, len);

	return 0;
}

void free_uri(URI * uri)
{
	safefree(uri->scheme);
	safefree(uri->authority);
	safefree(uri->path);
	safefree(uri->query);
	safefree(uri->fragment);
	safefree(uri);
}

URI *explode_uri(const char *string)
{
	URI *uri;
	regmatch_t pmatch[NMATCH];
	regex_t preg;

	if (!(uri = malloc(sizeof(URI))))
		return NULL;
	memset(uri, 0, sizeof(URI));

	if (regcomp(&preg, URIPATTERN, REG_EXTENDED) != 0) {
		log_message(LOG_ERR, "Regular Expression compiler error.");
		goto ERROR_EXIT;
	}

	if (regexec(&preg, string, NMATCH, pmatch, 0) != 0) {
		log_message(LOG_ERR, "Regular Expression search error.");
		goto ERROR_EXIT;
	}

	regfree(&preg);

	if (pmatch[SCHEME].rm_so != -1) {
		if (extract_uri(pmatch, string, &uri->scheme, SCHEME) < 0)
			goto ERROR_EXIT;
	}

	if (pmatch[AUTHORITY].rm_so != -1) {
		if (extract_uri(pmatch, string, &uri->authority, AUTHORITY) <
		    0) goto ERROR_EXIT;
	}

	if (pmatch[PATH].rm_so != -1) {
		if (extract_uri(pmatch, string, &uri->path, PATH) < 0)
			goto ERROR_EXIT;
	}

	if (pmatch[QUERY_MARK].rm_so != -1) {
		if (extract_uri(pmatch, string, &uri->query, QUERY) < 0)
			goto ERROR_EXIT;
	}

	if (pmatch[FRAGMENT_MARK].rm_so != -1) {
		if (extract_uri(pmatch, string, &uri->fragment, FRAGMENT) < 0)
			goto ERROR_EXIT;
	}

	return uri;

      ERROR_EXIT:
	free_uri(uri);
	return NULL;
}
