/* $Id: filter.c,v 1.16.2.1 2003-10-16 21:19:09 rjkaes Exp $
 *
 * Copyright (c) 1999  George Talusan (gstalusan@uwaterloo.ca)
 * Copyright (c) 2002  James E. Flemer (jflemer@acm.jhu.edu)
 * Copyright (c) 2002  Robert James Kaes (rjkaes@flarenet.com)
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
#include "log.h"
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
static filter_policy_t default_policy = FILTER_DEFAULT_ALLOW;

/*
 * Initializes a linked list of strings containing hosts/urls to be filtered
 */
void
filter_init(void)
{
	FILE *fd;
	struct filter_list *p;
	char buf[FILTER_BUFFER_LEN];
	char *s;
	int cflags;

	if (!fl && !already_init) {
		fd = fopen(config.filter, "r");
		if (fd) {
			p = NULL;

			cflags = REG_NEWLINE | REG_NOSUB;
			if (config.filter_extended)
				cflags |= REG_EXTENDED;
			if (!config.filter_casesensitive)
				cflags |= REG_ICASE;

			while (fgets(buf, FILTER_BUFFER_LEN, fd)) {
				/*
				 * Remove any trailing white space and
				 * comments.
				 */
				s = buf;
				while (*s) {
					if (isspace((unsigned char)*s)) break;
					if (*s == '#') {
						/*
						 * If the '#' char is preceeded by
						 * an escape, it's not a comment
						 * string.
						 */
						if (s == buf || *(s - 1) != '\\')
							break;
					}
					++s;
				}
				*s = '\0';

				/* skip leading whitespace */
				s = buf;
				while (*s && isspace((unsigned char)*s))
					s++;

				/* skip blank lines and comments */
				if (*s == '\0')
					continue;

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

/* Return 0 to allow, non-zero to block */
int
filter_domain(const char *host)
{
	struct filter_list *p;
	int result;

	if (!fl || !already_init)
		goto COMMON_EXIT;

	for (p = fl; p; p = p->next) {
		result = regexec(p->cpat, host, (size_t) 0, (regmatch_t *) 0, 0);

		if (result == 0) {
			if (default_policy == FILTER_DEFAULT_ALLOW)
				return 1;
			else
				return 0;
		}
	}

  COMMON_EXIT:
	if (default_policy == FILTER_DEFAULT_ALLOW)
		return 0;
	else
		return 1;
}

/* returns 0 to allow, non-zero to block */
int
filter_url(const char *url)
{
	struct filter_list *p;
	int result;

	if (!fl || !already_init)
		goto COMMON_EXIT;

	for (p = fl; p; p = p->next) {
		result = regexec(p->cpat, url, (size_t) 0, (regmatch_t *) 0, 0);

		if (result == 0) {
			if (default_policy == FILTER_DEFAULT_ALLOW)
				return 1;
			else
				return 0;
		}
	}

  COMMON_EXIT:
	if (default_policy == FILTER_DEFAULT_ALLOW)
		return 0;
	else
		return 1;
}

/*
 * Set the default filtering policy
 */
void
filter_set_default_policy(filter_policy_t policy)
{
	default_policy = policy;
}
