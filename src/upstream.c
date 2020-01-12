/* tinyproxy - A fast light-weight HTTP proxy
 * Copyright (C) 1998 Steven Young <sdyoung@miranda.org>
 * Copyright (C) 1999-2005 Robert James Kaes <rjkaes@users.sourceforge.net>
 * Copyright (C) 2000 Chris Lightfoot <chris@ex-parrot.com>
 * Copyright (C) 2002 Petr Lampa <lampa@fit.vutbr.cz>
 * Copyright (C) 2009 Michael Adam <obnox@samba.org>
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

/*
 * Routines for handling the list of upstream proxies.
 */

#include "upstream.h"
#include "heap.h"
#include "log.h"
#include "base64.h"
#include "basicauth.h"

#ifdef UPSTREAM_SUPPORT
static const char *proxy_list_string (struct upstream_proxy_list *plist)
{
#define MAXBUF ((size_t)(1024 * 96))
        static char hostport[MAXBUF];
        static char pbuffer[MAXBUF];
        struct upstream_proxy_list *upl = plist;

        bzero (&pbuffer, MAXBUF);
        snprintf (pbuffer, MAXBUF, "%s:%d", upl->host, upl->port);
        upl = upl->next;
        while (upl) {
                bzero (&hostport, MAXBUF);
                snprintf (hostport, MAXBUF, "|%s:%d", upl->host, upl->port);
                strncat (pbuffer, hostport, MAXBUF - strlen (hostport));
                upl = upl->next;
        }
        return pbuffer;
}

/* return 1 if IP string is valid, else return 0 */
static int is_valid_ip (const char *str)
{
        int num, ret = 0, dots = 0;
        char *ptr, *ip_str;

        if (str == NULL)
                return 0;

        ip_str = safestrdup (str);
        ptr = strtok (ip_str, ".");

        if (ptr == NULL)
                goto cleanup;

        while (ptr) {
                char *dptr = ptr;

                /* after parsing string, it must contain only digits */
                while (*dptr) {
                        if (*dptr >= '0' && *dptr <= '9')
                                ++dptr;
                        else
                                goto cleanup;
                }

                num = atoi (ptr);

                /* check for valid IP */
                if (num >= 0 && num <= 255) {
                        /* parse remaining string */
                        ptr = strtok (NULL, ".");
                        if (ptr != NULL)
                                ++dots;
                } else
                        goto cleanup;
        }

        /* valid IP string must contain 3 dots */
        if (dots != 3)
                goto cleanup;

        ret = 1;
cleanup:
        safefree (ip_str);
        return ret;
}

static char *get_hostip (int *lookup_err, char *host, in_addr_t ip,
                         in_addr_t mask)
{
        char *hostip;

        hostip = host;

        if (!is_valid_ip (host)) {      /* resolve host and check ip */
                int ret;
                struct addrinfo *res, *ressave;

                res = NULL;
                ret = getaddrinfo (host, NULL, NULL, &res);
                ressave = res;
                if (ret != 0) {
                        *lookup_err = ret;
                        if (ret == EAI_SYSTEM)
                                log_message (LOG_ERR,
                                             "get_hostip: Could not retrieve address info for %s: %s",
                                             host, strerror (errno));
                        else
                                log_message (LOG_ERR,
                                             "get_hostip: Could not retrieve address info for %s: %s",
                                             host, gai_strerror (ret));
                } else {
                        do {
                                struct in_addr tmp;
                                struct sockaddr_in *stmp;
                                stmp = (struct sockaddr_in *) (res->ai_addr);
                                tmp = stmp->sin_addr;

                                if ((ntohl (inet_addr (inet_ntoa (tmp))) & mask)
                                    == ip) {
                                        /* return if IP matches */
                                        hostip = inet_ntoa (tmp);
                                        break;
                                }

                        } while ((res = res->ai_next) != NULL);
                }
                if (ressave)
                        freeaddrinfo (ressave);
        }
        return safestrdup (hostip);
}

const char *
proxy_type_name(proxy_type type)
{
    switch(type) {
        case PT_NONE: return "none";
        case PT_HTTP: return "http";
        case PT_SOCKS4: return "socks4";
        case PT_SOCKS5: return "socks5";
        default: return "unknown";
    }
}

static struct upstream_proxy_list *uplcpy (const struct upstream_proxy_list
                                           *plist)
{
        struct upstream_proxy_list *upr, *upp, *uptr;

        if (!plist)
                return NULL;

        upr = upp = (upstream_proxy_list_t *)
            safemalloc (sizeof (upstream_proxy_list_t));
        upp->host = safestrdup (plist->host);
        upp->port = plist->port;
        upp->last_failed_connect = plist->last_failed_connect;
        upp->next = NULL;
        uptr = plist->next;
        while (uptr) {
                upp->next = (upstream_proxy_list_t *)
                    safemalloc (sizeof (upstream_proxy_list_t));
                upp = upp->next;
                upp->host = safestrdup (uptr->host);
                upp->port = uptr->port;
                upp->next = NULL;
                uptr = uptr->next;
        }
        return upr;
}

/**
 * Construct an upstream struct from input data.
 */
static struct upstream *upstream_build (const struct upstream_proxy_list *plist,
                                        const char *domain, const char *user,
                                        const char *pass, proxy_type type)
{
        char *ptr;
        struct upstream *up;
        struct upstream_proxy_list *upp;
#ifdef UPSTREAM_REGEX
        int cflags = REG_NEWLINE | REG_NOSUB;
        int rflag = 0;
        const char *rptr = NULL;
#endif

        up = (struct upstream *) safemalloc (sizeof (struct upstream));
        if (!up) {
                log_message (LOG_ERR,
                             "Unable to allocate memory in upstream_build()");
                return NULL;
        }

        up->type = type;
        up->domain = up->ua.user = up->pass = NULL;
        up->plist = NULL;
#ifdef UPSTREAM_REGEX
        up->pat = NULL;
        up->cpat = NULL;
        if (domain && !strncasecmp (domain, "regex(", 6)) {     /* basic regex case senstive */
                rflag = 1;
                rptr = domain + 6;
        }
        if (domain && !strncasecmp (domain, "regexe(", 7)) {    /* extended regex case senstive */
                rflag = 1;
                rptr = domain + 7;
                cflags |= REG_EXTENDED;
        }
        if (domain && !strncasecmp (domain, "regexi(", 7)) {    /* basic regex case insenstive */
                rflag = 1;
                rptr = domain + 7;
                cflags |= REG_ICASE;
        }
        if ((domain && (!strncasecmp (domain, "regexie(", 8) || /* extended regex case insenstive */
                        !strncasecmp (domain, "regexei(", 8)))) {       /* extended regex case insenstive */
                rflag = 1;
                rptr = domain + 8;
                cflags |= REG_EXTENDED;
                cflags |= REG_ICASE;
        }
        if (rflag) {
                if (domain[strlen (domain) - 1] != ')') {
                        log_message (LOG_WARNING, "Bad regex: %s", domain);
                        goto fail;
                } else {
                        up->pat = safestrdup (rptr);
                        up->pat[strlen (rptr) - 1] = '\0';
                }
        }
#endif
        up->ip = up->mask = 0;
        if (user) {
                if (type == PT_HTTP) {
                        char b[BASE64ENC_BYTES((256+2)-1) + 1];
                        ssize_t ret;
                        ret = basicauth_string(user, pass, b, sizeof b);
                        if (ret == 0) {
                                log_message (LOG_ERR,
                                             "User / pass in upstream config too long");
                                return NULL;
                        }
                        up->ua.authstr = safestrdup (b);
                } else {
                        up->ua.user = safestrdup (user);
                        up->pass = safestrdup (pass);
                }
        }

        if (domain == NULL) {
                if (!plist || plist->host[0] == '\0' || plist->port < 1) {
                        log_message (LOG_WARNING,
                                     "Nonsense upstream rule: invalid host or port");
                        goto fail;
                }

                up->plist = uplcpy (plist);

                log_message (LOG_INFO, "Added upstream %s %s for [default]",
                             proxy_type_name (type),
                             proxy_list_string (up->plist));
        } else if (plist == NULL || type == PT_NONE) {
                if (!domain || domain[0] == '\0') {
                        log_message (LOG_WARNING,
                                     "Nonsense no-upstream rule: empty domain");
                        goto fail;
                }
#ifdef UPSTREAM_REGEX
                if (!rflag) {
#endif
                        ptr = strchr (domain, '/');
                        if (ptr) {
                                struct in_addr addrstruct;

                                *ptr = '\0';
                                if (is_valid_ip (domain)) {
                                        if (inet_aton (domain, &addrstruct) !=
                                            0) {
                                                up->ip =
                                                    ntohl (addrstruct.s_addr);
                                                *ptr++ = '/';

                                                if (is_valid_ip (ptr)) {
                                                        if (inet_aton
                                                            (ptr, &addrstruct)
                                                            != 0)
                                                                up->mask =
                                                                    ntohl
                                                                    (addrstruct.
                                                                     s_addr);
                                                } else if (atoi (ptr) < 33
                                                           && atoi (ptr) > -1) {
                                                        up->mask =
                                                            ~((1 <<
                                                               (32 -
                                                                atoi (ptr))) -
                                                              1);
                                                } else {
                                                        up->domain =
                                                            safestrdup (domain);
                                                }
                                        }
                                } else {
                                        *ptr++ = '/';
                                        up->domain = safestrdup (domain);
                                }
                        } else {
                                up->domain = safestrdup (domain);
                        }
#ifdef UPSTREAM_REGEX
                } else {
                        int ret = 0;
                        up->cpat = (regex_t *) safemalloc (sizeof (regex_t));
                        ret = regcomp (up->cpat, up->pat, cflags);
                        if (ret != 0) {
                                log_message (LOG_WARNING,
                                             "Bad regex: %s", up->pat);
                                goto fail;
                        }
                        up->domain = safestrdup (domain);
                }
#endif

                log_message (LOG_INFO, "Added no-upstream for %s", domain);
        } else {
                if (!plist || plist->host[0] == '\0' || plist->port < 1
                    || !domain || domain[0] == '\0') {
                        log_message (LOG_WARNING,
                                     "Nonsense upstream rule: invalid parameters");
                        goto fail;
                }
                up->plist = uplcpy (plist);
                up->domain = safestrdup (domain);
#ifdef UPSTREAM_REGEX
                if (rflag) {
                        int ret = 0;
                        up->cpat = (regex_t *) safemalloc (sizeof (regex_t));
                        ret = regcomp (up->cpat, up->pat, cflags);
                        if (ret != 0) {
                                log_message (LOG_WARNING,
                                             "Bad regex: %s", up->pat);
                                goto fail;
                        }
                }
#endif
                log_message (LOG_INFO, "Added upstream %s %s for %s",
                             proxy_type_name (type),
                             proxy_list_string (up->plist), domain);
        }

        return up;

fail:
        safefree (up->ua.user);
        safefree (up->ua.authstr);
        safefree (up->pass);
        upp = up->plist;
        while (upp) {
                struct upstream_proxy_list *tmpp = upp;
                upp = upp->next;
                safefree (tmpp->host);
                safefree (tmpp);
        }
        safefree (up->domain);
#ifdef UPSTREAM_REGEX
        safefree (up->pat);
        if (up->cpat)
                regfree (up->cpat);
        safefree (up->cpat);
#endif
        safefree (up);

        return NULL;
}

/*
 * Add an entry to the upstream list
 */
void upstream_add (const struct upstream_proxy_list *plist,
                   const char *domain, const char *user,
                   const char *pass, proxy_type type,
                   struct upstream **upstream_list)
{
        struct upstream *up;
        struct upstream_proxy_list *upp;

        up = upstream_build (plist, domain, user, pass, type);

        if (up == NULL) {
                return;
        }

        if (!up->domain && !up->ip) {   /* always add default to end */
                struct upstream *tmp = *upstream_list;

                while (tmp) {
                        if (!tmp->domain && !tmp->ip) {
                                log_message (LOG_WARNING,
                                             "Duplicate default upstream");
                                goto upstream_cleanup;
                        }

                        if (!tmp->next) {
                                up->next = NULL;
                                tmp->next = up;
                                return;
                        }

                        tmp = tmp->next;
                }
        }

        up->next = *upstream_list;
        *upstream_list = up;

        return;

upstream_cleanup:
        upp = up->plist;
        while (upp) {
                struct upstream_proxy_list *tmpp = upp;
                upp = upp->next;
                safefree (tmpp->host);
                safefree (tmpp);
        }
        safefree (up->domain);
        safefree (up->ua.user);
        safefree (up->ua.authstr);
        safefree (up->pass);
#ifdef UPSTREAM_REGEX
        safefree (up->pat);
        if (up->cpat)
                regfree (up->cpat);
        safefree (up->cpat);
#endif
        safefree (up);

        return;
}

/*
 * Check if a host is in the upstream list
 */
struct upstream *upstream_get (struct request_s *request, struct upstream *up)
{
        char *host = request->host;
        int lookup_err;

        in_addr_t my_ip = INADDR_NONE;
        lookup_err = 0;

        DEBUG2 ("Given url %s", request->url ? request->url : "NULL");
        DEBUG2 ("Given host %s", request->host);
        while (up) {
                DEBUG2 ("Upstream type: %s\n", proxy_type_name (up->type));
#ifdef UPSTREAM_REGEX
                DEBUG2 (, "Check against pattern: %s\n",
                        up->pat ? up->pat : "NULL");
#endif
                DEBUG2 (, "Check against domain: %s\n",
                        up->domain ? up->domain : "NULL");
                if (up->ip && up->mask) {
                        struct in_addr tmp1, tmp2;
			(void)(tmp1); /* Avoid gcc warning */
			(void)(tmp2); /* Avoid gcc warning */
                        tmp1.s_addr = htonl (up->ip);
                        tmp2.s_addr = htonl (up->mask);
                        DEBUG2 ("Check against ip/mask: %s/%s\n",
                                inet_ntoa (tmp1), inet_ntoa (tmp2));
                } else {
                        DEBUG2 ("Check against ip/mask: NO\n");
                }
#ifdef UPSTREAM_REGEX
                if (up->cpat) {
                        int result;
                        char *url = request->url;
                        result =
                            regexec (up->cpat, url, (size_t) 0,
                                     (regmatch_t *) 0, 0);
                        if (up->type == PT_NONE && result == 0) {
                                up = NULL;
                                break;
                        }
                        if (result == 0)
                                break;  /* regex match */
                } else if (up->domain) {
#else
                if (up->domain) {
#endif
                        if (strcasecmp (host, up->domain) == 0) {
                                if (up->type == PT_NONE)
                                        up = NULL;
                                break;  /* exact match */
                        }

                        if (up->domain[0] == '.') {
                                char *dot = strchr (host, '.');

                                if (!dot && !up->domain[1]) {
                                        if (up->type == PT_NONE)
                                                up = NULL;
                                        break;  /* local host matches "." */
                                }

                                while (dot && strcasecmp (dot, up->domain))
                                        dot = strchr (dot + 1, '.');

                                if (dot) {
                                        if (up->type == PT_NONE)
                                                up = NULL;
                                        break;  /* subdomain match */
                                }
                        }
                } else if (up->ip && up->mask) {
                        char *hostip = NULL;

                        if (!lookup_err) {
                                hostip =
                                    get_hostip (&lookup_err, host, up->ip,
                                                up->mask);

                                if (is_valid_ip (hostip)) {
                                        if (my_ip == INADDR_NONE)
                                                my_ip =
                                                    ntohl (inet_addr (hostip));

                                        if ((my_ip & up->mask) == up->ip) {
                                                if (up->type == PT_NONE)
                                                        up = NULL;
                                                safefree (hostip);
                                                break;
                                        }
                                }
                                safefree (hostip);
                        }
                } else {
                        break;  /* No domain or IP, default upstream */
                }

                up = up->next;
        }

        if (up && !up->plist)
                up = NULL;

        if (up)
                log_message (LOG_INFO,
                             "Found upstream proxy/proxies %s %s for %s",
                             proxy_type_name (up->type),
                             proxy_list_string (up->plist), host);
        else
                log_message (LOG_INFO, "No upstream proxy for %s", host);

        return up;
}

void free_upstream_list (struct upstream *up)
{
        while (up) {
                struct upstream *tmp = up;
                struct upstream_proxy_list *upp = up->plist;
                up = up->next;
                while (upp) {
                        struct upstream_proxy_list *tmpp = upp;
                        upp = upp->next;
                        safefree (tmpp->host);
                        safefree (tmpp);
                }
                safefree (tmp->domain);
                safefree (tmp->ua.user);
                safefree (tmp->ua.authstr);
                safefree (tmp->pass);
#ifdef UPSTREAM_REGEX
                safefree (tmp->pat);
                if (tmp->cpat)
                        regfree (tmp->cpat);
                safefree (tmp->cpat);
#endif
                safefree (tmp);
        }
}

#endif
