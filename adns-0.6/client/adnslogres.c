/*
 * adnslogres.c
 * - a replacement for the Apache logresolve program using adns
 */
/*
 *  This file is
 *   Copyright (C) 1999 Tony Finch <dot@dotat.at>
 *   Copyright (C) 1999 Ian Jackson <ian@davenant.greenend.org.uk>
 *
 *  It is part of adns, which is
 *    Copyright (C) 1997-1999 Ian Jackson <ian@davenant.greenend.org.uk>
 *    Copyright (C) 1999 Tony Finch <dot@dotat.at>
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software Foundation,
 *  Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * This version was originally supplied by Tony Finch, but has been
 * modified by Ian Jackson as it was incorporated into adns.
 */

static const char * const cvsid =
	"$Id: adnslogres.c,v 1.1.1.1 2000-02-16 17:32:28 sdyoung Exp $";

#include <sys/types.h>
#include <sys/time.h>

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#include "adns.h"

/* maximum number of concurrent DNS queries */
#define MAXPENDING 1000

/* maximum length of a line */
#define MAXLINE 1024

/* option flags */
#define OPT_DEBUG 1
#define OPT_POLL 2

static const char *progname;

#define msg(fmt, args...) fprintf(stderr, "%s: " fmt "\n", progname, ##args)

static void aargh(const char *cause) {
  const char *why = strerror(errno);
  if (!why) why = "Unknown error";
  msg("%s: %s (%d)", cause, why, errno);
  exit(1);
}

/*
 * Parse the IP address and convert to a reverse domain name.
 */
static char *ipaddr2domain(char *start, char **addr, char **rest) {
  static char buf[30]; /* "123.123.123.123.in-addr.arpa.\0" */
  char *ptrs[5];
  int i;

  ptrs[0]= start;
retry:
  while (!isdigit(*ptrs[0]))
    if (!*ptrs[0]++) {
      strcpy(buf, "invalid.");
      *addr= *rest= NULL;
      return buf;
    }
  for (i= 1; i < 5; i++) {
    ptrs[i]= ptrs[i-1];
    while (isdigit(*ptrs[i]++));
    if ((i == 4 && !isspace(ptrs[i][-1])) ||
	(i != 4 && ptrs[i][-1] != '.') ||
	(ptrs[i]-ptrs[i-1] > 4)) {
      ptrs[0]= ptrs[i]-1;
      goto retry;
    }
  }
  sprintf(buf, "%.*s.%.*s.%.*s.%.*s.in-addr.arpa.",
	  ptrs[4]-ptrs[3]-1, ptrs[3],
	  ptrs[3]-ptrs[2]-1, ptrs[2],
	  ptrs[2]-ptrs[1]-1, ptrs[1],
	  ptrs[1]-ptrs[0]-1, ptrs[0]);
  *addr= ptrs[0];
  *rest= ptrs[4]-1;
  return buf;
}

static void printline(FILE *outf, char *start, char *addr, char *rest, char *domain) {
  if (domain)
    fprintf(outf, "%.*s%s%s", addr - start, start, domain, rest);
  else
    fputs(start, outf);
  if (ferror(outf)) aargh("write output");
}

typedef struct logline {
  struct logline *next;
  char *start, *addr, *rest;
  adns_query query;
} logline;

static logline *readline(FILE *inf, adns_state adns, int opts) {
  static char buf[MAXLINE];
  char *str;
  logline *line;

  if (fgets(buf, MAXLINE, inf)) {
    str= malloc(sizeof(*line) + strlen(buf) + 1);
    if (!str) aargh("malloc");
    line= (logline*)str;
    line->next= NULL;
    line->start= str+sizeof(logline);
    strcpy(line->start, buf);
    str= ipaddr2domain(line->start, &line->addr, &line->rest);
    if (opts & OPT_DEBUG)
      msg("submitting %.*s -> %s", line->rest-line->addr, line->addr, str);
    if (adns_submit(adns, str, adns_r_ptr,
		    adns_qf_quoteok_cname|adns_qf_cname_loose,
		    NULL, &line->query))
      aargh("adns_submit");
    return line;
  }
  if (!feof(inf))
    aargh("fgets");
  return NULL;
}
	
static void proclog(FILE *inf, FILE *outf, int opts) {
  int eof, err, len;
  adns_state adns;
  adns_answer *answer;
  logline *head, *tail, *line;

  errno= adns_init(&adns, (opts & OPT_DEBUG) ? adns_if_debug : 0, 0);
  if (errno) aargh("adns_init");
  head= tail= readline(inf, adns, opts);
  len= 1; eof= 0;
  while (head) {
    if (opts & OPT_DEBUG)
      msg("%d in queue; checking %.*s", len,
	  head->rest-head->addr, head->addr);
    if (eof || len > MAXPENDING)
      if (opts & OPT_POLL)
	err= adns_wait_poll(adns, &head->query, &answer, NULL);
      else
	err= adns_wait(adns, &head->query, &answer, NULL);
    else
      err= adns_check(adns, &head->query, &answer, NULL);
    if (err != EAGAIN) {
      printline(outf, head->start, head->addr, head->rest,
		answer->status == adns_s_ok ? *answer->rrs.str : NULL);
      line= head; head= head->next;
      free(line); free(answer);
      len--;
    }
    if (!eof) {
      line= readline(inf, adns, opts);
      if (!line)
	eof= 1;
      else {
	if (!head)
	  head= line;
	else
	  tail->next= line;
	tail= line;
	len++;
      }
    }
  }
  adns_finish(adns);
}

static void usage(void) {
  fprintf(stderr, "usage: %s [-d] [-p] [logfile]\n", progname);
  exit(1);
}

int main(int argc, char *argv[]) {
  int c, opts;
  FILE *inf;

  progname= strrchr(*argv, '/');
  if (progname)
    progname++;
  else
    progname= *argv;
  opts= 0;

  while ((c= getopt(argc, argv, "dp")) != -1)
    switch (c) {
    case 'd':
      opts|= OPT_DEBUG;
      break;
    case 'p':
      opts|= OPT_POLL;
      break;
    default:
      usage();
    }

  argc-= optind;
  argv+= optind;

  inf= NULL;
  if (argc == 0)
    inf= stdin;
  else if (argc == 1)
    inf= fopen(*argv, "r");
  else
    usage();

  if (!inf)
    aargh("couldn't open input");

  proclog(inf, stdout, opts);

  if (fclose(inf))
    aargh("fclose input");
  if (fclose(stdout))
    aargh("fclose output");

  return 0;
}
