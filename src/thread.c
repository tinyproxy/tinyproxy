/* $Id: thread.c,v 1.3 2000-12-09 02:35:30 rjkaes Exp $
 *
 * Handles the creation/destruction of the various threads required for
 * processing incoming connections.
 *
 * Copyright (C) 2000 Robert James Kaes (rjkaes@flarenet.com)
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
#include "reqs.h"
#include "sock.h"
#include "thread.h"

static int listenfd;
static socklen_t addrlen;
struct thread_s {
	pthread_t tid;
	enum { T_EMPTY, T_WAITING, T_CONNECTED } status;
	unsigned int connects;
};
static struct thread_s *thread_ptr;
static pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;

struct thread_config_s {
	unsigned int maxclients, maxrequestsperchild;
	unsigned int maxspareservers, minspareservers, startservers;
} thread_config;

static unsigned int servers_waiting;	/* servers waiting for a connection */
static pthread_mutex_t servers_mutex = PTHREAD_MUTEX_INITIALIZER;

#define SERVER_INC() do { \
    pthread_mutex_lock(&servers_mutex); \
    servers_waiting++; \
    pthread_mutex_unlock(&servers_mutex); \
} while (0)

#define SERVER_DEC() do { \
    pthread_mutex_lock(&servers_mutex); \
    servers_waiting--; \
    pthread_mutex_unlock(&servers_mutex); \
} while (0)

/*
 * Set the configuration values for the various thread related settings.
 */
int thread_configure(thread_config_t type, unsigned int val)
{
	switch (type) {
	case THREAD_MAXCLIENTS:
		thread_config.maxclients = val;
		break;
	case THREAD_MAXSPARESERVERS:
		thread_config.maxspareservers = val;
		break;
	case THREAD_MINSPARESERVERS:
		thread_config.minspareservers = val;
		break;
	case THREAD_STARTSERVERS:
		thread_config.startservers = val;
		break;
	case THREAD_MAXREQUESTSPERCHILD:
		thread_config.maxrequestsperchild = val;
		break;
	default:
		DEBUG2("Invalid type (%d)", type);
		return -1;
	}

	return 0;
}

/*
 * This is the main (per thread) loop.
 */
static void *thread_main(void *arg)
{
	int connfd;
	struct sockaddr *cliaddr;
	socklen_t clilen;
	struct thread_s *ptr;

	ptr = (struct thread_s *)arg;

	cliaddr = malloc(addrlen);
	if (!cliaddr) {
		log(LOG_ERR, "Could not allocate memory");
		return NULL;
	}
	
	for ( ; ; ) {
		clilen = addrlen;
		pthread_mutex_lock(&mlock);
		connfd = accept(listenfd, cliaddr, &clilen);
		pthread_mutex_unlock(&mlock);

		ptr->status = T_CONNECTED;

		SERVER_DEC();

		handle_connection(connfd);
		close(connfd);

		if (thread_config.maxrequestsperchild != 0)
			ptr->connects++;

		if (ptr->connects > thread_config.maxrequestsperchild) {
			ptr->connects = 0;
			ptr->status = T_EMPTY;
			return NULL;
		} else {
			ptr->status = T_WAITING;

			SERVER_INC();
		}
	}
}
/*
 * Create the initial pool of threads.
 */
int thread_pool_create(void)
{
	unsigned int i;

	if (thread_config.maxclients == 0) {
		log(LOG_ERR, "You must set MaxClients to a value greater than 0");
		return -1;
	}
	if (thread_config.startservers == 0) {
		log(LOG_ERR, "You must set StartServers to a value greate than 0");
		return -1;
	}

	thread_ptr = calloc(thread_config.maxclients, sizeof(struct thread_s));
	if (!thread_ptr)
		return -1;

	if (thread_config.startservers > thread_config.maxclients) {
		log(LOG_WARNING, "Can not start more than 'MaxClients' servers. Starting %d servers", thread_config.maxclients);
		thread_config.startservers = thread_config.maxclients;
	}

	for (i = 0; i < thread_config.startservers; i++) {
		thread_ptr[i].status = T_WAITING;
		servers_waiting++;
		pthread_create(&thread_ptr[i].tid, NULL, &thread_main, &thread_ptr[i]);
	}
	for (i = thread_config.startservers; i < thread_config.maxclients; i++) {
		thread_ptr[i].status = T_EMPTY;
		thread_ptr[i].connects = 0;
	}

	return 0;
}

/*
 * Keep the proper number of servers running. This is the birth and death
 * of the servers. It monitors this at least once a second.
 */
int thread_main_loop(void)
{
	int i;

	/* If there are not enough spare servers, create more */
	if (servers_waiting < thread_config.minspareservers) {
		for (i = 0; i < thread_config.maxclients; i++) {
			if (thread_ptr[i].status == T_EMPTY) {
				pthread_create(&thread_ptr[i].tid, NULL, &thread_main, &thread_ptr[i]);
				thread_ptr[i].status = T_WAITING;

				SERVER_INC();

				log(LOG_NOTICE, "Created a new thread.");
				break;
			}
		}
	} else if (servers_waiting > thread_config.maxspareservers) {
		for (i = 0; i < thread_config.maxclients; i++) {
			if (thread_ptr[i].status == T_WAITING) {
				pthread_join(thread_ptr[i].tid, NULL);

				SERVER_DEC();

				thread_ptr[i].status = T_EMPTY;
				log(LOG_NOTICE, "Killed off a thread.");
				break;
			}
		}
	}
	
	return 0;
}

inline int thread_listening_sock(unsigned int port)
{
	listenfd = listen_sock(port, &addrlen);
	return listenfd;
}

inline void thread_close_sock(void)
{
	close(listenfd);
}
