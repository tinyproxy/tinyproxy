/* $Id: grammar.y,v 1.13 2002-05-27 01:52:44 rjkaes Exp $
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
#include "child.h"
#include "log.h"
#include "reqs.h"

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
%token KW_ANONYMOUS KW_XTINYPROXY
%token KW_FILTER KW_FILTERURLS KW_FILTEREXTENDED
%token KW_TUNNEL KW_UPSTREAM
%token KW_CONNECTPORT KW_BIND
%token KW_ALLOW KW_DENY

/* yes/no switches */
%token KW_YES KW_NO

/* settings for loglevel */
%token KW_LOGLEVEL
%token KW_LOG_CRITICAL KW_LOG_ERROR KW_LOG_WARNING KW_LOG_NOTICE KW_LOG_CONNECT KW_LOG_INFO

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
%type <num> loglevels

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
	| KW_MAXCLIENTS NUMBER		{ child_configure(CHILD_MAXCLIENTS, $2); }
	| KW_MAXSPARESERVERS NUMBER	{ child_configure(CHILD_MAXSPARESERVERS, $2); }
	| KW_MINSPARESERVERS NUMBER	{ child_configure(CHILD_MINSPARESERVERS, $2); }
	| KW_STARTSERVERS NUMBER	{ child_configure(CHILD_STARTSERVERS, $2); }
	| KW_MAXREQUESTSPERCHILD NUMBER	{ child_configure(CHILD_MAXREQUESTSPERCHILD, $2); }
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
	| KW_ANONYMOUS string		{ anonymous_insert($2); }
	| KW_FILTER string
	  {
#ifdef FILTER_ENABLE
		  config.filter = $2;
#else
	          log_message(LOG_WARNING, "Filter support was not compiled in.");
#endif
	  }
        | KW_FILTERURLS yesno
          {
#ifdef FILTER_ENABLE
		  config.filter_url = $2;
#else
		  log_message(LOG_WARNING, "Filter support wss not compiled in.");
#endif
	  }
        | KW_FILTEREXTENDED yesno
          {
#ifdef FILTER_ENABLE
		  config.filter_extended = $2;
#else
		  log_message(LOG_WARNING, "Filter support was not compiled in.");
#endif
	  }
	| KW_XTINYPROXY network_address
	   {
#ifdef XTINYPROXY_ENABLE
	   	  config.my_domain = $2;
#else
		  log_message(LOG_WARNING, "X-Tinyproxy header support was not compiled in.");
#endif
	   }
	| KW_TUNNEL unique_address ':' NUMBER
	  {
#ifdef TUNNEL_SUPPORT
	          config.tunnel_name = $2;
		  config.tunnel_port = $4;
#else
		  log_message(LOG_WARNING, "Tunnel support was not compiled in.");
#endif
	  }
        | KW_UPSTREAM unique_address ':' NUMBER
          {
#ifdef UPSTREAM_SUPPORT
                  config.upstream_name = $2;
                  config.upstream_port = $4;
#else
                  log_message(LOG_WARNING, "Upstream proxy support was not compiled in.");
#endif
          }
	| KW_LISTEN NUMERIC_ADDRESS
          {
		  log_message(LOG_INFO, "Establishing listening socket on IP %s", $2);
                  config.ipAddr = $2;
          }
	| KW_ALLOW network_address	{ insert_acl($2, ACL_ALLOW); }
	| KW_DENY network_address	{ insert_acl($2, ACL_DENY); }
        | KW_LOGLEVEL loglevels         { set_log_level($2); }
        | KW_CONNECTPORT NUMBER         { add_connect_port_allowed($2); }
        | KW_BIND NUMERIC_ADDRESS
          {
		  log_message(LOG_INFO, "Binding outgoing connections to %s", $2);
	          config.bind_address = $2;
          }
	;

loglevels
        : KW_LOG_CRITICAL               { $$ = LOG_CRIT; }
        | KW_LOG_ERROR                  { $$ = LOG_ERR; }
        | KW_LOG_WARNING                { $$ = LOG_WARNING; }
        | KW_LOG_NOTICE                 { $$ = LOG_NOTICE; }
        | KW_LOG_CONNECT                { $$ = LOG_CONN; }
        | KW_LOG_INFO                   { $$ = LOG_INFO; }
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

void
yyerror(char *s)
{
	fprintf(stderr, "Line %d: %s\n", yylineno, s);
}
