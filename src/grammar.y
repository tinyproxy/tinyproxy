/* $Id: grammar.y,v 1.2 2001-05-27 02:25:21 rjkaes Exp $
 *
 * This is the grammar for tinyproxy's configuration file. It needs to be
 * in sync with scanner.l. If you know more about yacc and lex than I do
 * please update these files.
 *
 * Copyright (C) 2000  Robert James Kaes (rjkaes@flarenet.com)
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

%{

#include "tinyproxy.h"

#include "acl.h"
#include "anonymous.h"
#include "log.h"
#include "thread.h"

void yyerror(char *s);
int yylex(void);

%}

%union {
	unsigned int num;
	char *cptr;
	void *ptr;
}

/* statements */
%token KW_PORT KW_LISTEN
%token KW_LOGFILE KW_PIDFILE KW_SYSLOG
%token KW_MAXCLIENTS KW_MAXSPARESERVERS KW_MINSPARESERVERS KW_STARTSERVERS
%token KW_MAXREQUESTSPERCHILD
%token KW_TIMEOUT
%token KW_USER KW_GROUP
%token KW_ANONYMOUS KW_FILTER KW_XTINYPROXY KW_TUNNEL
%token KW_ALLOW KW_DENY

/* yes/no switches */
%token KW_YES KW_NO

%token <cptr> IDENTIFIER
%token <num>  NUMBER
%token <cptr> STRING
%token <cptr> NUMERIC_ADDRESS
%token <cptr> STRING_ADDRESS
%token <cptr> NETMASK_ADDRESS

%type <num> yesno
%type <cptr> string
%type <cptr> network_address
%type <cptr> unique_address

%%

start
	: /* empty */
	| start line
	;

line
	: '\n'
	| statement '\n'
	;

statement
        : KW_PORT NUMBER		{ config.port = $2; }
	| KW_TIMEOUT NUMBER		{ config.idletimeout = $2; }
	| KW_SYSLOG yesno
	  {
#ifdef HAVE_SYSLOG_H
	          config.syslog = $2;
#else
		  log_message(LOG_WARNING, "Syslog support was not compiled in.");
#endif
	  }
	| KW_MAXCLIENTS NUMBER		{ thread_configure(THREAD_MAXCLIENTS, $2); }
	| KW_MAXSPARESERVERS NUMBER	{ thread_configure(THREAD_MAXSPARESERVERS, $2); }
	| KW_MINSPARESERVERS NUMBER	{ thread_configure(THREAD_MINSPARESERVERS, $2); }
	| KW_STARTSERVERS NUMBER	{ thread_configure(THREAD_STARTSERVERS, $2); }
	| KW_MAXREQUESTSPERCHILD NUMBER	{ thread_configure(THREAD_MAXREQUESTSPERCHILD, $2); }
        | KW_LOGFILE string
	  {
	          config.logf_name = $2;
		  if (!config.logf_name) {
		          fprintf(stderr, "bad log file\n");
		  }
	  }
	| KW_PIDFILE string		{ config.pidpath = $2; }
	| KW_USER string		{ config.username = $2; }
	| KW_GROUP string		{ config.group = $2; }
	| KW_ANONYMOUS string		{ anon_insert($2); }
	| KW_FILTER string
	  {
#ifdef FILTER_ENABLE
		  config.filter = $2;
#else
	          log_message(LOG_WARNING, "Filter support was not compiled in.");
#endif
	  }
	| KW_XTINYPROXY network_address	{ config.my_domain = $2; }
	| KW_TUNNEL unique_address ':' NUMBER
	  {
#ifdef TUNNEL_SUPPORT
	          config.tunnel_name = $2;
		  config.tunnel_port = $4;
#else
		  log_message(LOG_WARNING, "Tunnel support was not compiled in.");
#endif
	  }
	| KW_LISTEN NUMERIC_ADDRESS	{ config.ipAddr = $2; }
	| KW_ALLOW network_address	{ insert_acl($2, ACL_ALLOW); }
	| KW_DENY network_address	{ insert_acl($2, ACL_DENY); }
	;

network_address
	: unique_address
	| NETMASK_ADDRESS
	;

unique_address
	: STRING_ADDRESS
	| NUMERIC_ADDRESS
	;

yesno
	: KW_YES			{ $$ = 1; }
	| KW_NO				{ $$ = 0; }
	| NUMBER			{ $$ = $1; }
	;

string
	: IDENTIFIER
	| STRING
	;

%%

extern unsigned int yylineno;

void yyerror(char *s)
{
	fprintf(stderr, "Line %d: %s\n", yylineno, s);
}
