/* $Id: filter.c,v 1.1.1.1 2000-02-16 17:32:18 sdyoung Exp $
 *
 * Copyright (c) 1999  George Talusan (gstalusan@uwaterloo.ca)
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

#ifdef HAVE_CONFIG_H
#include <defines.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "config.h"
#include "utils.h"
#include "tinyproxy.h"
#include "filter.h"

#include "regexp.h"
#include <sysexits.h>

static int err;

struct filter_list {
	struct filter_list *next;
	char *pat;
	regex_t *cpat;
};


static struct filter_list *fl = NULL;
static int already_init = 0;

/* initializes a linked list of strings containing hosts to be filtered */
void filter_init(void)
{
	FILE *fd;
	struct filter_list *p;
	char buf[255];
	char *s;

	if (!fl && !already_init) {
		fd = fopen(config.filter, "r");
		if (fd) {
			p = NULL;

			while (fgets(buf, 255, (FILE *) fd)) {
				s = buf;
				if (!p)	/* head of list */
					fl = p = (struct filter_list *)
					    xmalloc(sizeof
						    (struct filter_list));
				else {	/* next entry */
					p->next = (struct filter_list *)
					    xmalloc(sizeof
						    (struct filter_list));
					p = p->next;
				}
				/* initialize space to zero */
				memset(p, 0, sizeof(struct filter_list));

				/* replace first whitespace with \0 */
				while (*s++)
					if (isspace((int) *s))
						*s = '\0';

				p->pat = xstrdup(buf);
				p->cpat = xmalloc(sizeof(regex_t));
				if (
				    (err =
				     regcomp(p->cpat, p->pat,
					     REG_NEWLINE | REG_NOSUB)) != 0) {
					fprintf(stderr,
						"Bad regex in %s: %s\n",
						config.filter, p->pat);
					exit(EX_DATAERR);
				}
			}
			already_init = 1;
			fclose(fd);
		}
	}
}

/* unlink the list */
void filter_destroy(void)
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
int filter_host(char *host)
{
	struct filter_list *p;
	char *s, *port;
	int result;

	assert(host);

	if (!fl || !already_init)
		return (0);

	/* strip off the port number */
	s = xstrdup(host);
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
