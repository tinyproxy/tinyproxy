/* $Id: grammar.y,v 1.23 2003-06-26 18:17:09 rjkaes Exp $
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
#include "filter.h"
#include "htmlerror.h"
#include "log.h"
#include "reqs.h"

void yyerror(char *s);
int yylex(void);

%}

%union {
	unsigned int num;
	char *cptr;
}

/* statements */
%token KW_PORT KW_LISTEN
%token KW_LOGFILE KW_PIDFILE KW_SYSLOG
%token KW_MAXCLIENTS KW_MAXSPARESERVERS KW_MINSPARESERVERS KW_STARTSERVERS
%token KW_MAXREQUESTSPERCHILD
%token KW_TIMEOUT
%token KW_USER KW_GROUP
%token KW_ANONYMOUS KW_XTINYPROXY
%token KW_FILTER KW_FILTERURLS KW_FILTEREXTENDED KW_FILTER_DEFAULT_DENY
%token KW_FILTER_CASESENSITIVE
%token KW_UPSTREAM
%token KW_CONNECTPORT KW_BIND
%token KW_STATHOST
%token KW_ALLOW KW_DENY
%token KW_ERRORPAGE KW_DEFAULT_ERRORPAGE
%token KW_STATPAGE
%token KW_VIA_PROXY_NAME

/* yes/no switches */
%token KW_YES KW_NO

/* settings for loglevel */
%token KW_LOGLEVEL
%token KW_LOG_CRITICAL KW_LOG_ERROR KW_LOG_WARNING KW_LOG_NOTICE KW_LOG_CONNECT KW_LOG_INFO

%token <cptr> IDENTIFIER
%token <num>  NUMBER
%token <cptr> STRING
%token <cptr> NUMERIC_ADDRESS
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
	| KW_ERRORPAGE NUMBER string	{ add_new_errorpage($3, $2); }
	| KW_DEFAULT_ERRORPAGE string	{ config.errorpage_undef = $2; }
	| KW_STATPAGE string	{ config.statpage = $2; }
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
        | KW_FILTER_CASESENSITIVE yesno
          {
#ifdef FILTER_ENABLE
		  config.filter_casesensitive = $2;
#else
		  log_message(LOG_WARNING, "Filter support was not compiled in.");
#endif
	  }
        | KW_FILTER_DEFAULT_DENY yesno
          {
#ifdef FILTER_ENABLE
		  if ($2)
			  filter_set_default_policy(FILTER_DEFAULT_DENY);
#else
		  log_message(LOG_WARNING, "FIlter support was not compiled in.");
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
        | KW_UPSTREAM unique_address ':' NUMBER
          {
#ifdef UPSTREAM_SUPPORT
		  upstream_add($2, $4, NULL);
#else
                  log_message(LOG_WARNING, "Upstream proxy support was not compiled in.");
#endif
          }
	| KW_UPSTREAM unique_address ':' NUMBER STRING
	  {
#ifdef UPSTREAM_SUPPORT
		  upstream_add($2, $4, $5);
#else
                  log_message(LOG_WARNING, "Upstream proxy support was not compiled in.");
#endif
	  }
	| KW_NO KW_UPSTREAM STRING
	  {
#ifdef UPSTREAM_SUPPORT
		  upstream_add(NULL, 0, $3);
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
#ifndef TRANSPARENT_PROXY
		  log_message(LOG_INFO, "Binding outgoing connections to %s", $2);
	          config.bind_address = $2;
#else
		  log_message(LOG_WARNING, "The 'Bind' directive can not be used with transparent proxy support.  Ignoring the directive.");
#endif
          }
        | KW_VIA_PROXY_NAME string
          {
		  log_message(LOG_INFO, "Setting \"Via\" proxy name to: %s", $2);
		  config.via_proxy_name = $2;
          }
        | KW_STATHOST string
          {
		  log_message(LOG_INFO, "Stathost is set to \"%s\"", $2);
		  config.stathost = $2;
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
	: IDENTIFIER
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

extern int yylineno;

void
yyerror(char *s)
{
	static int headerdisplayed = 0;

	if (!headerdisplayed) {
		fprintf(stderr, "Errors in configuration file:\n");
		headerdisplayed = 1;
	}

	fprintf(stderr, "\t%s:%d: %s\n", config.config_file, yylineno, s);
	exit(EXIT_FAILURE);
}
