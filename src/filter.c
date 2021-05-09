/* tinyproxy - A fast light-weight HTTP proxy
 * Copyright (C) 1999 George Talusan <gstalusan@uwaterloo.ca>
 * Copyright (C) 2002 James E. Flemer <jflemer@acm.jhu.edu>
 * Copyright (C) 2002 Robert James Kaes <rjkaes@users.sourceforge.net>
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

/* A substring of the domain to be filtered goes into the file
 * pointed at by DEFAULT_FILTER.
 */

#include "main.h"

#include <regex.h>
#include <fnmatch.h>
#include "filter.h"
#include "heap.h"
#include "log.h"
#include "reqs.h"
#include "conf.h"
#include "sblist.h"

#define FILTER_BUFFER_LEN (512)

static int err;

struct filter_list {
        union {
                regex_t cpatb;
                char *pattern;
        } u;
};

static sblist *fl = NULL;
static int already_init = 0;

/*
 * Initializes a list of strings containing hosts/urls to be filtered
 */
void filter_init (void)
{
        FILE *fd;
        struct filter_list fe;
        char buf[FILTER_BUFFER_LEN];
        char *s, *start;
        int cflags, lineno = 0;

        if (fl || already_init) {
                return;
        }

        fd = fopen (config->filter, "r");
        if (!fd) {
                perror ("filter file");
                exit (EX_DATAERR);
        }

        cflags = REG_NEWLINE | REG_NOSUB;
        cflags |= (REG_EXTENDED * !!(config->filter_opts & FILTER_OPT_TYPE_ERE));
        cflags |= (REG_ICASE * !(config->filter_opts & FILTER_OPT_CASESENSITIVE));

        while (fgets (buf, FILTER_BUFFER_LEN, fd)) {
                ++lineno;
                /* skip leading whitespace */
                s = buf;
                while (*s && isspace ((unsigned char) *s))
                        s++;
                start = s;

                /*
                 * Remove any trailing white space and
                 * comments.
                 */
                while (*s) {
                        if (isspace ((unsigned char) *s))
                                break;
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
                s = start;

                /* skip blank lines and comments */
                if (*s == '\0')
                        continue;

                if (!fl) fl = sblist_new(sizeof(struct filter_list),
                                         4096/sizeof(struct filter_list));

                if (config->filter_opts & FILTER_OPT_TYPE_FNMATCH) {
                        fe.u.pattern = safestrdup(s);
                        if (!fe.u.pattern) goto oom;
                } else {

                        err = regcomp (&fe.u.cpatb, s, cflags);
                        if (err != 0) {
                                if (err == REG_ESPACE) goto oom;
                                fprintf (stderr,
                                         "Bad regex in %s: line %d - %s\n",
                                         config->filter, lineno, s);
                                exit (EX_DATAERR);
                        }
                }
                if (!sblist_add(fl, &fe)) {
                oom:;
                        fprintf (stderr,
                                 "out of memory parsing filter file %s: line %d\n",
                                 config->filter, lineno);
                        exit (EX_DATAERR);
                }
        }
        if (ferror (fd)) {
                perror ("fgets");
                exit (EX_DATAERR);
        }
        fclose (fd);

        already_init = 1;
}

/* unlink the list */
void filter_destroy (void)
{
        struct filter_list *p;
        size_t i;

        if (already_init) {
                if (fl) {
                        for (i = 0; i < sblist_getsize(fl); ++i) {
                                p = sblist_get(fl, i);
                                if (config->filter_opts & FILTER_OPT_TYPE_FNMATCH)
                                        safefree(p->u.pattern);
                                else
                                        regfree (&p->u.cpatb);
                        }
                        sblist_free(fl);
                }
                fl = NULL;
                already_init = 0;
        }
}

/**
 * reload the filter file if filtering is enabled
 */
void filter_reload (void)
{
        if (config->filter) {
                log_message (LOG_NOTICE, "Re-reading filter file.");
                filter_destroy ();
                filter_init ();
        }
}

/* Return 0 to allow, non-zero to block */
int filter_run (const char *str)
{
        struct filter_list *p;
        size_t i;
        int result;

        if (!fl || !already_init)
                goto COMMON_EXIT;

        for (i = 0; i < sblist_getsize(fl); ++i) {
                p = sblist_get(fl, i);
                if (config->filter_opts & FILTER_OPT_TYPE_FNMATCH)
                        result = fnmatch (p->u.pattern, str, 0);
                else
                        result =
                            regexec (&p->u.cpatb, str, (size_t) 0, (regmatch_t *) 0, 0);

                if (result == 0) {
                        if (!(config->filter_opts & FILTER_OPT_DEFAULT_DENY))
                                return 1;
                        else
                                return 0;
                }
        }

COMMON_EXIT:
        if (!(config->filter_opts & FILTER_OPT_DEFAULT_DENY))
                return 0;
        else
                return 1;
}
