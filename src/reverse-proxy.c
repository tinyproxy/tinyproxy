/* tinyproxy - A fast light-weight HTTP proxy
 * Copyright (C) 1999-2005 Robert James Kaes <rjkaes@users.sourceforge.net>
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

/* Allow tinyproxy to be used as a reverse proxy. */

#include "main.h"
#include "reverse-proxy.h"

#include "conns.h"
#include "heap.h"
#include "html-error.h"
#include "log.h"
#include "conf.h"

/*
 * Add entry to the reversepath list
 */
void reversepath_add (const char *path, const char *url,
                      struct reversepath **reversepath_list)
{
        struct reversepath *reverse;

        if (url == NULL) {
                log_message (LOG_WARNING,
                             "Illegal reverse proxy rule: missing url");
                return;
        }

        if (!strstr (url, "://")) {
                log_message (LOG_WARNING,
                             "Skipping reverse proxy rule: '%s' is not a valid url",
                             url);
                return;
        }

        if (path && *path != '/') {
                log_message (LOG_WARNING,
                             "Skipping reverse proxy rule: path '%s' "
                             "doesn't start with a /", path);
                return;
        }

        reverse = (struct reversepath *) safemalloc (sizeof
                                                     (struct reversepath));
        if (!reverse) {
                log_message (LOG_ERR,
                             "Unable to allocate memory in reversepath_add()");
                return;
        }

        if (!path)
                reverse->path = safestrdup ("/");
        else
                reverse->path = safestrdup (path);

        reverse->url = safestrdup (url);

        reverse->next = *reversepath_list;
        *reversepath_list = reverse;

        log_message (LOG_INFO,
                     "Added reverse proxy rule: %s -> %s", reverse->path,
                     reverse->url);
}

/*
 * Check if a request url is in the reversepath list
 */
struct reversepath *reversepath_get (char *url, struct reversepath *reverse)
{
        while (reverse) {
                if (strstr (url, reverse->path) == url)
                        return reverse;

                reverse = reverse->next;
        }

        return NULL;
}

/**
 * Free a reversepath list
 */

void free_reversepath_list (struct reversepath *reverse)
{
        while (reverse) {
                struct reversepath *tmp = reverse;
                reverse = reverse->next;
                safefree (tmp->url);
                safefree (tmp->path);
                safefree (tmp);
        }
}

/*
 * Rewrite the URL for reverse proxying.
 */
char *reverse_rewrite_url (struct conn_s *connptr, hashmap_t hashofheaders,
                           char *url)
{
        char *rewrite_url = NULL;
        char *cookie = NULL;
        char *cookieval;
        struct reversepath *reverse = NULL;

        /* Reverse requests always start with a slash */
        if (*url == '/') {
                /* First try locating the reverse mapping by request url */
                reverse = reversepath_get (url, config.reversepath_list);
                if (reverse) {
                        rewrite_url = (char *)
                            safemalloc (strlen (url) + strlen (reverse->url) +
                                        1);
                        strcpy (rewrite_url, reverse->url);
                        strcat (rewrite_url, url + strlen (reverse->path));
                } else if (config.reversemagic
                           && hashmap_entry_by_key (hashofheaders,
                                                    "cookie",
                                                    (void **) &cookie) > 0) {

                        /* No match - try the magical tracking cookie next */
                        if ((cookieval = strstr (cookie, REVERSE_COOKIE "="))
                            && (reverse =
                                reversepath_get (cookieval +
                                                 strlen (REVERSE_COOKIE) + 1,
                                                 config.reversepath_list)))
                        {

                                rewrite_url = (char *) safemalloc
                                        (strlen (url) +
                                         strlen (reverse->url) +
                                         1);
                                strcpy (rewrite_url, reverse->url);
                                strcat (rewrite_url, url + 1);

                                log_message (LOG_INFO,
                                             "Magical tracking cookie says: %s",
                                             reverse->path);
                        }
                }
        }

        /* Forward proxy support off and no reverse path match found */
        if (config.reverseonly && !rewrite_url) {
                log_message (LOG_ERR, "Bad request");
                indicate_http_error (connptr, 400, "Bad Request",
                                     "detail",
                                     "Request has an invalid URL", "url", url,
                                     NULL);
                return NULL;
        }

        log_message (LOG_CONN, "Rewriting URL: %s -> %s", url, rewrite_url);

        /* Store reverse path so that the magical tracking cookie can be set */
        if (config.reversemagic && reverse)
                connptr->reversepath = safestrdup (reverse->path);

        return rewrite_url;
}
