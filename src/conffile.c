/* $Id: conffile.c,v 1.1 2004-08-13 20:19:50 rjkaes Exp $
 *
 * Parses the configuration file and sets up the config_s structure for
 * use by the application.  This file replaces the old grammar.y and
 * scannar.l files.  It takes up less space and _I_ think is easier to
 * add new directives to.  Who knows if I'm right though.
 *
 * Copyright (C) 2004  Robert James Kaes (rjkaes@users.sourceforge.net)
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

#include "tinyproxy.h"

#include "conffile.h"

#include "acl.h"
#include "anonymous.h"
#include "child.h"
#include "filter.h"
#include "heap.h"
#include "htmlerror.h"
#include "log.h"
#include "reqs.h"

/*
 * The configuration directives are defined in the structure below.  Each
 * directive requires a regular expression to match against, and a
 * function to call when the regex is matched.
 *
 * Below are defined certain constant regular expression strings that
 * can (and likely should) be used when building the regex for the
 * given directive.
 */
#define WS "[[:space:]]+"
#define STR "\"([^\"]+)\""
#define BOOL "(yes|on|no|off)"
#define INT "((0x)?[[:digit:]]+)"
#define ALNUM "([-a-z0-9._]+)"
#define IP "((([0-9]{1,3})\\.){3}[0-9]{1,3})"
#define IPMASK "(" IP "(/[[:digit:]]+)?)"
#define BEGIN "^[[:space:]]*"
#define END "[[:space:]]*$"

/*
 * Limit the maximum number of substring matches to a reasonably high
 * number.  Given the usual structure of the configuration file, sixteen
 * substring matches should be plenty.
 */
#define RE_MAX_MATCHES 16


/*
 * All configuration handling functions are REQUIRED to be defined
 * with the same function template as below.
 */
typedef int (*CONFFILE_HANDLER)(struct config_s*, const char*, regmatch_t[]);

/*
 * Define the pattern used by any directive handling function.  The
 * following arguments are defined:
 *
 *   struct config_s* conf   pointer to the current configuration structure
 *   const char* line          full line matched by the regular expression
 *   regmatch_t match[]        offsets to the substrings matched
 *
 * The handling function must return 0 if the directive was processed
 * properly.  Any errors are reported by returning a non-zero value.
 */
#define HANDLE_FUNC(func) int func(struct config_s* conf, const char* line, regmatch_t match[])


/*
 * This is a do nothing function used for the comment and blank lines
 * in the configuration file.  We don't do anything for those, but
 * the function pointer needs to be defined to something so we simply
 * return true for those lines.
 */
static HANDLE_FUNC(handle_nop)
{
        return 0;
}

/*
 * List all the handling functions.  These are defined later, but they need
 * to be in-scope before the big structure below.
 */

static HANDLE_FUNC(handle_allow);
static HANDLE_FUNC(handle_anonymous);
static HANDLE_FUNC(handle_bind);
static HANDLE_FUNC(handle_bindsame);
static HANDLE_FUNC(handle_connectport);
static HANDLE_FUNC(handle_defaulterrorfile);
static HANDLE_FUNC(handle_deny);
static HANDLE_FUNC(handle_errorfile);
static HANDLE_FUNC(handle_errorfile);
#ifdef FILTER_ENABLE
static HANDLE_FUNC(handle_filter);
static HANDLE_FUNC(handle_filtercasesensitive);
static HANDLE_FUNC(handle_filterdefaultdeny);
static HANDLE_FUNC(handle_filterextended);
static HANDLE_FUNC(handle_filterurls);
#endif
static HANDLE_FUNC(handle_group);
static HANDLE_FUNC(handle_listen);
static HANDLE_FUNC(handle_logfile);
static HANDLE_FUNC(handle_logfile);
static HANDLE_FUNC(handle_loglevel);
static HANDLE_FUNC(handle_maxclients);
static HANDLE_FUNC(handle_maxrequestsperchild);
static HANDLE_FUNC(handle_maxspareservers);
static HANDLE_FUNC(handle_minspareservers);
static HANDLE_FUNC(handle_pidfile);
static HANDLE_FUNC(handle_port);
#ifdef REVERSE_SUPPORT
static HANDLE_FUNC(handle_reversebaseurl);
static HANDLE_FUNC(handle_reversemagic);
static HANDLE_FUNC(handle_reverseonly);
static HANDLE_FUNC(handle_reversepath);
#endif
static HANDLE_FUNC(handle_startservers);
static HANDLE_FUNC(handle_statfile);
static HANDLE_FUNC(handle_stathost);
static HANDLE_FUNC(handle_syslog);
static HANDLE_FUNC(handle_timeout);
//static HANDLE_FUNC(handle_upstream);
static HANDLE_FUNC(handle_user);
static HANDLE_FUNC(handle_viaproxyname);
#ifdef XTINYPROXY_ENABLE
static HANDLE_FUNC(handle_xtinyproxy);
#endif


/*
 * This macro can be used to make standard directives in the form:
 *   directive arguments [arguments ...]
 *
 * The directive itself will be the first matched substring.
 *
 * Note that this macro is not required.  As you can see below, the
 * comment and blank line elements are defined explicitly since they
 * do not follow the pattern above.  This macro is for convenience
 * only.
 */
#define STDCONF(d, re, func) { BEGIN "(" d ")" WS re END, func, NULL }


/*
 * Holds the regular expression used to match the configuration directive,
 * the function pointer to the rountine to handle the directive, and
 * for internal use, a pointer to the compiled regex so it only needs
 * to be compiled one.
 */
struct {
        const char* re;
        CONFFILE_HANDLER handler;
        regex_t* cre;
} directives[] = {
        /* comments */
        { BEGIN "#", handle_nop },
        
        /* blank lines */
        { "^[[:space:]]+$", handle_nop },
        
        /* string arguments */
        STDCONF("logfile", STR, handle_logfile),
        STDCONF("pidfile", STR, handle_pidfile),
        STDCONF("anonymous", STR, handle_anonymous),
        STDCONF("viaproxyname", STR, handle_viaproxyname),
        STDCONF("defaulterrorfile", STR, handle_defaulterrorfile),
        STDCONF("statfile", STR, handle_statfile),
        STDCONF("stathost", STR, handle_stathost),
        STDCONF("xtinyproxy", STR, handle_xtinyproxy),

        /* boolean arguments */
        STDCONF("syslog", BOOL, handle_syslog),
        STDCONF("bindsame", BOOL, handle_bindsame),

        /* integer arguments */
        STDCONF("port", INT, handle_port),
        STDCONF("maxclients", INT, handle_maxclients),
        STDCONF("maxspareservers", INT, handle_maxspareservers),
        STDCONF("minspareservers", INT, handle_minspareservers),
        STDCONF("startservers", INT, handle_startservers),
        STDCONF("maxrequestsperchild", INT, handle_maxrequestsperchild),
        STDCONF("timeout", INT, handle_timeout),
        STDCONF("connectport", INT, handle_connectport),

        /* alphanumeric arguments */
        STDCONF("user", ALNUM, handle_user),
        STDCONF("group", ALNUM, handle_group),

        /* ip arguments */
        STDCONF("listen", IP, handle_listen),
        STDCONF("allow", "(" IPMASK "|" ALNUM ")", handle_allow),
        STDCONF("deny", "(" IPMASK "|" ALNUM ")", handle_deny),
        STDCONF("bind", IP, handle_bind),
          
        /* error files */
        STDCONF("errorfile", INT WS STR, handle_errorfile),

#ifdef FILTER_ENABLE
        STDCONF("filter", STR, handle_filter),
        STDCONF("filterurls", BOOL, handle_filterurls),
        STDCONF("filterextended", BOOL, handle_filterextended),
        STDCONF("filterdefaultdeny", BOOL, handle_filterdefaultdeny),
        STDCONF("filtercasesensitive", BOOL, handle_filtercasesensitive),
#endif


#ifdef REVERSE_SUPPORT
        /* Reverse proxy arguments */
        STDCONF("reversebaseurl", STR, handle_reversebaseurl),
        STDCONF("reverseonly", BOOL, handle_reverseonly),
        STDCONF("reversemagic", BOOL, handle_reversemagic),
        STDCONF("reversepath", STR WS STR, handle_reversepath),
#endif

        /* upstream is rather complicated */
//        { BEGIN "(no[[:space:]]+)?upstream" WS, handle_upstream },

        /* loglevel */
        STDCONF("loglevel", "(critical|error|warning|notice|connect|info)", handle_loglevel)
};
const unsigned int ndirectives = sizeof(directives)/sizeof(directives[0]);

/*
 * Compiles the regular expressions used by the configuration file.  This
 * routine MUST be called before trying to parse the configuration file.
 *
 * Returns 0 on success; negative upon failure.
 */
int
config_compile(void)
{
        int i, r;

        for (i = 0; i != ndirectives; ++i) {
                assert(!directives[i].cre);
                
                directives[i].cre = safemalloc(sizeof(regex_t));
                if (!directives[i].cre)
                        return -1;

                r = regcomp(directives[i].cre,
                            directives[i].re,
                            REG_EXTENDED | REG_ICASE | REG_NEWLINE);
                
                if (r) return r;
        }
        return 0;
}


/*
 * Attempt to match the supplied line with any of the configuration
 * regexes defined above.  If a match is found, call the handler
 * function to process the directive.
 *
 * Returns 0 if a match was found and successfully processed; otherwise,
 * a negative number is returned.
 */
static int
check_match(struct config_s* conf, const char* line)
{
        regmatch_t match[RE_MAX_MATCHES];
        unsigned int i;

        assert(ndirectives > 0);

        for (i = 0; i != ndirectives; ++i) {
                assert(directives[i].cre);
                if (!regexec(directives[i].cre, line, RE_MAX_MATCHES, match, 0)) {
                        assert(directives[i].handler);
                        return (*directives[i].handler)(conf, line, match);
                }
        }

        return -1;
}

/*
 * Parse the previously opened configuration stream.
 */
int
config_parse(struct config_s* conf, FILE* f)
{
        char buffer[1024]; /* 1KB lines should be plenty */
        unsigned long lineno = 1;

        while (fgets(buffer, sizeof(buffer), f)) {
                if (check_match(conf, buffer)) {
                        printf("Problem with line %ld\n", lineno);
                        return 1;
                }
                ++lineno;
        }
        return 0;
}


/*
 * Functions to handle the various configuration file directives.
 */

/*
 * String arguments.
 */

static char*
get_string_arg(const char* line, regmatch_t* match)
{
        char *p;
        const unsigned int len = match->rm_eo - match->rm_so;

        assert(line);
        assert(len > 0);
        
        p = safemalloc(len + 1);
        if (!p)
                return NULL;

        memcpy(p, line + match->rm_so, len);
        p[len] = '\0';
        return p;
}

static int
set_string_arg(char** var, const char* line, regmatch_t* match)
{
        char* arg = get_string_arg(line, match);
        if (!arg)
                return -1;
        *var = safestrdup(arg);
        safefree(arg);
        return *var ? 0 : -1;
}

static HANDLE_FUNC(handle_logfile)
{
        return set_string_arg(&conf->logf_name, line, &match[2]);
}
static HANDLE_FUNC(handle_pidfile)
{
        return set_string_arg(&conf->pidpath, line, &match[2]);
}
static HANDLE_FUNC(handle_anonymous)
{
        char *arg = get_string_arg(line, &match[2]);
        if (!arg)
                return -1;

        anonymous_insert(arg);
        safefree(arg);
        return 0;
}
static HANDLE_FUNC(handle_viaproxyname)
{
        return set_string_arg(&conf->via_proxy_name, line, &match[2]);
}
static HANDLE_FUNC(handle_defaulterrorfile)
{
        return set_string_arg(&conf->errorpage_undef, line, &match[2]);
}
static HANDLE_FUNC(handle_statfile)
{
        return set_string_arg(&conf->statpage, line, &match[2]);
}
static HANDLE_FUNC(handle_stathost)
{
        return set_string_arg(&conf->stathost, line, &match[2]);
}
static HANDLE_FUNC(handle_xtinyproxy)
{
        return set_string_arg(&conf->my_domain, line, &match[2]);
}


/*
 * Boolean arguments.
 */

static int
get_bool_arg(const char* line, regmatch_t* match)
{
        assert(line);
        assert(match && match->rm_so != -1);

        const char* p = line + match->rm_so;

        /* "y"es or o"n" map as true, otherwise it's false. */
        if (tolower(p[0]) == 'y' || tolower(p[1]) == 'n')
                return 1;
        else
                return 0;
}

static int
set_bool_arg(unsigned int* var, const char* line, regmatch_t* match)
{
        assert(var);
        assert(line);
        assert(match && match->rm_so != -1);
        
        *var = get_bool_arg(line, match);
        return 0;
}

static HANDLE_FUNC(handle_syslog)
{
        return set_bool_arg(&conf->syslog, line, &match[2]);
}
static HANDLE_FUNC(handle_bindsame)
{
        return set_bool_arg(&conf->bindsame, line, &match[2]);
}


/*
 * Integer arguments.
 */

static inline long int
get_int_arg(const char* line, regmatch_t* match)
{
        assert(line);
        assert(match && match->rm_so != -1);
        
        return strtol(line + match->rm_so, NULL, 0);
}
static int
set_int_arg(int long* var, const char* line, regmatch_t* match)
{
        assert(var);
        assert(line);
        assert(match);
        
        *var = get_int_arg(line, match);
        return 0;
}
static HANDLE_FUNC(handle_port)
{
        return set_int_arg((long int*)&conf->port, line, &match[2]);
}
static HANDLE_FUNC(handle_maxclients)
{
        child_configure(CHILD_MAXCLIENTS, get_int_arg(line, &match[2]));
        return 0;
}
static HANDLE_FUNC(handle_maxspareservers)
{
        child_configure(CHILD_MAXSPARESERVERS, get_int_arg(line, &match[2]));
        return 0;
}
static HANDLE_FUNC(handle_minspareservers)
{
        child_configure(CHILD_MINSPARESERVERS, get_int_arg(line, &match[2]));
        return 0;
}
static HANDLE_FUNC(handle_startservers)
{
        child_configure(CHILD_STARTSERVERS, get_int_arg(line, &match[2]));
        return 0;
}
static HANDLE_FUNC(handle_maxrequestsperchild)
{
        child_configure(CHILD_MAXREQUESTSPERCHILD, get_int_arg(line, &match[2]));
        return 0;
}
static HANDLE_FUNC(handle_timeout)
{
        return set_int_arg((long int*)&conf->idletimeout, line, &match[2]);
}
static HANDLE_FUNC(handle_connectport)
{
        add_connect_port_allowed(get_int_arg(line, &match[2]));
        return 0;
}


/*
 * Alpha numeric argument
 */
static HANDLE_FUNC(handle_user)
{
        return set_string_arg(&conf->username, line, &match[2]);
}
static HANDLE_FUNC(handle_group)
{
        return set_string_arg(&conf->group, line, &match[2]);
}


/*
 * IP addresses
 */
static HANDLE_FUNC(handle_allow)
{
        char* arg = get_string_arg(line, &match[2]);
        insert_acl(arg, ACL_ALLOW);
        safefree(arg);
        return 0;
}
static HANDLE_FUNC(handle_deny)
{
        char *arg = get_string_arg(line, &match[2]);
        insert_acl(arg, ACL_DENY);
        safefree(arg);
        return 0;
}
static HANDLE_FUNC(handle_bind)
{
        return set_string_arg(&conf->bind_address, line, &match[2]);
}
static HANDLE_FUNC(handle_listen)
{
        return set_string_arg(&conf->ipAddr, line, &match[2]);
}


/*
 * Error file has a integer and string argument
 */
static HANDLE_FUNC(handle_errorfile)
{
        long int err = get_int_arg(line, &match[2]);
        char *page = get_string_arg(line, &match[3]);
        add_new_errorpage(page, err);
        safefree(page);
        return 0;
}


/*
 * Log level's are strings.
 */
struct log_levels_s {
        const char* string;
        int level;
};
static struct log_levels_s log_levels[] = {
        { "critical", LOG_CRIT },
        { "error", LOG_ERR },
        { "warning", LOG_WARNING },
        { "notice", LOG_NOTICE },
        { "connect", LOG_CONN },
        { "info", LOG_INFO }
};

static HANDLE_FUNC(handle_loglevel)
{
        static const unsigned int nlevels = sizeof(log_levels)/sizeof(log_levels[0]);
        unsigned int i;
        
        char *arg = get_string_arg(line, &match[2]);
        for (i = 0; i != nlevels; ++i) {
                if (!strcasecmp(arg, log_levels[i].string)) {
                        set_log_level(log_levels[i].level);
                        return 0;
                }
        }
        return -1;
}


#ifdef FILTER_ENABLE
static HANDLE_FUNC(handle_filter)
{
        return set_string_arg(&conf->filter, line, &match[2]);
}
static HANDLE_FUNC(handle_filterurls)
{
        return set_bool_arg(&conf->filter_url, line, &match[2]);
}
static HANDLE_FUNC(handle_filterextended)
{
        return set_bool_arg(&conf->filter_extended, line, &match[2]);
}
static HANDLE_FUNC(handle_filterdefaultdeny)
{
        assert(match[2].rm_so != -1);
        
        if (get_bool_arg(line, &match[2]))
                filter_set_default_policy(FILTER_DEFAULT_DENY);
        return 0;
}
static HANDLE_FUNC(handle_filtercasesensitive)
{
        return set_bool_arg(&conf->filter_casesensitive, line, &match[2]);
}
#endif


#ifdef REVERSE_SUPPORT
static HANDLE_FUNC(handle_reverseonly)
{
        return set_bool_arg(&conf->reverseonly, line, &match[2]);
}
static HANDLE_FUNC(handle_reversemagic)
{
        return set_bool_arg(&conf->reversemagic, line, &match[2]);
}
#endif
