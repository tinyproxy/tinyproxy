/* $Id: thread.c,v 1.19 2001-12-28 22:29:11 rjkaes Exp $
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
#include "utils.h"

/*
 * This is the stack frame size used by all the threads. We'll start by
 * setting it to 128 KB.
 */
#define THREAD_STACK_SIZE (1024 * 32)

static int listenfd;
static socklen_t addrlen;

/*
 * Stores the internal data needed for each thread (connection)
 */
struct thread_s {
	pthread_t tid;
	enum { T_EMPTY, T_WAITING, T_CONNECTED } status;
	unsigned int connects;
};

/*
 * A pointer to an array of threads. A certain number of threads are
 * created when the program is started.
 */
static struct thread_s *thread_ptr;
static pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;

/* Used to override the default statck size. */
static pthread_attr_t thread_attr;

static struct thread_config_s {
	unsigned int maxclients, maxrequestsperchild;
	unsigned int maxspareservers, minspareservers, startservers;
} thread_config;

static unsigned int servers_waiting;	/* servers waiting for a connection */
static pthread_mutex_t servers_mutex = PTHREAD_MUTEX_INITIALIZER;

#define SERVER_LOCK()   pthread_mutex_lock(&servers_mutex)
#define SERVER_UNLOCK() pthread_mutex_unlock(&servers_mutex)

#define SERVER_INC() do { \
    SERVER_LOCK(); \
    DEBUG2("INC: servers_waiting: %u", servers_waiting); \
    servers_waiting++; \
    SERVER_UNLOCK(); \
} while (0)

#define SERVER_DEC() do { \
    SERVER_LOCK(); \
    servers_waiting--; \
    DEBUG2("DEC: servers_waiting: %u", servers_waiting); \
    SERVER_UNLOCK(); \
} while (0)

/*
 * Set the configuration values for the various thread related settings.
 */
short int
thread_configure(thread_config_t type, unsigned int val)
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
static void *
thread_main(void *arg)
{
	int connfd;
	struct sockaddr *cliaddr;
	socklen_t clilen;
	struct thread_s *ptr;

	ptr = (struct thread_s *) arg;

	cliaddr = safemalloc(addrlen);
	if (!cliaddr)
		return NULL;

	while (!config.quit) {
		clilen = addrlen;

		pthread_mutex_lock(&mlock);
		connfd = accept(listenfd, cliaddr, &clilen);
		pthread_mutex_unlock(&mlock);

		/*
		 * Make sure no error occurred...
		 */
		if (connfd < 0) {
			log_message(LOG_ERR, "Accept returned an error (%s) ... retrying.", strerror(errno));
			continue;
		}


		ptr->status = T_CONNECTED;

		SERVER_DEC();

		handle_connection(connfd);
		close(connfd);

		if (thread_config.maxrequestsperchild != 0) {
			ptr->connects++;

			DEBUG2("%u connections so far...", ptr->connects);

			if (ptr->connects >= thread_config.maxrequestsperchild) {
				log_message(LOG_NOTICE,
					    "Thread has reached MaxRequestsPerChild (%u > %u). Killing thread.",
					    ptr->connects,
					    thread_config.maxrequestsperchild);

				ptr->status = T_EMPTY;

				safefree(cliaddr);

				return NULL;
			}
		}

		SERVER_LOCK();
		if (servers_waiting >= thread_config.maxspareservers) {
			/*
			 * There are too many spare threads, kill ourself
			 * off.
			 */
			SERVER_UNLOCK();

			log_message(LOG_NOTICE,
				    "Waiting servers exceeds MaxSpareServers. Killing thread.");

			ptr->status = T_EMPTY;

			safefree(cliaddr);
			return NULL;
		}
		SERVER_UNLOCK();

		ptr->status = T_WAITING;

		SERVER_INC();
	}

	safefree(cliaddr);
	return NULL;
}

/*
 * Create the initial pool of threads.
 */
short int
thread_pool_create(void)
{
	unsigned int i;

	/*
	 * Initialize thread_attr to contain a non-default stack size
	 * because the default on some OS's is too small. Also, make sure
	 * we're using a detached creation method so all resources are
	 * reclaimed when the thread exits.
	 */
	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setstacksize(&thread_attr, THREAD_STACK_SIZE);

	if (thread_config.maxclients == 0) {
		log_message(LOG_ERR,
			    "thread_pool_create: \"MaxClients\" must be greater than zero.");
		return -1;
	}
	if (thread_config.startservers == 0) {
		log_message(LOG_ERR,
			    "thread_pool_create: \"StartServers\" must be greater than zero.");
		return -1;
	}

	thread_ptr =
	    safecalloc((size_t) thread_config.maxclients,
		       sizeof(struct thread_s));
	if (!thread_ptr)
		return -1;

	if (thread_config.startservers > thread_config.maxclients) {
		log_message(LOG_WARNING,
			    "Can not start more than \"MaxClients\" servers. Starting %u servers instead.",
			    thread_config.maxclients);
		thread_config.startservers = thread_config.maxclients;
	}

	for (i = 0; i < thread_config.startservers; i++) {
		thread_ptr[i].status = T_WAITING;
		pthread_create(&thread_ptr[i].tid, &thread_attr, &thread_main,
			       &thread_ptr[i]);
	}
	servers_waiting = thread_config.startservers;

	for (i = thread_config.startservers; i < thread_config.maxclients; i++) {
		thread_ptr[i].status = T_EMPTY;
		thread_ptr[i].connects = 0;
	}

	return 0;
}

/*
 * Keep the proper number of servers running. This is the birth of the
 * servers. It monitors this at least once a second.
 */
void
thread_main_loop(void)
{
	int i;

	/* If there are not enough spare servers, create more */
	SERVER_LOCK();
	if (servers_waiting < thread_config.minspareservers) {
		SERVER_UNLOCK();

		for (i = 0; i < thread_config.maxclients; i++) {
			if (thread_ptr[i].status == T_EMPTY) {
				pthread_create(&thread_ptr[i].tid, &thread_attr,
					       &thread_main, &thread_ptr[i]);
				thread_ptr[i].status = T_WAITING;
				thread_ptr[i].connects = 0;

				SERVER_INC();

				log_message(LOG_NOTICE,
					    "Waiting servers is less than MinSpareServers. Creating new thread.");

				break;
			}
		}
	}
	SERVER_UNLOCK();
}

int
thread_listening_sock(uint16_t port)
{
	listenfd = listen_sock(port, &addrlen);
	return listenfd;
}

void
thread_close_sock(void)
{
	close(listenfd);
}
