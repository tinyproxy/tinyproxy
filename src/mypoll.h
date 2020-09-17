#ifndef MYPOLL_H
#define MYPOLL_H

#include "config.h"

#ifdef HAVE_POLL_H
#define SELECT_OR_POLL "poll"

#include <poll.h>
typedef struct pollfd pollfd_struct;

#define MYPOLL_READ POLLIN
#define MYPOLL_WRITE POLLOUT

#else

#define SELECT_OR_POLL "select"
#include <sys/select.h>
typedef struct mypollfd {
	int   fd;
	short events;
	short revents;
} pollfd_struct;

#define MYPOLL_READ (1<<1)
#define MYPOLL_WRITE (1<<2)
#endif

int mypoll(pollfd_struct* fds, int nfds, int timeout);

#endif
