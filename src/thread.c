/* $Id: thread.c,v 1.1 2000-09-12 00:07:44 rjkaes Exp $
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
};
static struct thread_s *thread_ptr;
static pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;

struct thread_config_s {
	unsigned int maxclients, maxrequestsperchild;
	unsigned int maxspareservers, minspareservers, startservers;
} thread_config;

static unsigned int servers_waiting;	/* servers waiting for a connection */
static pthread_mutex_t servers_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t servers_cond = PTHREAD_COND_INITIALIZER;

#define LOCK_SERVERS()	 pthread_mutex_lock(&servers_mutex)
#define UNLOCK_SERVERS() pthread_mutex_unlock(&servers_mutex)

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

		LOCK_SERVERS();
		ptr->status = T_CONNECTED;
		servers_waiting--;
		UNLOCK_SERVERS();

		handle_connection(connfd);
		close(connfd);

		LOCK_SERVERS();
		ptr->status = T_WAITING;
		servers_waiting++;
		UNLOCK_SERVERS();
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
	struct timeval tv;
	struct timespec ts;

	while (config.quit == FALSE) {
		/* Wait for one of the threads to signal */
		pthread_mutex_lock(&servers_mutex);
		while (config.quit == FALSE
		       && servers_waiting < thread_config.maxspareservers
		       && servers_waiting > thread_config.minspareservers) {
			if (gettimeofday(&tv, NULL) < 0) {
				return -1;
			}
			ts.tv_sec = tv.tv_sec + 1;
			ts.tv_nsec = tv.tv_usec * 1000;
			pthread_cond_timedwait(&servers_cond, &servers_mutex, &ts);
		}

		if (config.quit == TRUE)
			return 0;

		/* If there are not enough spare servers, create more */
		if (servers_waiting < thread_config.minspareservers) {
			for (i = 0; i < thread_config.maxclients; i++) {
				if (thread_ptr[i].status == T_EMPTY) {
					pthread_create(&thread_ptr[i].tid, NULL, &thread_main, &thread_ptr[i]);
					thread_ptr[i].status = T_WAITING;
					servers_waiting++;
					log(LOG_NOTICE, "Created a new thread.");
					break;
				}
			}
		} else if (servers_waiting > thread_config.maxspareservers) {
			for (i = 0; i < thread_config.maxclients; i++) {
				if (thread_ptr[i].status == T_WAITING) {
					pthread_join(thread_ptr[i].tid, NULL);
					servers_waiting--;
					thread_ptr[i].status = T_EMPTY;
					log(LOG_NOTICE, "Killed off a thread.");
					break;
				}
			}
		}
		pthread_mutex_unlock(&servers_mutex);
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
