/* tinyproxy - A fast light-weight HTTP proxy
 * Copyright (C) 2002       Petr Lampa <lampa@fit.vutbr.cz>
 * Copyright (C) 2008       Robert James Kaes <rjk@wormbytes.ca>
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
 * This section of code is used for the transparent proxy option.  You will
 * need to configure your firewall to redirect all connections for HTTP
 * traffic to tinyproxy for this to work properly.
 */

#include "transparent-proxy.h"
#include "conns.h"
#include "heap.h"
#include "html-error.h"
#include "log.h"
#include "reqs.h"
#include "text.h"
#include "conf.h"

/*
 * Build a URL from parts.
 */
static int build_url (char **url, const char *host, int port, const char *path)
{
        int len;

        assert (url != NULL);
        assert (host != NULL);
        assert (port > 0 && port < 32768);
        assert (path != NULL);

        len = strlen (host) + strlen (path) + 14;
        *url = (char *) saferealloc (*url, len);
        if (*url == NULL)
                return -1;

        return snprintf (*url, len, "http://%s:%d%s", host, port, path);
}

int
do_transparent_proxy (struct conn_s *connptr, pseudomap *hashofheaders,
                      struct request_s *request, struct config_s *conf,
                      char **url)
{
        socklen_t length;
        char *data;
        size_t ulen = strlen (*url);
        size_t i;

        data = pseudomap_find (hashofheaders, "host");
        if (!data) {
                union sockaddr_union dest_addr;
                const void *dest_inaddr;
                char namebuf[INET6_ADDRSTRLEN+1];
                int af;
                length = sizeof(dest_addr);

                if (getsockname
                    (connptr->client_fd, (void *) &dest_addr,
                     &length) < 0 || length > sizeof(dest_addr)) {
                addr_err:;
                        log_message (LOG_ERR,
                                     "process_request: cannot get destination IP for %d",
                                     connptr->client_fd);
                        indicate_http_error (connptr, 400, "Bad Request",
                                             "detail", "Unknown destination",
                                             "url", *url, NULL);
                        return 0;
                }

                af = SOCKADDR_UNION_AF(&dest_addr);
                dest_inaddr = SOCKADDR_UNION_ADDRESS(&dest_addr);

                if (!inet_ntop(af, dest_inaddr, namebuf, sizeof namebuf))
                        goto addr_err;

                request->host = safestrdup (namebuf);
                request->port = ntohs (SOCKADDR_UNION_PORT(&dest_addr));

                request->path = (char *) safemalloc (ulen + 1);
                strlcpy (request->path, *url, ulen + 1);

                build_url (url, request->host, request->port, request->path);
                log_message (LOG_INFO,
                             "process_request: trans IP %s %s for %d",
                             request->method, *url, connptr->client_fd);
        } else {
                length = strlen (data);
                request->host = (char *) safemalloc (length + 1);
                if (sscanf (data, "%[^:]:%hu", request->host, &request->port) !=
                    2) {
                        strlcpy (request->host, data, length + 1);
                        request->port = HTTP_PORT;
                }

                request->path = (char *) safemalloc (ulen + 1);
                strlcpy (request->path, *url, ulen + 1);

                build_url (url, request->host, request->port, request->path);
                log_message (LOG_INFO,
                             "process_request: trans Host %s %s for %d",
                             request->method, *url, connptr->client_fd);
        }

        if (conf->listen_addrs == NULL) {
                return 1;
        }

        for (i = 0; i < sblist_getsize(conf->listen_addrs); i++) {
                char **addr;

                addr = sblist_get(conf->listen_addrs, i);

                if (addr && *addr && strcmp(request->host, *addr) == 0) {
                        log_message(LOG_ERR,
                                    "transparent: destination IP %s is local "
                                    "on socket fd %d",
                                    request->host, connptr->client_fd);
                        indicate_http_error(connptr, 400, "Bad Request",
                                            "detail",
                                            "You tried to connect to the "
                                            "machine the proxy is running on",
                                            "url", *url, NULL);
                        return 0;
                }
        }

        return 1;
}
