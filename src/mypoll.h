#ifndef MYPOLL_H
#define MYPOLL_H

#include <sys/select.h>

#ifdef HAVE_POLL_H
#include <poll.h>
typedef struct pollfd pollfd_struct;
#else
typedef struct mypollfd {
	int   fd;
	short events;
	short revents;
} pollfd_struct;
#endif

#define MYPOLL_READ (1<<1)
#define MYPOLL_WRITE (1<<2)

int mypoll(pollfd_struct* fds, int nfds, int timeout);

#endif
