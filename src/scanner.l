/* $Id: scanner.l,v 1.22 2003-06-26 18:26:10 rjkaes Exp $
 *
 * This builds the scanner for the tinyproxy configuration file. This
 * file needs to stay in sync with grammar.y. If someone knows lex and yacc
 * better than I do, please update these files.
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

#include "grammar.h"

struct keyword {
        char *kw_name;
        int kw_token;
};

static struct keyword keywords[] = {
        /* statements */
        { "port",		 KW_PORT },
	{ "logfile",		 KW_LOGFILE },
	{ "syslog",		 KW_SYSLOG },
	{ "maxclients",		 KW_MAXCLIENTS },
	{ "maxspareservers",	 KW_MAXSPARESERVERS },
	{ "minspareservers",	 KW_MINSPARESERVERS },
	{ "startservers",	 KW_STARTSERVERS },
	{ "maxrequestsperchild", KW_MAXREQUESTSPERCHILD },
	{ "pidfile",		 KW_PIDFILE },
	{ "timeout",		 KW_TIMEOUT },
	{ "listen",		 KW_LISTEN },
	{ "user",		 KW_USER },
	{ "group",		 KW_GROUP },
	{ "anonymous",		 KW_ANONYMOUS },
	{ "filter",		 KW_FILTER },
        { "filterurls",          KW_FILTERURLS },
        { "filterextended",      KW_FILTEREXTENDED },
        { "filterdefaultdeny",   KW_FILTER_DEFAULT_DENY },
        { "filtercasesensitive", KW_FILTER_CASESENSITIVE },
	{ "xtinyproxy",		 KW_XTINYPROXY },
        { "upstream",            KW_UPSTREAM },
	{ "allow",		 KW_ALLOW },
        { "deny",                KW_DENY },
        { "connectport",         KW_CONNECTPORT },
        { "bind",                KW_BIND },
        { "viaproxyname",        KW_VIA_PROXY_NAME },
        { "stathost",            KW_STATHOST },
	{ "errorfile",	         KW_ERRORPAGE },
	{ "defaulterrorfile",    KW_DEFAULT_ERRORPAGE },
	{ "statfile",            KW_STATPAGE },

        /* loglevel and the settings */
        { "loglevel",            KW_LOGLEVEL },
	{ "critical",		 KW_LOG_CRITICAL },
	{ "error",		 KW_LOG_ERROR },
	{ "warning",		 KW_LOG_WARNING },
	{ "notice",		 KW_LOG_NOTICE },
	{ "connect",		 KW_LOG_CONNECT },
	{ "info",		 KW_LOG_INFO },

	/* on/off switches */
	{ "yes",		 KW_YES },
	{ "on",			 KW_YES },
	{ "no",			 KW_NO },
	{ "off",		 KW_NO }
	
}; 

#define YY_NO_UNPUT 1

#define MAX_REGEXP_LEN	1024

char tiny_buf[MAX_REGEXP_LEN];
char *tiny_str;

static int check_reserved_words(char *token);
static void append_string(int length, char *str);
static void append_char(char c);

%}

%option noyywrap batch yylineno

white		[ \t]
digit		[0-9]
alpha		[a-zA-Z]
alphanum	[a-zA-Z0-9]
word		[^ \#'"\(\)\{\}\\;\n\t,|\.]

%x string

%%

\#.*$		        ;
\n                      { return '\n'; }
":"                     { return ':'; }
{white}+                ;
0x{digit}+              { yylval.num = strtol(yytext, NULL, 16); return NUMBER; }
0{digit}+               { yylval.num = strtol(yytext, NULL, 8); return NUMBER; }
{digit}+                { yylval.num = atoi(yytext); return NUMBER; }
{alpha}({alphanum}|[-._])+	        { return check_reserved_words(yytext); }

\"		{
			        tiny_str = tiny_buf;
				BEGIN(string);
			}
<string>\\a		{ append_char(7); }
<string>\\n		{ append_char(10); }
<string>\\r		{ append_char(13); }
<string>\\t		{ append_char(9); }
<string>\\v		{ append_char(11); }
<string>\\[^anrtv]	{ append_string(1, yytext + 1); }
<string>\"		{
				BEGIN(INITIAL);
				yylval.cptr = strdup(tiny_buf);
				return STRING;
			}
<string>[^"\\]+		{ append_string(strlen(yytext), yytext); }


({digit}{1,3}\.){3}{digit}{1,3} { yylval.cptr = strdup(yytext); return NUMERIC_ADDRESS; }
({digit}{1,3}\.){3}{digit}{1,3}\/{digit}+ { yylval.cptr = strdup(yytext); return NETMASK_ADDRESS; }


%%

int
check_reserved_words(char *token)
{
	int i;

	for (i = 0; i < (sizeof(keywords) / sizeof(struct keyword)); i++) {
	        if (strcasecmp(keywords[i].kw_name, token) == 0) {
		        return keywords[i].kw_token;
		}
	}
	yylval.cptr = strdup(token);
	return IDENTIFIER;
}

static void
append_string(int length, char *s)
{
	int to_copy = min(MAX_REGEXP_LEN - (tiny_str - tiny_buf) - 1, length);

	memcpy(tiny_str, s, to_copy);
	tiny_str += to_copy;
	*tiny_str = 0;
}

static void
append_char(char c)
{
	*tiny_str = c;
	tiny_str++;
	*tiny_str = 0;
}
