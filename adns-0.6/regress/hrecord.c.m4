m4_dnl hrecord.c.m4
m4_dnl (part of complex test harness, not of the library)
m4_dnl - recording routines

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

#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

#include "harness.h"

static FILE *Toutputfile;

void Tshutdown(void) {
}

static void R_recordtime(void) {
  int r;
  struct timeval tv, tvrel;

  Tensureoutputfile();
  r= gettimeofday(&tv,0); if (r) Tfailed("gettimeofday syscallbegin");
  tvrel.tv_sec= tv.tv_sec - currenttime.tv_sec;
  tvrel.tv_usec= tv.tv_usec - currenttime.tv_usec;
  if (tv.tv_usec < 0) { tvrel.tv_usec += 1000000; tvrel.tv_sec--; }
  Tvbf("\n +%ld.%06ld",(long)tvrel.tv_sec,(long)tvrel.tv_usec);
  currenttime= tv;
}

void Tensureoutputfile(void) {
  const char *fdstr;
  int fd, r;

  if (Toutputfile) return;
  
  Toutputfile= stdout;
  fdstr= getenv("ADNS_TEST_OUT_FD");
  if (fdstr) {
    fd= atoi(fdstr);
    Toutputfile= fdopen(fd,"a"); if (!Toutputfile) Tfailed("fdopen ADNS_TEST_OUT_FD");
  }

  r= gettimeofday(&currenttime,0); if (r) Tfailed("gettimeofday syscallbegin");
  if (fprintf(Toutputfile," start %ld.%06ld\n",
	      (long)currenttime.tv_sec,(long)currenttime.tv_usec) == EOF) Toutputerr();
}

void Q_vb(void) {
  if (!adns__vbuf_append(&vb,"",1)) Tnomem();
  Tensureoutputfile();
  if (fprintf(Toutputfile," %s\n",vb.buf) == EOF) Toutputerr();
  if (fflush(Toutputfile)) Toutputerr();
}

static void R_vb(void) {
  Q_vb();
}

m4_define(`hm_syscall', `
 hm_create_proto_h
int H$1(hm_args_massage($3,void)) {
 int r, e;

 hm_create_hqcall_vars
 $3

 hm_create_hqcall_init($1)
 $3

 hm_create_hqcall_args
 Q$1(hm_args_massage($3));

 hm_create_realcall_args
 r= $1(hm_args_massage($3));
 e= errno;

 vb.used= 0;
 Tvba("$1=");
 m4_define(`hm_rv_any',`
  if (r==-1) { Tvberrno(e); goto x_error; }
  Tvbf("%d",r);')
 m4_define(`hm_rv_fd',`hm_rv_any($'`1)')
 m4_define(`hm_rv_succfail',`
  if (r) { Tvberrno(e); goto x_error; }
  Tvba("OK");')
 m4_define(`hm_rv_must',`Tmust("$1","return",!r); Tvba("OK");')
 m4_define(`hm_rv_len',`
  if (r==-1) { Tvberrno(e); goto x_error; }
  Tmust("$1","return",r<=$'`1);
  Tvba("OK");')
 m4_define(`hm_rv_fcntl',`
  if (r==-1) { Tvberrno(e); goto x_error; }
  if (cmd == F_GETFL) {
    Tvbf(r & O_NONBLOCK ? "O_NONBLOCK|..." : "~O_NONBLOCK&...");
  } else {
    if (cmd == F_SETFL) {
      Tmust("$1","return",!r);
    } else {
      Tmust("cmd","F_GETFL/F_SETFL",0);
    }
    Tvba("OK");
  }')
 $2

 hm_create_nothing
 m4_define(`hm_arg_fdset_io',`Tvba(" $'`1="); Tvbfdset($'`2,$'`1);')
 m4_define(`hm_arg_pollfds_io',`Tvba(" $'`1="); Tvbpollfds($'`1,$'`2);')
 m4_define(`hm_arg_addr_out',`Tvba(" $'`1="); Tvbaddr($'`1,*$'`2);')
 $3

 hm_create_nothing
 m4_define(`hm_arg_bytes_out',`Tvbbytes($'`2,r);')
 $3

 m4_define(`hm_rv_any',`x_error:')
 m4_define(`hm_rv_fd',`x_error:')
 m4_define(`hm_rv_succfail',`x_error:')
 m4_define(`hm_rv_len',`x_error:')
 m4_define(`hm_rv_fcntl',`x_error:')
 m4_define(`hm_rv_must',`')
 $2

 R_recordtime();
 R_vb();
 errno= e;
 return r;
}
')

m4_include(`hsyscalls.i4')
