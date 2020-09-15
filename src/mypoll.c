#include "mypoll.h"

#define MYPOLL_READ (1<<1)
#define MYPOLL_WRITE (1<<2)

int mypoll(pollfd_struct* fds, int nfds, int timeout) {
	fd_set rset, wset, *r=0, *w=0;
	int i, ret, maxfd=-1;
	struct timeval tv = {0}, *t = 0;

	for(i=0; i<nfds; ++i) {
		if(fds[i].events & MYPOLL_READ) r = &rset;
		if(fds[i].events & MYPOLL_WRITE) w = &wset;
		if(r && w) break;
	}
	if(r) FD_ZERO(r);
	if(w) FD_ZERO(w);
	for(i=0; i<nfds; ++i) {
		if(fds[i].fd > maxfd) maxfd = fds[i].fd;
		if(fds[i].events & MYPOLL_READ) FD_SET(fds[i].fd, r);
		if(fds[i].events & MYPOLL_WRITE) FD_SET(fds[i].fd, w);
	}

	if(timeout >= 0) t = &tv;
	if(timeout > 0) tv.tv_sec = timeout;

	ret = select(maxfd+1, r, w, 0, t);

	switch(ret) {
		case -1:
		case 0:
			return ret;
	}

	for(i=0; i<nfds; ++i) {
		fds[i].revents = 0;
		if(r && FD_ISSET(fds[i].fd, r)) fds[i].revents |= MYPOLL_READ;
		if(w && FD_ISSET(fds[i].fd, w)) fds[i].revents |= MYPOLL_WRITE;
	}
	return ret;
}

