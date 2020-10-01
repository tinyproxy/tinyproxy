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


const char* upstream_build_error_string(enum upstream_build_error ube) {
        static const char *emap[] = {
        [UBE_SUCCESS] = "",
        [UBE_OOM] = "Unable to allocate memory in upstream_build()",
        [UBE_USERLEN] = "User / pass in upstream config too long",
        [UBE_EDOMAIN] = "Nonsense upstream none rule: empty domain",
        [UBE_INVHOST] = "Nonsense upstream rule: invalid host or port",
        [UBE_INVPARAMS] = "Nonsense upstream rule: invalid parameters",
        [UBE_NETMASK] = "Nonsense upstream rule: failed to parse netmask",
        };
        return emap[ube];
}

/**
 * Construct an upstream struct from input data.
 */
static struct upstream *upstream_build (const char *host, int port, const char *domain,
                        const char *user, const char *pass,
			proxy_type type, enum upstream_build_error *ube)
{
        char *ptr;
        struct upstream *up;

        *ube = UBE_SUCCESS;
        up = (struct upstream *) safemalloc (sizeof (struct upstream));
        if (!up) {
                *ube = UBE_OOM;
                return NULL;
        }

        up->type = type;
        up->host = up->domain = up->ua.user = up->pass = NULL;
        up->ip = up->mask = 0;
        if (user) {
                if (type == PT_HTTP) {
                        char b[BASE64ENC_BYTES((256+2)-1) + 1];
                        ssize_t ret;
                        ret = basicauth_string(user, pass, b, sizeof b);
                        if (ret == 0) {
                                *ube = UBE_USERLEN;
                                return NULL;
                        }
                        up->ua.authstr = safestrdup (b);
                } else {
                        up->ua.user = safestrdup (user);
                        up->pass = safestrdup (pass);
                }
        }

        if (domain == NULL) {
                if (type == PT_NONE) {
                e_nonedomain:;
                        *ube = UBE_EDOMAIN;
                        goto fail;
                }
                if (!host || !host[0] || port < 1) {
                        *ube = UBE_INVHOST;
                        goto fail;
                }

                up->host = safestrdup (host);
                up->port = port;

                log_message (LOG_INFO, "Added upstream %s %s:%d for [default]",
                             proxy_type_name(type), host, port);
        } else {
                if (type == PT_NONE) {
                        if (!domain[0]) goto e_nonedomain;
                } else {
                        if (!host || !host[0] || !domain[0]) {
                                *ube = UBE_INVPARAMS;
                                goto fail;
                        }
                        up->host = safestrdup (host);
                        up->port = port;
                }

                ptr = strchr (domain, '/');
                if (ptr) {
                        struct in_addr addrstruct;

                        *ptr = '\0';
                        if (inet_aton (domain, &addrstruct) != 0) {
                                up->ip = ntohl (addrstruct.s_addr);
                                *ptr++ = '/';

                                if (strchr (ptr, '.')) {
                                        if (inet_aton (ptr, &addrstruct) != 0)
                                                up->mask =
                                                    ntohl (addrstruct.s_addr);
                                } else {
                                        up->mask =
                                            ~((1 << (32 - atoi (ptr))) - 1);
                                }
                                up->ip = up->ip & up->mask;
                        } else {
                                *ube = UBE_NETMASK;
                                goto fail;
                        }
                } else {
                        up->domain = safestrdup (domain);
                }

                if (type == PT_NONE)
                        log_message (LOG_INFO, "Added upstream none for %s", domain);
                else
                        log_message (LOG_INFO, "Added upstream %s %s:%d for %s",
                                     proxy_type_name(type), host, port, domain);
        }

        return up;

fail:
        safefree (up->ua.user);
        safefree (up->pass);
        safefree (up->host);
        safefree (up->domain);
        safefree (up);

        return NULL;
}

/*
 * Add an entry to the upstream list
 */
enum upstream_build_error upstream_add (
                   const char *host, int port, const char *domain,
                   const char *user, const char *pass,
                   proxy_type type, struct upstream **upstream_list)
{
        struct upstream *up;
        enum upstream_build_error ube;

        up = upstream_build (host, port, domain, user, pass, type, &ube);
        if (up == NULL) {
                return ube;
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
                                return ube;
                        }

                        tmp = tmp->next;
                }
        }

        up->next = *upstream_list;
        *upstream_list = up;

        return ube;

upstream_cleanup:
        safefree (up->host);
        safefree (up->domain);
        safefree (up);

        return ube;
}

/*
 * Check if a host is in the upstream list
 */
struct upstream *upstream_get (char *host, struct upstream *up)
{
        in_addr_t my_ip = INADDR_NONE;

        while (up) {
                if (up->domain) {
                        if (strcasecmp (host, up->domain) == 0)
                                break;  /* exact match */

                        if (up->domain[0] == '.') {
                                char *dot = strchr (host, '.');

                                if (!dot && !up->domain[1])
                                        break;  /* local host matches "." */

                                while (dot && strcasecmp (dot, up->domain))
                                        dot = strchr (dot + 1, '.');

                                if (dot)
                                        break;  /* subdomain match */
                        }
                } else if (up->ip) {
                        if (my_ip == INADDR_NONE)
                                my_ip = ntohl (inet_addr (host));

                        if ((my_ip & up->mask) == up->ip)
                                break;
                } else {
                        break;  /* No domain or IP, default upstream */
                }

                up = up->next;
        }

        if (up && (!up->host))
                up = NULL;

        if (up)
                log_message (LOG_INFO, "Found upstream proxy %s %s:%d for %s",
                             proxy_type_name(up->type), up->host, up->port, host);
        else
                log_message (LOG_INFO, "No upstream proxy for %s", host);

        return up;
}

void free_upstream_list (struct upstream *up)
{
        while (up) {
                struct upstream *tmp = up;
                up = up->next;
                safefree (tmp->domain);
                safefree (tmp->host);
                safefree (tmp);
        }
}

#endif
