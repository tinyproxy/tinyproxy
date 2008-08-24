/* $Id: child.c,v 1.11.2.2 2004-06-14 20:08:12 rjkaes Exp $
 *
 * Handles the creation/destruction of the various children required for
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

#include "child.h"
#include "daemon.h"
#include "filter.h"
#include "heap.h"
#include "log.h"
#include "reqs.h"
#include "sock.h"
#include "utils.h"

static int listenfd;
static socklen_t addrlen;

/*
 * Stores the internal data needed for each child (connection)
 */
struct child_s {
	pid_t tid;
	unsigned int connects;
	enum { T_EMPTY, T_WAITING, T_CONNECTED } status;
};

/*
 * A pointer to an array of children. A certain number of children are
 * created when the program is started.
 */
static struct child_s *child_ptr;

static struct child_config_s {
	int maxclients, maxrequestsperchild;
	int maxspareservers, minspareservers, startservers;
} child_config;

static int* servers_waiting;	/* servers waiting for a connection */

/*
 * Lock/Unlock the "servers_waiting" variable so that two children cannot
 * modify it at the same time.
 */
#define SERVER_COUNT_LOCK()   _child_lock_wait()
#define SERVER_COUNT_UNLOCK() _child_lock_release()

/* START OF LOCKING SECTION */

/*
 * These variables are required for the locking mechanism.  Also included
 * are the "private" functions for locking/unlocking.
 */
static struct flock lock_it, unlock_it;
static int lock_fd = -1;

static void
_child_lock_init(void)
{
	char lock_file[] = "/tmp/tinyproxy.servers.lock.XXXXXX";

        /* Only allow u+rw bits. This may be required for some versions
         * of glibc so that mkstemp() doesn't make us vulnerable.
         */
        umask(0177);

	lock_fd = mkstemp(lock_file);
	unlink(lock_file);

	lock_it.l_type = F_WRLCK;
	lock_it.l_whence = SEEK_SET;
	lock_it.l_start = 0;
	lock_it.l_len = 0;

	unlock_it.l_type = F_UNLCK;
	unlock_it.l_whence = SEEK_SET;
	unlock_it.l_start = 0;
	unlock_it.l_len = 0;
}

static void
_child_lock_wait(void)
{
	int rc;

	while ((rc = fcntl(lock_fd, F_SETLKW, &lock_it)) < 0) {
		if (errno == EINTR)
			continue;
		else
			return;
	}
}

static void
_child_lock_release(void)
{
	if (fcntl(lock_fd, F_SETLKW, &unlock_it) < 0)
		return;
}

/* END OF LOCKING SECTION */

#define SERVER_INC() do { \
    SERVER_COUNT_LOCK(); \
    ++(*servers_waiting); \
    DEBUG2("INC: servers_waiting: %d", *servers_waiting); \
    SERVER_COUNT_UNLOCK(); \
} while (0)

#define SERVER_DEC() do { \
    SERVER_COUNT_LOCK(); \
    --(*servers_waiting); \
    DEBUG2("DEC: servers_waiting: %d", *servers_waiting); \
    SERVER_COUNT_UNLOCK(); \
} while (0)

/*
 * Set the configuration values for the various child related settings.
 */
short int
child_configure(child_config_t type, int val)
{
	switch (type) {
	case CHILD_MAXCLIENTS:
		child_config.maxclients = val;
		break;
	case CHILD_MAXSPARESERVERS:
		child_config.maxspareservers = val;
		break;
	case CHILD_MINSPARESERVERS:
		child_config.minspareservers = val;
		break;
	case CHILD_STARTSERVERS:
		child_config.startservers = val;
		break;
	case CHILD_MAXREQUESTSPERCHILD:
		child_config.maxrequestsperchild = val;
		break;
	default:
		DEBUG2("Invalid type (%d)", type);
		return -1;
	}

	return 0;
}

/*
 * This is the main (per child) loop.
 */
static void
child_main(struct child_s* ptr)
{
	int connfd;
	struct sockaddr *cliaddr;
	socklen_t clilen;

	cliaddr = safemalloc(addrlen);
	if (!cliaddr) {
		log_message(LOG_CRIT,
			    "Could not allocate memory for child address.");
		exit(0);
	}

	ptr->connects = 0;

	while (!config.quit) {
		ptr->status = T_WAITING;

		clilen = addrlen;

		connfd = accept(listenfd, cliaddr, &clilen);

#ifndef NDEBUG
		/*
		 * Enable the TINYPROXY_DEBUG environment variable if you
		 * want to use the GDB debugger.
		 */
		if (getenv("TINYPROXY_DEBUG")) {
			/* Pause for 10 seconds to allow us to connect debugger */
			fprintf(stderr,
				"Process has accepted connection: %u\n", ptr->tid);
			sleep(10);
			fprintf(stderr, "Continuing process: %u\n", ptr->tid);
		}
#endif

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

		if (child_config.maxrequestsperchild != 0) {
			ptr->connects++;

			DEBUG2("%u connections so far...", ptr->connects);

			if (ptr->connects == child_config.maxrequestsperchild) {
				log_message(LOG_NOTICE,
					    "Child has reached MaxRequestsPerChild (%u). Killing child.",
					    ptr->connects);
				break;
			}
		}

		SERVER_COUNT_LOCK();
		if (*servers_waiting > child_config.maxspareservers) {
			/*
			 * There are too many spare children, kill ourself
			 * off.
			 */
			log_message(LOG_NOTICE,
				    "Waiting servers (%d) exceeds MaxSpareServers (%d). Killing child.",
				    *servers_waiting, child_config.maxspareservers);
			SERVER_COUNT_UNLOCK();

			break;
		} else {
			SERVER_COUNT_UNLOCK();
		}

		SERVER_INC();
	}

	ptr->status = T_EMPTY;

	safefree(cliaddr);
	exit(0);
}

/*
 * Fork a child "child" (or in our case a process) and then start up the
 * child_main() function.
 */
static int
child_make(struct child_s* ptr)
{
	pid_t pid;
       
	if ((pid = fork()) > 0)
		return pid;	/* parent */

	/*
	 * Reset the SIGNALS so that the child can be reaped.
	 */
	set_signal_handler(SIGCHLD, SIG_DFL);
	set_signal_handler(SIGTERM, SIG_DFL);
	set_signal_handler(SIGHUP, SIG_DFL);

	child_main(ptr); /* never returns */
	return -1;
}

/*
 * Create a pool of children to handle incoming connections
 */
short int
child_pool_create(void)
{
	unsigned int i;

	/*
	 * Make sure the number of MaxClients is not zero, since this
	 * variable determines the size of the array created for children
	 * later on.
	 */
	if (child_config.maxclients == 0) {
		log_message(LOG_ERR,
			    "child_pool_create: \"MaxClients\" must be greater than zero.");
		return -1;
	}
	if (child_config.startservers == 0) {
		log_message(LOG_ERR,
			    "child_pool_create: \"StartServers\" must be greater than zero.");
		return -1;
	}

	child_ptr = calloc_shared_memory(child_config.maxclients,
					 sizeof(struct child_s));
	if (!child_ptr) {
		log_message(LOG_ERR, "Could not allocate memory for children.");
		return -1;
	}

	servers_waiting = malloc_shared_memory(sizeof(int));
	if (servers_waiting == MAP_FAILED) {
		log_message(LOG_ERR, "Could not allocate memory for child counting.");
		return -1;
	}
	*servers_waiting = 0;

	/*
	 * Create a "locking" file for use around the servers_waiting
	 * variable.
	 */
	_child_lock_init();

	if (child_config.startservers > child_config.maxclients) {
		log_message(LOG_WARNING,
			    "Can not start more than \"MaxClients\" servers. Starting %u servers instead.",
			    child_config.maxclients);
		child_config.startservers = child_config.maxclients;
	}

	for (i = 0; i != child_config.maxclients; i++) {
		child_ptr[i].status = T_EMPTY;
		child_ptr[i].connects = 0;
	}

	for (i = 0; i != child_config.startservers; i++) {
		DEBUG2("Trying to create child %d of %d", i + 1, child_config.startservers);
		child_ptr[i].status = T_WAITING;
		child_ptr[i].tid = child_make(&child_ptr[i]);

		if (child_ptr[i].tid < 0) {
			log_message(LOG_WARNING,
				    "Could not create child number %d of %d",
				    i, child_config.startservers);
			return -1;
		} else {
			log_message(LOG_INFO,
				    "Creating child number %d of %d ...",
				    i + 1, child_config.startservers);

			SERVER_INC();
		}
	}

	log_message(LOG_INFO, "Finished creating all children.");

	return 0;
}

/*
 * Keep the proper number of servers running. This is the birth of the
 * servers. It monitors this at least once a second.
 */
void
child_main_loop(void)
{
	int i;

	while (1) {
		if (config.quit)
			return;

		/* If there are not enough spare servers, create more */
		SERVER_COUNT_LOCK();
		if (*servers_waiting < child_config.minspareservers) {
			log_message(LOG_NOTICE,
				    "Waiting servers (%d) is less than MinSpareServers (%d). Creating new child.",
				    *servers_waiting, child_config.minspareservers);

			SERVER_COUNT_UNLOCK();

			for (i = 0; i != child_config.maxclients; i++) {
				if (child_ptr[i].status == T_EMPTY) {
					child_ptr[i].status = T_WAITING;
					child_ptr[i].tid = child_make(&child_ptr[i]);
					if (child_ptr[i].tid < 0) {
						log_message(LOG_NOTICE,
							    "Could not create child");

						child_ptr[i].status = T_EMPTY;
						break;
					}

					SERVER_INC();

					break;
				}
			}
		} else {
			SERVER_COUNT_UNLOCK();
		}

		sleep(5);

		/* Handle log rotation if it was requested */
		if (received_sighup) {
			truncate_log_file();

#ifdef FILTER_ENABLE
			if (config.filter) {
				filter_destroy();
				filter_init();
			}
	log_message(LOG_NOTICE, "Re-reading filter file.");
#endif				/* FILTER_ENABLE */

			received_sighup = FALSE;
		}
	}
}

/*
 * Go through all the non-empty children and cancel them.
 */
void
child_kill_children(void)
{
	int i;
	
	for (i = 0; i != child_config.maxclients; i++) {
		if (child_ptr[i].status != T_EMPTY)
			kill(child_ptr[i].tid, SIGTERM);
	}
}

int
child_listening_sock(uint16_t port)
{
	listenfd = listen_sock(port, &addrlen);
	return listenfd;
}

void
child_close_sock(void)
{
	close(listenfd);
}
