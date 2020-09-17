/* tinyproxy - A fast light-weight HTTP proxy
 * Copyright (C) 2000 Robert James Kaes <rjkaes@users.sourceforge.net>
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

/* Handles the creation/destruction of the various children required for
 * processing incoming connections.
 */

#include "main.h"

#include "child.h"
#include "daemon.h"
#include "filter.h"
#include "heap.h"
#include "log.h"
#include "reqs.h"
#include "sock.h"
#include "utils.h"
#include "conf.h"
#include "sblist.h"
#include "loop.h"
#include "conns.h"
#include "mypoll.h"
#include <pthread.h>

static sblist* listen_fds;

struct client {
        union sockaddr_union addr;
};

struct child {
	pthread_t thread;
	struct client client;
	struct conn_s conn;
	volatile int done;
};

static void* child_thread(void* data)
{
	struct child *c = data;
	handle_connection (&c->conn, &c->client.addr);
	c->done = 1;
	return NULL;
}

static sblist *childs;

static void collect_threads(void)
{
	size_t i;
	for (i = 0; i < sblist_getsize(childs); ) {
		struct child *c = *((struct child**)sblist_get(childs, i));
		if (c->done) {
			pthread_join(c->thread, 0);
			sblist_delete(childs, i);
			safefree(c);
		} else i++;
	}
}

/*
 * This is the main loop accepting new connections.
 */
void child_main_loop (void)
{
        int connfd;
        union sockaddr_union cliaddr_storage;
        struct sockaddr *cliaddr = (void*) &cliaddr_storage;
        socklen_t clilen = sizeof(cliaddr_storage);
        int nfds = sblist_getsize(listen_fds);
        pollfd_struct *fds = safecalloc(nfds, sizeof *fds);
        ssize_t i;
        int ret, listenfd, was_full = 0;
        pthread_attr_t *attrp, attr;
        struct child *child;

        childs = sblist_new(sizeof (struct child*), config->maxclients);

        for (i = 0; i < nfds; i++) {
                int *fd = sblist_get(listen_fds, i);
                fds[i].fd = *fd;
                fds[i].events |= MYPOLL_READ;
        }

        /*
         * We have to wait for connections on multiple fds,
         * so use select/poll/whatever.
         */
        while (!config->quit) {

                collect_threads();

                if (sblist_getsize(childs) >= config->maxclients) {
                        if (!was_full)
                                log_message (LOG_WARNING,
                                             "Maximum number of connections reached. "
                                             "Refusing new connections.");
                        was_full = 1;
                        usleep(16);
                        continue;
                }

                was_full = 0;
                listenfd = -1;

                /* Handle log rotation if it was requested */
                if (received_sighup) {

                        reload_config (1);

#ifdef FILTER_ENABLE
                        filter_reload ();
#endif /* FILTER_ENABLE */

                        received_sighup = FALSE;
                }

                ret = mypoll(fds, nfds, -1);

                if (ret == -1) {
                        if (errno == EINTR) {
                                continue;
                        }
                        log_message (LOG_ERR, "error calling " SELECT_OR_POLL ": %s",
                                     strerror(errno));
                        continue;
                } else if (ret == 0) {
                        log_message (LOG_WARNING, "Strange: " SELECT_OR_POLL " returned 0 "
                                     "but we did not specify a timeout...");
                        continue;
                }

                for (i = 0; i < nfds; i++) {
                        if (fds[i].revents & MYPOLL_READ) {
                                /*
                                 * only accept the connection on the first
                                 * fd that we find readable. - fair?
                                 */
                                listenfd = fds[i].fd;
                                break;
                        }
                }

                if (listenfd == -1) {
                        log_message(LOG_WARNING, "Strange: None of our listen "
                                    "fds was readable after " SELECT_OR_POLL);
                        continue;
                }

                /*
                 * We have a socket that is readable.
                 * Continue handling this connection.
                 */

                connfd = accept (listenfd, cliaddr, &clilen);


                /*
                 * Make sure no error occurred...
                 */
                if (connfd < 0) {
                        log_message (LOG_ERR,
                                     "Accept returned an error (%s) ... retrying.",
                                     strerror (errno));
                        continue;
                }

                child = safecalloc(1, sizeof(struct child));
                if (!child) {
oom:
                        close(connfd);
                        log_message (LOG_CRIT,
                                     "Could not allocate memory for child.");
                        usleep(16); /* prevent 100% CPU usage in OOM situation */
                        continue;
                }

                child->done = 0;

                if (!sblist_add(childs, &child)) {
                        free(child);
                        goto oom;
                }

                conn_struct_init(&child->conn);
                child->conn.client_fd = connfd;

                memcpy(&child->client.addr, &cliaddr_storage, sizeof(cliaddr_storage));

                attrp = 0;
                if (pthread_attr_init(&attr) == 0) {
                        attrp = &attr;
                        pthread_attr_setstacksize(attrp, 256*1024);
                }

                if (pthread_create(&child->thread, attrp, child_thread, child) != 0) {
                        sblist_delete(childs, sblist_getsize(childs) -1);
                        free(child);
                        goto oom;
		}
        }
	safefree(fds);
}

/*
 * Go through all the non-empty children and cancel them.
 */
void child_kill_children (int sig)
{
	size_t i, tries = 0;

	if (sig != SIGTERM) return;
	log_message (LOG_INFO,
	             "trying to bring down %zu threads...",
	             sblist_getsize(childs)
	);


again:
	for (i = 0; i < sblist_getsize(childs); i++) {
		struct child *c = *((struct child**)sblist_get(childs, i));
		if (!c->done) pthread_kill(c->thread, SIGCHLD);
	}
	usleep(8192);
	collect_threads();
	if (sblist_getsize(childs) != 0)
		if(tries++ < 8) goto again;
	if (sblist_getsize(childs) != 0)
		log_message (LOG_CRIT,
		             "child_kill_children: %zu threads still alive!",
		             sblist_getsize(childs)
		);
}

void child_free_children(void) {
	sblist_free(childs);
	childs = 0;
}

/**
 * Listen on the various configured interfaces
 */
int child_listening_sockets(sblist *listen_addrs, uint16_t port)
{
        int ret;
        size_t i;

        assert (port > 0);

        if (listen_fds == NULL) {
                listen_fds = sblist_new(sizeof(int), 16);
                if (listen_fds == NULL) {
                        log_message (LOG_ERR, "Could not create the list "
                                     "of listening fds");
                        return -1;
                }
        }

        if (!listen_addrs || !sblist_getsize(listen_addrs))
        {
                /*
                 * no Listen directive:
                 * listen on the wildcard address(es)
                 */
                ret = listen_sock(NULL, port, listen_fds);
                return ret;
        }

        for (i = 0; i < sblist_getsize(listen_addrs); i++) {
                char **addr;

                addr = sblist_get(listen_addrs, i);
                if (!addr || !*addr) {
                        log_message(LOG_WARNING,
                                    "got NULL from listen_addrs - skipping");
                        continue;
                }

                ret = listen_sock(*addr, port, listen_fds);
                if (ret != 0) {
                        return ret;
                }
        }

        return 0;
}

void child_close_sock (void)
{
        size_t i;

        for (i = 0; i < sblist_getsize(listen_fds); i++) {
                int *fd = sblist_get(listen_fds, i);
                close (*fd);
        }

        sblist_free(listen_fds);

        listen_fds = NULL;
}
