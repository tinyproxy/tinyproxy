m4_dnl hcommon.c
m4_dnl (part of complex test harness, not of the library)
m4_dnl - routines used for both record and playback

m4_dnl  This file is
m4_dnl    Copyright (C) 1997-1999 Ian Jackson <ian@davenant.greenend.org.uk>
m4_dnl
m4_dnl  It is part of adns, which is
m4_dnl    Copyright (C) 1997-1999 Ian Jackson <ian@davenant.greenend.org.uk>
m4_dnl    Copyright (C) 1999 Tony Finch <dot@dotat.at>
m4_dnl  
m4_dnl  This program is free software; you can redistribute it and/or modify
m4_dnl  it under the terms of the GNU General Public License as published by
m4_dnl  the Free Software Foundation; either version 2, or (at your option)
m4_dnl  any later version.
m4_dnl  
m4_dnl  This program is distributed in the hope that it will be useful,
m4_dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of
m4_dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
m4_dnl  GNU General Public License for more details.
m4_dnl  
m4_dnl  You should have received a copy of the GNU General Public License
m4_dnl  along with this program; if not, write to the Free Software Foundation,
m4_dnl  Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 

m4_include(hmacros.i4)

#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "harness.h"
#include "internal.h"

vbuf vb;
FILE *Toutputfile= 0;
struct timeval currenttime;

const struct Terrno Terrnos[]= {
  { "EAGAIN",                    EAGAIN                       },
  { "EINPROGRESS",               EINPROGRESS                  },
  { "EINTR",                     EINTR                        },
  { "EINVAL",                    EINVAL                       },
  { "EMSGSIZE",                  EMSGSIZE                     },
  { "ENOBUFS",                   ENOBUFS                      },
  { "ENOENT",                    ENOENT                       },
  { "ENOPROTOOPT",               ENOPROTOOPT                  },
  { "ENOSPC",                    ENOSPC                       },
  { "EWOULDBLOCK",               EWOULDBLOCK                  },
  { "EHOSTUNREACH",              EHOSTUNREACH                 },
  { "ECONNREFUSED",              ECONNREFUSED                 },
  { "EPIPE",                     EPIPE                        },
  {  0,                          0                            }
};

static vbuf vbw;

int Hgettimeofday(struct timeval *tv, struct timezone *tz) {
  Tmust("gettimeofday","tz",!tz);
  *tv= currenttime;
  return 0;
}

int Hwritev(int fd, const struct iovec *vector, size_t count) {
  size_t i;
  
  vbw.used= 0;
  for (i=0; i<count; i++, vector++) {
    if (!adns__vbuf_append(&vbw,vector->iov_base,vector->iov_len)) Tnomem();
  }
  return Hwrite(fd,vbw.buf,vbw.used);
}

m4_define(`hm_syscall', `
 hm_create_proto_q
void Q$1(hm_args_massage($3,void)) {

 vb.used= 0;
 Tvba("$1");
 m4_define(`hm_na',`')
 m4_define(`hm_arg_nullptr',`')
 m4_define(`hm_arg_int', `Tvbf(" $'`1=%d",$'`1);')
 m4_define(`hm_arg_fdset_io', `Tvbf(" $'`1="); Tvbfdset($'`2,$'`1);')
 m4_define(`hm_arg_pollfds_io', `Tvbf(" $'`1="); Tvbpollfds($'`1,$'`2);')
 m4_define(`hm_arg_timeval_in_rel_null', `
  if ($'`1) Tvbf(" $'`1=%ld.%06ld",(long)$'`1->tv_sec,(long)$'`1->tv_usec);
  else Tvba(" $'`1=null");')
 m4_define(`hm_arg_must', `')
 m4_define(`hm_arg_socktype', `
  Tvbf($'`1==SOCK_STREAM ? " $'`1=SOCK_STREAM" : " $'`1=SOCK_DGRAM");')
 m4_define(`hm_arg_ign', `')
 m4_define(`hm_arg_fd', `Tvbf(" $'`1=%d",$'`1);')
 m4_define(`hm_arg_fcntl_cmd_arg', `
  if ($'`1 == F_SETFL) {
   Tvbf(" $'`1=F_SETFL %s",arg & O_NONBLOCK ? "O_NONBLOCK|..." : "~O_NONBLOCK&...");
  } else if ($'`1 == F_GETFL) {
   Tvba(" $'`1=F_GETFL");
  } else {
   Tmust("$'`1","F_GETFL/F_SETFL",0);
  }')
 m4_define(`hm_arg_addr_in', `Tvba(" $'`1="); Tvbaddr($'`1,$'`2);')
 m4_define(`hm_arg_bytes_in', `')
 m4_define(`hm_arg_bytes_out', `Tvbf(" $'`4=%lu",(unsigned long)$'`4);')
 m4_define(`hm_arg_addr_out', `Tvbf(" *$'`2=%d",$'`2);')
  $3

 hm_create_nothing
 m4_define(`hm_arg_bytes_in', `Tvbbytes($'`2,$'`4);')
  $3

  Q_vb();
}
')

m4_include(`hsyscalls.i4')


void Tvbaddr(const struct sockaddr *addr, int len) {
  const struct sockaddr_in *ai= (const struct sockaddr_in*)addr;
  
  assert(len==sizeof(struct sockaddr_in));
  assert(ai->sin_family==AF_INET);
  Tvbf("%s:%u",inet_ntoa(ai->sin_addr),htons(ai->sin_port));
}

void Tvbbytes(const void *buf, int len) {
  const byte *bp;
  int i;

  if (!len) { Tvba("\n     ."); return; }
  for (i=0, bp=buf; i<len; i++, bp++) {
    if (!(i&31)) Tvba("\n     ");
    else if (!(i&3)) Tvba(" ");
    Tvbf("%02x",*bp);
  }
  Tvba(".");
}

void Tvbfdset(int max, const fd_set *fds) {
  int i;
  const char *comma= "";
  
  Tvba("[");
  for (i=0; i<max; i++) {
    if (!FD_ISSET(i,fds)) continue;
    Tvba(comma);
    Tvbf("%d",i);
    comma= ",";
  }
  Tvba("]");
}

static void Tvbpollevents(int events) {
  const char *delim= "";

  events &= (POLLIN|POLLOUT|POLLPRI);
  if (!events) { Tvba("0"); return; }
  if (events & POLLIN) { Tvba("POLLIN"); delim= "|"; }
  if (events & POLLOUT) { Tvba(delim); Tvba("POLLOUT"); delim= "|"; }
  if (events & POLLPRI) { Tvba(delim); Tvba("POLLPRI"); }
}

void Tvbpollfds(const struct pollfd *fds, int nfds) {
  const char *comma= "";
  
  Tvba("[");
  while (nfds>0) {
    Tvba(comma);
    Tvbf("{fd=%d, events=",fds->fd);
    Tvbpollevents(fds->events);
    Tvba(", revents=");
    Tvbpollevents(fds->revents);
    Tvba("}");
    comma= ", ";
    nfds--; fds++;
  }
  Tvba("]");
}

void Tvberrno(int e) {
  const struct Terrno *te;

  for (te= Terrnos; te->n && te->v != e; te++);
  assert(te->n);
  Tvba(te->n);
}

void Tvba(const char *str) {
  if (!adns__vbuf_appendstr(&vb,str)) Tnomem();
}

void Tvbvf(const char *fmt, va_list al) {
  char buf[1000];
  buf[sizeof(buf)-2]= '\t';
  vsnprintf(buf,sizeof(buf),fmt,al);
  assert(buf[sizeof(buf)-2] == '\t');

  Tvba(buf);
}

void Tvbf(const char *fmt, ...) {
  va_list al;
  va_start(al,fmt);
  Tvbvf(fmt,al);
  va_end(al);
}


void Tmust(const char *call, const char *arg, int cond) {
  if (cond) return;
  fprintf(stderr,"adns test harness: case not handled: system call %s, arg %s",call,arg);
  exit(-1);
}

void Tfailed(const char *why) {
  fprintf(stderr,"adns test harness: failure: %s: %s\n",why,strerror(errno));
  exit(-1);
}

void Tnomem(void) {
  Tfailed("unable to malloc/realloc");
}

void Toutputerr(void) {
  Tfailed("write error on test harness output");
}

struct malloced {
  struct malloced *next, *back;
  size_t sz;
  unsigned long count;
  struct { double d; long ul; void *p; void (*fp)(void); } data;
};

static unsigned long malloccount, mallocfailat;
static struct { struct malloced *head, *tail; } mallocedlist;

#define MALLOCHSZ ((char*)&mallocedlist.head->data - (char*)mallocedlist.head)

void *Hmalloc(size_t sz) {
  struct malloced *newnode;
  const char *mfavar;
  char *ep;

  assert(sz);

  newnode= malloc(MALLOCHSZ + sz);  if (!newnode) Tnomem();

  LIST_LINK_TAIL(mallocedlist,newnode);
  newnode->sz= sz;
  newnode->count= ++malloccount;
  if (!mallocfailat) {
    mfavar= getenv("ADNS_REGRESS_MALLOCFAILAT");
    if (mfavar) {
      mallocfailat= strtoul(mfavar,&ep,10);
      if (!mallocfailat || *ep) Tfailed("ADNS_REGRESS_MALLOCFAILAT bad value");
    } else {
      mallocfailat= ~0UL;
    }
  }
  assert(newnode->count != mallocfailat);
  memset(&newnode->data,0xc7,sz);
  return &newnode->data;
}

void Hfree(void *ptr) {
  struct malloced *oldnode;

  if (!ptr) return;

  oldnode= (void*)((char*)ptr - MALLOCHSZ);
  LIST_UNLINK(mallocedlist,oldnode);
  memset(&oldnode->data,0x38,oldnode->sz);
  free(oldnode);
}

void *Hrealloc(void *op, size_t nsz) {
  struct malloced *oldnode;
  void *np;
  size_t osz;

  if (op) { oldnode= (void*)((char*)op - MALLOCHSZ); osz= oldnode->sz; } else { osz= 0; }
  np= Hmalloc(nsz);
  memcpy(np,op, osz>nsz ? nsz : osz);
  Hfree(op);
  return np;
}

void Hexit(int rv) {
  struct malloced *loopnode;

  Tshutdown();
  adns__vbuf_free(&vb);
  adns__vbuf_free(&vbw);
  if (mallocedlist.head) {
    fprintf(stderr,"adns test harness: memory leaked:");
    for (loopnode=mallocedlist.head; loopnode; loopnode=loopnode->next)
      fprintf(stderr," %lu(%lu)",loopnode->count,(unsigned long)loopnode->sz);
    putc('\n',stderr);
    if (ferror(stderr)) exit(-1);
  }
  exit(rv);
}
