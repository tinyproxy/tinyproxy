/* $Id: filter.c,v 1.11 2002-05-27 01:56:22 rjkaes Exp $
 *
 * Copyright (c) 1999  George Talusan (gstalusan@uwaterloo.ca)
 * Copyright (c) 2002  James E. Flemer (jflemer@acm.jhu.edu)
 *
 * A substring of the domain to be filtered goes into the file
 * pointed at by DEFAULT_FILTER.
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

#include "filter.h"
#include "heap.h"
#include "regexp.h"
#include "reqs.h"

#define FILTER_BUFFER_LEN (512)

static int err;

struct filter_list {
	struct filter_list *next;
	char *pat;
	regex_t *cpat;
};

static struct filter_list *fl = NULL;
static int already_init = 0;

/*
 * Initializes a linked list of strings containing hosts/urls to be filtered
 */
void
filter_init(void)
{
	FILE *fd;
	struct filter_list *p;
	char buf[FILTER_BUFFER_LEN];
	char *s, *t;
	int cflags;

	if (!fl && !already_init) {
		fd = fopen(config.filter, "r");
		if (fd) {
			p = NULL;

			cflags = REG_NEWLINE | REG_NOSUB;
			if (config.filter_extended)
				cflags |= REG_EXTENDED;

			while (fgets(buf, FILTER_BUFFER_LEN, fd)) {
				s = buf;
				if (!p)	/* head of list */
					fl = p =
					    safecalloc(1,
						       sizeof(struct
							      filter_list));
				else {	/* next entry */
					p->next =
					    safecalloc(1,
						       sizeof(struct
							      filter_list));
					p = p->next;
				}

				/* strip trailing whitespace & comments */
				t = s;
				while (*s && *s != '#') {
					if (!isspace((unsigned char)*(s++)))
						t = s;
				}
				*t = '\0';

				/* skip leading whitespace */
				s = buf;
				while (*s && isspace((unsigned char)*s))
					s++;

				/* skip blank lines and comments */
				if (*s == '\0')
					continue;

				p->pat = safestrdup(s);
				p->cpat = safemalloc(sizeof(regex_t));
				if ((err = regcomp(p->cpat, p->pat, cflags)) != 0) {
					fprintf(stderr, "Bad regex in %s: %s\n",
						config.filter, p->pat);
					exit(EX_DATAERR);
				}
			}
			if (ferror(fd)) {
				perror("fgets");
				exit(EX_DATAERR);
			}
			fclose(fd);

			already_init = 1;
		}
	}
}

/* unlink the list */
void
filter_destroy(void)
{
	struct filter_list *p, *q;

	if (already_init) {
		for (p = q = fl; p; p = q) {
			regfree(p->cpat);
			safefree(p->cpat);
			safefree(p->pat);
			q = p->next;
			safefree(p);
		}
		fl = NULL;
		already_init = 0;
	}
}

/* returns 0 if host is not an element of filter list, non-zero otherwise */
int
filter_domain(const char *host)
{
	struct filter_list *p;
	char *s, *port;
	int result;

	if (!fl || !already_init)
		return (0);

	/* strip off the port number */
	s = safestrdup(host);
	port = strchr(s, ':');
	if (port)
		*port = '\0';

	result = 0;

	for (p = fl; p; p = p->next) {
		result = !regexec(p->cpat, s, (size_t) 0, (regmatch_t *) 0, 0);

		if (result)
			break;
	}
	safefree(s);
	return (result);
}

/* returns 0 if url is not an element of filter list, non-zero otherwise */
int
filter_url(const char *url)
{
	struct filter_list *p;

	if (!fl || !already_init)
		return (0);

	for (p = fl; p; p = p->next) {
		if (!regexec(p->cpat, url, (size_t) 0, (regmatch_t *) 0, 0)) {
			return 1;
		}
	}
	return 0;
}
